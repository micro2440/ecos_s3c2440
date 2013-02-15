//==========================================================================
//
//      synthts.c
//
//      Provide touchscreen (implemented through the mouse) for framebuffer
//      devices for the synthetic target.
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2010 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later
// version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with eCos; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// As a special exception, if other files instantiate templates or use
// macros or inline functions from this file, or you compile this file
// and link it with other works to produce a work based on this file,
// this file does not by itself cause the resulting work to be covered by
// the GNU General Public License. However the source code for this file
// must still be made available in accordance with section (3) of the GNU
// General Public License v2.
//
// This exception does not invalidate any other reasons why a work based
// on this file might be covered by the GNU General Public License.
// -------------------------------------------
// ####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//###DESCRIPTIONBEGIN####
//
// Author(s):     Rutger Hofman, VU Amsterdam
// Date:          2010-07-20
//
//###DESCRIPTIONEND####
//========================================================================

#include <pkgconf/devs_framebuf_synth.h>
#include <pkgconf/devs_touch_synth.h>
#include <pkgconf/io_framebuf.h>
#include <cyg/io/framebuf.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_io.h>
#include <cyg/io/io.h>
#include <cyg/kernel/kapi.h>
#include <cyg/fileio/fileio.h>
#include <cyg/io/devtab.h>

#include <cyg/framebuf/protocol.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>      // sscanf

#if defined CYGPKG_DEVS_FRAMEBUF_SYNTH_FB0_TOUCHSCREEN \
    || defined CYGPKG_DEVS_FRAMEBUF_SYNTH_FB1_TOUCHSCREEN \
    || defined CYGPKG_DEVS_FRAMEBUF_SYNTH_FB2_TOUCHSCREEN \
    || defined CYGPKG_DEVS_FRAMEBUF_SYNTH_FB3_TOUCHSCREEN
#  define REQUIRE_TOUCHSCREEN
#endif

