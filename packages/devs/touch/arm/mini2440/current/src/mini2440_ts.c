//==========================================================================
//
//      mini2440_ts.c
//
//      Touchscreen driver for the mini2440
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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
//#####DESCRIPTIONBEGIN####
//
// Author(s):    Ricky Wu
// Contributors: Ricky Wu
// Date:         2011-03-30
// Purpose:      
// Description:  Touchscreen driver for MINI2440
//
//####DESCRIPTIONEND####
//
//==========================================================================


#include <pkgconf/devs_touch_mini2440.h>

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/s3c2440x.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>

#include <cyg/fileio/fileio.h>  // For select() functionality
static cyg_selinfo      ts_select_info; 
static cyg_bool         ts_select_active;

#include <cyg/io/devtab.h>

/* ADS7846 flags */
#define ADS_START               (1 << 7)
#define ADS_MEASURE_Y           (0x01 << 4)
#define ADS_MEASURE_X           (0x05 << 4)
#define ADS_MODE_12_BIT         0
#define ADS_PD0                 0

// Misc constants
#define TS_INT (1<<0)
#define X_THRESHOLD 0x80
#define Y_THRESHOLD 0x80

/* ADCCON Register Bits */
#define S3C2410_ADCCON_ECFLG		(1<<15)
#define S3C2410_ADCCON_PRSCEN		(1<<14)
#define S3C2410_ADCCON_PRSCVL(x)	(((x)&0xFF)<<6)
#define S3C2410_ADCCON_PRSCVLMASK	(0xFF<<6)
#define S3C2410_ADCCON_SELMUX(x)	(((x)&0x7)<<3)
#define S3C2410_ADCCON_MUXMASK		(0x7<<3)
#define S3C2410_ADCCON_STDBM		(1<<2)
#define S3C2410_ADCCON_READ_START	(1<<1)
#define S3C2410_ADCCON_ENABLE_START	(1<<0)
#define S3C2410_ADCCON_STARTMASK	(0x3<<0)


/* ADCTSC Register Bits */
#define S3C2410_ADCTSC_UD_SEN		(1<<8) /* ghcstop add for s3c2440a */
#define S3C2410_ADCTSC_YM_SEN		(1<<7)
#define S3C2410_ADCTSC_YP_SEN		(1<<6)
#define S3C2410_ADCTSC_XM_SEN		(1<<5)
#define S3C2410_ADCTSC_XP_SEN		(1<<4)
#define S3C2410_ADCTSC_PULL_UP_DISABLE	(1<<3)
#define S3C2410_ADCTSC_AUTO_PST		(1<<2)
#define S3C2410_ADCTSC_XY_PST(x)	(((x)&0x3)<<0)

/* ADCDAT0 Bits */
#define S3C2410_ADCDAT0_UPDOWN		(1<<15)
#define S3C2410_ADCDAT0_AUTO_PST	(1<<14)
#define S3C2410_ADCDAT0_XY_PST		(0x3<<12)
#define S3C2410_ADCDAT0_XPDATA_MASK	(0x03FF)

/* ADCDAT1 Bits */
#define S3C2410_ADCDAT1_UPDOWN		(1<<15)
#define S3C2410_ADCDAT1_AUTO_PST	(1<<14)
#define S3C2410_ADCDAT1_XY_PST		(0x3<<12)
#define S3C2410_ADCDAT1_YPDATA_MASK	(0x03FF)

#define WAIT4INT(x)  (((x)<<8) | \
		     S3C2410_ADCTSC_YM_SEN | S3C2410_ADCTSC_YP_SEN | S3C2410_ADCTSC_XP_SEN | \
		     S3C2410_ADCTSC_XY_PST(3))

#define AUTOPST	     (S3C2410_ADCTSC_YM_SEN | S3C2410_ADCTSC_YP_SEN | S3C2410_ADCTSC_XP_SEN | \
		     S3C2410_ADCTSC_AUTO_PST | S3C2410_ADCTSC_XY_PST(0))

#ifdef CYGPKG_IO_FRAMEBUF
#include <cyg/io/framebuf.h>
#include <cyg/io/framebufs/framebufs.h>
#define WIDTH    CYG_FB_WIDTH(fb0)
#define HEIGHT   CYG_FB_HEIGHT(fb0)
#else
#define WIDTH    640
#define HEIGHT   480
#endif

// Functions in this module

