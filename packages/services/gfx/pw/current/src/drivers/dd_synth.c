/*===========================================================================
//
//        dd_synth.c
//
//        PW display driver for eCos synthetic target
//
//===========================================================================
//===========================================================================
// This file is part of PW graphic library.
// Copyright (C) 2004 Savin Zlobec 
//
// PW is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// PW is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with PW; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
//===========================================================================
//===========================================================================
//
//  Author(s): Savin Zlobec
//
//=========================================================================== */

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_io.h>
#include <cyg/infra/diag.h>

#include <i_pwin.h>

// -------------------------------------------------------------------------- 

#define DPY_WIDTH  160
#define DPY_HEIGHT 64

// -------------------------------------------------------------------------- 

#define P_GETPARAMS   0x01
#define P_GETKEY      0x02
#define P_SETPIXELS   0x03
#define P_RESETPIXELS 0x04
#define P_SETMISC     0x05

// -------------------------------------------------------------------------- 

static int synth_id = -1;

static cyg_handle_t  key_int_handle;
static cyg_interrupt key_int_data;

static int       set_pixels_cnt = 0;
static int       clr_pixels_cnt = 0;
static cyg_uint8 set_pixels_buf[DPY_WIDTH * DPY_HEIGHT * 2];
static cyg_uint8 clr_pixels_buf[DPY_WIDTH * DPY_HEIGHT * 2];

static cyg_alarm    to_alarm;
static cyg_handle_t to_alarm_h;
static cyg_mutex_t  dd_lock;
static cyg_cond_t   dd_cond;

// -------------------------------------------------------------------------- 

static Pw_DD      dd;
static Pw_LLD     lld;
static Pw_Display dpy;
static Pw_Bitmap  bit_buff1;
static Pw_Bitmap  bit_buff2;
static Pw_Byte    bit_buff1_bits[((DPY_WIDTH*DPY_HEIGHT + 0x07) >> 3)];
static Pw_Byte    bit_buff2_bits[((DPY_WIDTH*DPY_HEIGHT + 0x07) >> 3)];

// -------------------------------------------------------------------------- 

static void
set_pixel(unsigned int x, unsigned int y, unsigned int mode)
{
    unsigned int addr = (y * DPY_WIDTH) + x;

    if (mode)
    {
        set_pixels_buf[set_pixels_cnt++] = addr & 0xFF;    
        set_pixels_buf[set_pixels_cnt++] = (addr >> 8) & 0xFF;  
    }
    else
    {
        clr_pixels_buf[clr_pixels_cnt++] = addr & 0xFF;    
        clr_pixels_buf[clr_pixels_cnt++] = (addr >> 8) & 0xFF;  
    }
}

static void
flush_pix_buf(void)
{
    if (set_pixels_cnt > 0)
    { 
        synth_auxiliary_xchgmsg(synth_id, P_SETPIXELS, 0, 0, 
                                set_pixels_buf, set_pixels_cnt, 0, 0, 0, 0);
        set_pixels_cnt = 0;
    }
    if (clr_pixels_cnt > 0) 
    {
        synth_auxiliary_xchgmsg(synth_id, P_RESETPIXELS, 0, 0, 
                                clr_pixels_buf, clr_pixels_cnt, 0, 0, 0, 0);
        clr_pixels_cnt = 0;
    }
}

// -------------------------------------------------------------------------- 

static void
to_alarm_fn(cyg_handle_t alarm, cyg_addrword_t data)
{
    cyg_cond_signal(&dd_cond);
}

static void
to_alarm_init(void)
{
    cyg_handle_t h;

    cyg_clock_to_counter(cyg_real_time_clock(), &h);
    cyg_alarm_create(h, 
                     to_alarm_fn, 
                     (cyg_addrword_t) 0,
                     &to_alarm_h, 
                     &to_alarm);
    cyg_alarm_initialize( to_alarm_h, cyg_current_time() + 1, 1 );
}

// -------------------------------------------------------------------------- 

static void 
init_bit_buff(void)
{
    Pw_BitmapCreate(DPY_WIDTH, DPY_HEIGHT, bit_buff1_bits, &bit_buff1);
    Pw_BitmapCreate(DPY_WIDTH, DPY_HEIGHT, bit_buff2_bits, &bit_buff2);
    IPw_BitmapFillRect(&bit_buff1, 0, 0, DPY_WIDTH, DPY_HEIGHT, 0);
    IPw_BitmapFillRect(&bit_buff2, 0, 0, DPY_WIDTH, DPY_HEIGHT, 0);
}

static void 
free_bit_buff(void)
{
    Pw_BitmapDestroy(&bit_buff1);
    Pw_BitmapDestroy(&bit_buff2);
}

static void 
draw_point(Pw_GC* gc, Pw_Coord x, Pw_Coord y)
{
    IPw_BitmapDrawPoint(&bit_buff1, x, y, 
        ((Pw_GCGetColor(gc) == pw_black_pixel) ? 1 : 0));
} 

static void
draw_points(Pw_GC* gc, Pw_Coord x, Pw_Coord y, Pw_Byte bits)
{
    IPw_BitmapDrawPoints(&bit_buff1, x, y, bits,
        ((Pw_GCGetColor(gc) == pw_black_pixel) ? 1 : 0));
} 