// Set the DEBUG_LEVEL to 0 for no debugging, 2 for maximum debugging.
#define DEBUG_LEVEL 0
#define DEBUG(_level_, _str_, ...)              \
    CYG_MACRO_START                             \
    if (_level_ <= DEBUG_LEVEL) {               \
        diag_printf( _str_, ## __VA_ARGS__);    \
    }                                           \
    CYG_MACRO_END


typedef struct CYG_SYNTH_TS_EVENT {
    cyg_uint16          type;   /* 0x00 = up, 0x04 = down */
    cyg_uint16          x;
    cyg_uint16          y;
    cyg_uint16          unused_0;
} cyg_synth_ts_event_t;


typedef struct CYG_SYNTH_TS_EVENT_Q {
#ifdef CYGDBG_USE_ASSERTS
#define TS_Q_MAGIC      0x0a0b0c0d
    cyg_uint32          magic;
#endif
    int                 append;
    int                 consume;
    int                 n;
    cyg_synth_ts_event_t event[CYGNUM_DEVS_TOUCH_SYNTH_EVENT_BUFFER_SIZE];
    struct {
        int             active;
        cyg_selinfo     info;
    } select;
} cyg_synth_ts_event_q_t;


typedef struct CYG_SYNTH_FB_TS {
    int                 vector;
    cyg_handle_t        handle;
    cyg_interrupt       data;
    int                 devid;
    cyg_synth_ts_event_q_t q;
} cyg_synth_ts_t;


static int
cyg_synth_event_q_inc(int i)
{
    i++;

    if (i == CYGNUM_DEVS_TOUCH_SYNTH_EVENT_BUFFER_SIZE) {
        return 0;
    }

    return i;
}


#ifdef REQUIRE_TOUCHSCREEN

static Cyg_ErrNo 
cyg_synth_ts_read(cyg_io_handle_t handle, 
                  void *buffer, 
                  cyg_uint32 *len)
{
    int tot = *len;
    unsigned char *bp = buffer;
    struct cyg_devtab_entry *dt = (struct cyg_devtab_entry *)handle;
    cyg_synth_ts_t *ts = dt->priv;
    cyg_synth_ts_event_q_t *q = &ts->q;

    CYG_ASSERT(q->magic == TS_Q_MAGIC, "ts q magic corrupted");

    cyg_scheduler_lock();  // Prevent interaction with DSR code
    while (q->n > 0) {
        int     i = q->consume;
        cyg_synth_ts_event_t *e = &q->event[i];

        if (tot < sizeof *e) {
            break;
        }

        memcpy(bp, e, sizeof *e);
        bp += sizeof *e;
        tot -= sizeof *e;
        q->n--;
        q->consume = cyg_synth_event_q_inc(i);
    }
    cyg_scheduler_unlock(); // Allow DSRs again

    *len -= tot;

    return ENOERR;
}


static cyg_bool  
cyg_synth_ts_select(cyg_io_handle_t handle, 
                    cyg_uint32 which, 
                    cyg_addrword_t info)
{
    struct cyg_devtab_entry *dt = (struct cyg_devtab_entry *)handle;
    cyg_synth_ts_t *ts = dt->priv;
    cyg_synth_ts_event_q_t *q = &ts->q;

    CYG_ASSERT(q->magic == TS_Q_MAGIC, "ts q magic corrupted");

    if (which == CYG_FREAD) {
        cyg_scheduler_lock();  // Prevent interaction with DSR code
        if (q->n > 0) {
            cyg_scheduler_unlock();  // Reallow interaction with DSR code
            return true;
        }        
        if (! q->select.active) {
            q->select.active = true;
            cyg_selrecord(info, &q->select.info);
        }
        cyg_scheduler_unlock();  // Reallow interaction with DSR code
    }
    return false;
}


static Cyg_ErrNo 
cyg_synth_ts_set_config(cyg_io_handle_t handle, 
                        cyg_uint32 key, 
                        const void *buffer, 
                        cyg_uint32 *len)
{
    return EINVAL;
}


static Cyg_ErrNo 
cyg_synth_ts_get_config(cyg_io_handle_t handle, 
                        cyg_uint32 key, 
                        void *buffer, 
                        cyg_uint32 *len)
{
    return EINVAL;
}


static bool      
cyg_synth_ts_init(struct cyg_devtab_entry *tab)
{
    cyg_synth_ts_t *ts = tab->priv;
    cyg_synth_ts_event_q_t *q = &ts->q;

    cyg_selinit(&q->select.info);
#ifdef CYGDBG_USE_ASSERTS
    q->magic = TS_Q_MAGIC;
#endif

    return true;
}


static Cyg_ErrNo
cyg_synth_ts_lookup(struct cyg_devtab_entry **tab,
                    struct cyg_devtab_entry *st,
                    const char *name)
{
    return ENOERR;
}

#endif

#define TS_DEVICE(num) \
static cyg_synth_ts_t cyg_synth_fb ## num ## _ts;                       \
                                                                        \
CHAR_DEVIO_TABLE(cyg_synth_ts ## num ## _handlers,                      \
                 NULL,    /* No write() function */                     \
                 cyg_synth_ts_read,                                     \
                 cyg_synth_ts_select,                                   \
                 cyg_synth_ts_get_config,                               \
                 cyg_synth_ts_set_config)                               \
                                                                        \
CHAR_DEVTAB_ENTRY(cyg_synth_ts ## num ## _device,                       \
                  "/dev/ts" #num,                                       \
                  NULL,                                                 \
                  &cyg_synth_ts ## num ## _handlers,                    \
                  cyg_synth_ts_init,                                    \
                  cyg_synth_ts_lookup,                                  \
                  &cyg_synth_fb ## num ## _ts)

#ifdef CYGPKG_DEVS_FRAMEBUF_SYNTH_FB0_TOUCHSCREEN
TS_DEVICE(0)
#endif
#ifdef CYGPKG_DEVS_FRAMEBUF_SYNTH_FB1_TOUCHSCREEN
TS_DEVICE(1)
#endif
#ifdef CYGPKG_DEVS_FRAMEBUF_SYNTH_FB2_TOUCHSCREEN
TS_DEVICE(2)
#endif
#ifdef CYGPKG_DEVS_FRAMEBUF_SYNTH_FB3_TOUCHSCREEN
TS_DEVICE(3)
#endif


static cyg_uint32
cyg_synth_ts_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_drv_interrupt_acknowledge(vector);

    return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;
}


static void
cyg_synth_ts_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_synth_ts_t *ts = (cyg_synth_ts_t *)data;
    cyg_synth_ts_event_q_t *q = &ts->q;
    int         reply;
    unsigned char resp[512];
    int         len;
    int         type;
    int         x;
    int         y;

    CYG_ASSERT(q->magic == TS_Q_MAGIC, "ts q magic corrupted");

    DEBUG(1, "In synth fb ts DSR; vector %d devid %d\n",
          (int)vector, ts->devid);

    synth_auxiliary_xchgmsg(ts->devid,
                            SYNTH_FB_TS_EVENT, 0, 0, NULL, 0,
                            &reply,
                            resp,
                            &len,
                            sizeof resp);

    DEBUG(1, "Receive response[%d]: ", len);
    int i;
    for (i = 0; i < len; i++) {
        DEBUG(1, "%02x '%c' ", resp[i], resp[i]);
    }
    DEBUG(1, "\n");
    if (len == sizeof resp) {
        resp[len - 1] = '\0';
    } else {
        resp[len] = '\0';
    }
    if (sscanf((char *)resp, "0x%x %d %d", &type, &x, &y) != 3) {
        diag_printf("OOOOPPPPPPPPPPPSSSSSSSSSSSSS malformatted touchscreen event response\n");
    }
    DEBUG(1, "Received touchscreen status '%s'\n", resp);

    if (q->n == CYGNUM_DEVS_TOUCH_SYNTH_EVENT_BUFFER_SIZE) {
        diag_printf("Run out of unhandeld touchscreen events, discard\n");
        /* full, discard the message */
    } else {
        cyg_synth_ts_event_t *e = &q->event[q->append];
        int     next = cyg_synth_event_q_inc(q->append);

        e->type = type;
        e->x = x;
        e->y = y;

        q->append = next;
        q->n++;
    }

    if (q->select.active) {
        q->select.active = 0;
        cyg_selwakeup(&q->select.info);
    }
}