static Cyg_ErrNo ts_read(cyg_io_handle_t handle, 
                         void *buffer, 
                         cyg_uint32 *len);
static cyg_bool  ts_select(cyg_io_handle_t handle, 
                           cyg_uint32 which, 
                           cyg_addrword_t info);
static Cyg_ErrNo ts_set_config(cyg_io_handle_t handle, 
                               cyg_uint32 key, 
                               const void *buffer, 
                               cyg_uint32 *len);
static Cyg_ErrNo ts_get_config(cyg_io_handle_t handle, 
                               cyg_uint32 key, 
                               void *buffer, 
                               cyg_uint32 *len);
static bool      ts_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo ts_lookup(struct cyg_devtab_entry **tab, 
                           struct cyg_devtab_entry *st, 
                           const char *name);

CHAR_DEVIO_TABLE(mini2440_ts_handlers,
                 NULL,                                   // Unsupported write() function
                 ts_read,
                 ts_select,
                 ts_get_config,
                 ts_set_config);

CHAR_DEVTAB_ENTRY(mini2440_ts_device,
                  CYGDAT_DEVS_TOUCH_MINI2440_NAME,
                  NULL,                                   // Base device name
                  &mini2440_ts_handlers,
                  ts_init,
                  ts_lookup,
                  NULL);                                  // Private data pointer

struct _event {
    short button_state;
    short xPos, yPos;
    short _unused;
};
#define MAX_EVENTS CYGNUM_DEVS_TOUCH_MINI2440_EVENT_BUFFER_SIZE
static int   num_events;
static int   _event_put, _event_get;
static struct _event _events[MAX_EVENTS];

static bool _is_open = false;
#ifdef DEBUG_RAW_EVENTS
static unsigned char _ts_buf[512];
static int _ts_buf_ptr = 0;
#endif

#define STACK_SIZE CYGNUM_HAL_STACK_SIZE_TYPICAL
static cyg_thread ts_thread_data;
static cyg_handle_t ts_thread_handle;
#define SCAN_FREQ 20 // Hz
//#define SCAN_FREQ 5 // Hz
#define SCAN_DELAY ((1000/SCAN_FREQ)/10)

#if CYGPKG_DEVS_TOUCH_BLOCKREAD
static cyg_mutex_t res_lock;
static cyg_cond_t res_wait;
#endif

typedef struct {
    short min;
    short max;
    short span;
} bounds;

static bounds xBounds = {75, 940, 940-75};
static bounds yBounds = {0, 935-60, 935-60};

static Cyg_ErrNo 
ts_read(cyg_io_handle_t handle, 
        void *buffer, 
        cyg_uint32 *len)
{
    struct _event *ev;
    int tot = *len;
    unsigned char *bp = (unsigned char *)buffer;
    //diag_printf("ts_read\n");
#if CYGPKG_DEVS_TOUCH_BLOCKREAD
	cyg_mutex_lock(&res_lock);
	cyg_cond_wait(&res_wait);
#endif
    cyg_scheduler_lock();  // Prevent interaction with DSR code
    while (tot >= sizeof(struct _event)) {
        if (num_events > 0) {
            ev = &_events[_event_get++];
            if (_event_get == MAX_EVENTS) {
                _event_get = 0;
            }
            // Self calibrate
            /*
            if (ev->xPos > xBounds.max) xBounds.max = ev->xPos;
            if (ev->xPos < xBounds.min) xBounds.min = ev->xPos;
            if (ev->yPos > yBounds.max) yBounds.max = ev->yPos;
            if (ev->yPos < yBounds.min) yBounds.min = ev->yPos;
            if ((xBounds.span = xBounds.max - xBounds.min) <= 1) {
                xBounds.span = 1;
            }
            if ((yBounds.span = yBounds.max - yBounds.min) <= 1) {
                yBounds.span = 1;
            }
            */
            // Scale values - done here so these potentially lengthy
            // operations take place outside of interrupt processing
#ifdef DEBUG
            diag_printf("Raw[%d,%d], X[%d,%d,%d], Y[%d,%d,%d]",
                        ev->xPos, ev->yPos,
                        xBounds.max, xBounds.min, xBounds.span,
                        yBounds.max, yBounds.min, yBounds.span);
#endif

            ev->xPos = WIDTH - (((xBounds.max - ev->xPos) * WIDTH) / xBounds.span);
            ev->yPos = HEIGHT - (((yBounds.max - ev->yPos) * HEIGHT) / yBounds.span);
#ifdef DEBUG
            diag_printf(", Cooked[%d,%d]\n",
                        ev->xPos, ev->yPos);
#endif
            memcpy(bp, ev, sizeof(*ev));
            bp += sizeof(*ev);
            tot -= sizeof(*ev);
            num_events--;
        } else {
            break;  // No more events
        }
    }
    cyg_scheduler_unlock(); // Allow DSRs again
#if CYGPKG_DEVS_TOUCH_BLOCKREAD
	cyg_mutex_unlock(&res_lock);
#endif
    *len -= tot;
    return ENOERR;
}