static void 
draw_hor_line(Pw_GC* gc, Pw_Coord x1, Pw_Coord y1, Pw_Coord x2)
{
    IPw_BitmapDrawHorLine(&bit_buff1, x1, y1, x2,
        ((Pw_GCGetColor(gc) == pw_black_pixel) ? 1 : 0));
} 

static void 
draw_ver_line(Pw_GC* gc, Pw_Coord x1, Pw_Coord y1, Pw_Coord y2)
{
    IPw_BitmapDrawVerLine(&bit_buff1, x1, y1, y2,
        ((Pw_GCGetColor(gc) == pw_black_pixel) ? 1 : 0));
} 

static void 
fill_rect(Pw_GC* gc, Pw_Coord x, Pw_Coord y, Pw_Coord w, Pw_Coord h)
{
    IPw_BitmapFillRect(&bit_buff1, x, y, w, h,
        ((Pw_GCGetColor(gc) == pw_black_pixel) ? 1 : 0));
} 

static void 
display_refresh(Pw_Display* dpy, Pw_Rectangle* area)
{
    IPw_BitmapPixDiff(&bit_buff1, &bit_buff2, area, set_pixel);
    flush_pix_buf();
}

static Pw_Int
display_get_xinput_value(Pw_Display* dpy, Pw_uInt num)
{
    return 0;
}

static void 
open_display(void)
{
    init_bit_buff();
}

static void 
close_display(void)
{
    free_bit_buff();
}

// -------------------------------------------------------------------------- 

static cyg_uint32
key_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);
   
    return CYG_ISR_CALL_DSR;
}

static void 
key_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_cond_signal(&dd_cond);
    cyg_interrupt_unmask(vector);
}

static bool 
get_key_ev(int *key_num, bool *pressed)
{
    int         replay_code;
    cyg_uint8   replay_data[32];

    synth_auxiliary_xchgmsg(synth_id, P_GETKEY, 0, 0,
                            0, 0, &replay_code, replay_data, 0, 2);

    if (replay_code >= 0)
    {
        *key_num = replay_data[0];
        *pressed = (1 == replay_data[1]);

        return true;
    }

    return false;
}

// -------------------------------------------------------------------------- 

static void 
synth_init(void)
{
    int       replay_code;
    cyg_uint8 replay_data[32];
    int       key_int_vec;

    if (synth_auxiliary_running)
    {
        synth_id = synth_auxiliary_instantiate(0, 0, "dd_synth", "", "");

        if (-1 == synth_id)
        {
            diag_printf("Can't connect to simulator!\n");
            exit(1); 
        }

        synth_auxiliary_xchgmsg(synth_id, P_GETPARAMS, 0, 0, 
                                0, 0, &replay_code, replay_data, 0, 3);

        key_int_vec = replay_data[0];
        
        cyg_interrupt_create(key_int_vec,
                             0,
                             (CYG_ADDRWORD) 0,
                             (cyg_ISR_t*)key_isr,
                             (cyg_DSR_t*)key_dsr,
                             &key_int_handle,
                             &key_int_data);

        cyg_interrupt_attach(key_int_handle);
        cyg_interrupt_unmask(key_int_vec);
    }
    else
    {
        diag_printf("Auxiliary not running!\n");
        exit(1);
    }
}

// -------------------------------------------------------------------------- 

extern Pw_Bool Pw_Init(Pw_Display* dpy);

bool
pw_dd_init(void)
{
    lld.draw_point      = draw_point;
    lld.draw_points     = draw_points;
    lld.draw_hor_line   = draw_hor_line;
    lld.draw_ver_line   = draw_ver_line;
    lld.fill_rect       = fill_rect;
    dd.lld              = &lld;
    dd.display_refresh  = display_refresh;
    dd.get_xinput_value = display_get_xinput_value;
    dd.init_components  = Pw_Init;

    synth_init();

    cyg_mutex_init(&dd_lock);
    cyg_cond_init(&dd_cond, &dd_lock);

    IPw_InitInternals();

    open_display();

    IPw_DisplayOpen(DPY_WIDTH, DPY_HEIGHT, &dd, &dpy);

    if (!IPw_InitComponents(&dpy))
    {
        IPw_DisplayClose(&dpy);
        close_display();
        return false;
    }

    IPw_DisplayRefresh(&dpy);

    return true;
}

void
pw_dd_main_loop(void)
{
    cyg_tick_count_t otime, ctime;
    int  key_pressed = -1;
    int  key_num;
    bool pressed;

    otime = cyg_current_time() * 10;

    to_alarm_init();

    for (;;) 
    {
        cyg_mutex_lock(&dd_lock);
       
        cyg_cond_wait(&dd_cond);

        while (get_key_ev(&key_num, &pressed))
        {
            if (key_num >= 0)
            {
                if (pressed)
                {
                    if (key_pressed >= 0)
                        IPw_EventKeyReleased(&dpy, key_pressed);
                    IPw_EventKeyPressed(&dpy, key_num);
                    key_pressed = key_num;
                }
                else
                { 
                    if (key_pressed == key_num)
                        IPw_EventKeyReleased(&dpy, key_num);
                    key_pressed = -1;
                }
            }
        }

        ctime = cyg_current_time() * 10;
        IPw_TimeoutProcess(&dpy, ctime - otime);
        otime = ctime;

        cyg_mutex_unlock(&dd_lock);
    }
}

// --------------------------------------------------------------------------
// End of dd_synth.c 