cyg_synth_ts_t *
cyg_ts_synth_instantiate(int devid, int fb_number)
{
    // Get the touchscreen interrupt vector, instantiate handler for it
    cyg_synth_ts_t *ts;
    unsigned char resp_buf[3];      /* from 1 to 31 */
    int         len;
    int         reply;

    DEBUG(1, "get touchscreen interrupt vector\n");

    switch (fb_number) {
#ifdef CYGPKG_DEVS_FRAMEBUF_SYNTH_FB0_TOUCHSCREEN
    case 0:
        ts = &cyg_synth_fb0_ts;
        break;
#endif
#ifdef CYGPKG_DEVS_FRAMEBUF_SYNTH_FB1_TOUCHSCREEN
    case 1:
        ts = &cyg_synth_fb1_ts;
        break;
#endif
#ifdef CYGPKG_DEVS_FRAMEBUF_SYNTH_FB2_TOUCHSCREEN
    case 2:
        ts = &cyg_synth_fb2_ts;
        break;
#endif
#ifdef CYGPKG_DEVS_FRAMEBUF_SYNTH_FB3_TOUCHSCREEN
    case 3:
        ts = &cyg_synth_fb3_ts;
        break;
#endif
    default:
        return NULL;
    }

    ts->devid = devid;

    synth_auxiliary_xchgmsg(devid,
                            SYNTH_FB_TS_SETUP, 0, 0, NULL, 0,
                            &reply,
                            resp_buf,
                            &len,
                            sizeof resp_buf);
    if (len == sizeof resp_buf) {
        resp_buf[len - 1] = '\0';
    } else {
        resp_buf[len] = '\0';
    }
    DEBUG(1, "recv ts vector response, len %d value %s\n", len, resp_buf);
    if (sscanf((char *)resp_buf, "%u", &ts->vector) != 1) {
        return NULL;
    }
    cyg_drv_interrupt_create(ts->vector,
                             0,
                             (CYG_ADDRWORD)ts,
                             cyg_synth_ts_isr,
                             cyg_synth_ts_dsr,
                             &ts->handle,
                             &ts->data);
    cyg_drv_interrupt_attach(ts->handle);
    cyg_drv_interrupt_unmask(ts->vector);

    DEBUG(1, "fb ts intr vector %d\n", (int)ts->vector);

    return ts;
}