static cyg_bool  
ts_select(cyg_io_handle_t handle, 
          cyg_uint32 which, 
          cyg_addrword_t info)
{
    if (which == CYG_FREAD) {
        cyg_scheduler_lock();  // Prevent interaction with DSR code
        if (num_events > 0) {
            cyg_scheduler_unlock();  // Reallow interaction with DSR code
            return true;
        }        
        if (!ts_select_active) {
            ts_select_active = true;
            cyg_selrecord(info, &ts_select_info);
        }
        cyg_scheduler_unlock();  // Reallow interaction with DSR code
    }
    return false;
}

static Cyg_ErrNo 
ts_set_config(cyg_io_handle_t handle, 
              cyg_uint32 key, 
              const void *buffer, 
              cyg_uint32 *len)
{
    return EINVAL;
}

static Cyg_ErrNo 
ts_get_config(cyg_io_handle_t handle, 
              cyg_uint32 key, 
              void *buffer, 
              cyg_uint32 *len)
{
    return EINVAL;
}

static bool      
ts_init(struct cyg_devtab_entry *tab)
{
    cyg_uint32 _dummy;
    HAL_WRITE_UINT32(ADCCON,S3C2410_ADCCON_PRSCEN | S3C2410_ADCCON_PRSCVL(9));
    HAL_WRITE_UINT32(ADCDLY,50000);
    HAL_WRITE_UINT32(ADCTSC,WAIT4INT(0));

    //HAL_WRITE_UINT32(INTMSK, BIT_ALLMSK);
    HAL_READ_UINT32(SUBSRCPND, _dummy);
    _dummy |= BIT_SUB_TC;
    _dummy |= BIT_SUB_ADC;
    HAL_WRITE_UINT32(SUBSRCPND, _dummy);

    
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_ADC);
    cyg_selinit(&ts_select_info);
    /* install interrupt handler */
    HAL_READ_UINT32(INTSUBMSK, _dummy);
    _dummy &= ~BIT_SUB_ADC;
    _dummy &= ~BIT_SUB_TC;
    HAL_WRITE_UINT32(INTSUBMSK, _dummy);
    HAL_INTERRUPT_UNMASK(CYGNUM_HAL_INTERRUPT_ADC); 
    return true;
}

static cyg_uint32
read_ts_y(void)
{
    
    cyg_uint32 res;
    
    HAL_READ_UINT32(ADCDAT0, res);
    res &= S3C2410_ADCDAT0_XPDATA_MASK;
    res = 935 - res;
    //diag_printf("read_ts_y :%x\n",res);

    return res;
}

static cyg_uint32
read_ts_x(void)
{

    cyg_uint32 res;
    
    HAL_READ_UINT32(ADCDAT1, res);
    res &= S3C2410_ADCDAT1_YPDATA_MASK;
    //diag_printf("read_ts_x :%x\n",res);

    return res;
}

static void
cyg_mini2440_ts_isr(cyg_addrword_t param)
{
   cyg_uint32 res;
   cyg_uint32 reg;
   cyg_uint32 data0;
   cyg_uint32 data1;
   cyg_int32 updown;
   static short lastX=0, lastY=0;
   short x, y;
   struct _event *ev;
   static bool pen_down=false;
   static bool report_touch_input=false;

   //diag_printf("isrrrrrrrrrrrrrrrrrrrrrrrr\n");
   HAL_READ_UINT32(SUBSRCPND, res);

   if (res& (1 << 10))
   {
       //diag_printf("ADC Interrupt\n");
       report_touch_input=true;
       HAL_READ_UINT32(SUBSRCPND, reg);
       reg |= BIT_SUB_ADC;
       HAL_WRITE_UINT32(SUBSRCPND, reg);
       
       HAL_READ_UINT32(ADCDAT0, data0);
       HAL_READ_UINT32(ADCDAT1, data1);
       
     
       updown = ((data0 & S3C2410_ADCDAT0_UPDOWN)) || ((data1 & S3C2410_ADCDAT0_UPDOWN));
       
       if(updown)//Penup
       {
           //diag_printf("ADC pen_up#######################################################################\n");
           x = lastX;
           y = lastY;
           //diag_printf("X = %x, Y = %x\n", x, y);
           pen_down=false;
           HAL_WRITE_UINT32(ADCTSC,WAIT4INT(0));         
       }
       if(pen_down)
       {
           x = read_ts_x();
           y = read_ts_y();
           //diag_printf("X = %x, Y = %x\n", x, y);
           lastX = x;  lastY = y;
           HAL_WRITE_UINT32(ADCTSC, S3C2410_ADCTSC_PULL_UP_DISABLE | AUTOPST);
	   HAL_READ_UINT32(ADCCON, reg);
           reg |= S3C2410_ADCCON_ENABLE_START;
           HAL_WRITE_UINT32(ADCCON, reg);
       }
       else
       {
          //diag_printf("ADC pen_up#######################################################################\n");
          x = lastX;
          y = lastY;
          //diag_printf("X = %x, Y = %x\n", x, y);
          HAL_WRITE_UINT32(ADCTSC,WAIT4INT(1));
       }
   }
   if (res& (1 << 9)){
       //diag_printf("TS Interrupt\n");
       
       HAL_READ_UINT32(SUBSRCPND, reg);
       reg |= BIT_SUB_TC;
       HAL_WRITE_UINT32(SUBSRCPND, reg);

       HAL_READ_UINT32(ADCDAT0, data0);
       HAL_READ_UINT32(ADCDAT1, data1);
       
     
       updown = (!(data0 & S3C2410_ADCDAT0_UPDOWN)) && (!(data1 & S3C2410_ADCDAT0_UPDOWN));
       
       if(updown)
       {   
            report_touch_input = false;
            x=lastX;
            y=lastY;	     
            //diag_printf("X = %x, Y = %x\n", x, y);
	    //diag_printf("pen_down#######################################################################\n");
            if ((x < X_THRESHOLD) || (y < Y_THRESHOLD)) {
                // Ignore 'bad' samples
                 }
            //lastX = x;  lastY = y;
            HAL_WRITE_UINT32(ADCTSC, S3C2410_ADCTSC_PULL_UP_DISABLE | AUTOPST);
	    HAL_READ_UINT32(ADCCON, reg);
            reg |= S3C2410_ADCCON_ENABLE_START;
            HAL_WRITE_UINT32(ADCCON, reg);
            pen_down = true;
       }
       else
       {
       	    report_touch_input = true;
       	    x = lastX;
            y = lastY;
            //diag_printf("X = %x, Y = %x\n", x, y);
            HAL_WRITE_UINT32(ADCTSC,WAIT4INT(0));
            pen_down = false;
	    //diag_printf("pen_up#######################################################################\n");
       }
   }

   if (report_touch_input){
       if (num_events < MAX_EVENTS) {
            num_events++;
            ev = &_events[_event_put++];
            if (_event_put == MAX_EVENTS) {
                _event_put = 0;
            }
            ev->button_state = pen_down ? 0x04 : 0x00;
            ev->xPos = x;
            ev->yPos = y;
            if (ts_select_active) {
                ts_select_active = false;
                cyg_selwakeup(&ts_select_info);
            }
       }
   }
        
   cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_ADC);

   return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;
}

static void
cyg_mini2440_ts_dsr(cyg_addrword_t param)
{
#if CYGPKG_DEVS_TOUCH_BLOCKREAD
	cyg_cond_signal(&res_wait);
#endif
}

static Cyg_ErrNo 
ts_lookup(struct cyg_devtab_entry **tab, 
          struct cyg_devtab_entry *st, 
          const char *name)
{
    if (!_is_open) {
        _is_open = true;
		
#if CYGPKG_DEVS_TOUCH_BLOCKREAD
		cyg_drv_mutex_init(&res_lock);
		cyg_drv_cond_init(&res_wait,&res_lock);
#endif

       cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_ADC,
                             0,
                             (CYG_ADDRWORD)0,
                             cyg_mini2440_ts_isr,
                             cyg_mini2440_ts_dsr,
                             &ts_thread_handle,
                             &ts_thread_data);
       cyg_drv_interrupt_attach(ts_thread_handle);
       cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_ADC);
    }
    return ENOERR;
}



