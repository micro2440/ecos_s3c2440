/*===========================================================================
//
//        pw_test.c 
//
//        PW graphic library simple test 
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

#include <stdio.h>

#include <pwin.h>
#include "F9x15.pwf"
#include "slkscr.pwf"
#include "smile.pwb"

/* -------------------------------------------------------------------------- */

static Pw_Window  win1, win2, win3, win4;
static Pw_Timeout to1, to2, to3;
static Pw_Int     to1_time = 0;
static Pw_Int     to2_time = 0;
static Pw_Int     to3_time = 0;

static Pw_Int  msg_lidx[32];
static Pw_Char msg[] = 
    "This demo shows\n"
    "some of PW\n" 
    "windowing and drawing\n"
    "capabilities\n"
    "\n\n"
    "Try to move the\n"
    "window with the smiley\n" 
    "inside using arrow\n"
    "keys. You can also\n" 
    "bring it in front\n"
    "and put it to back\n"
    "with page up and\n"
    "down keys.\n";
   
/* -------------------------------------------------------------------------- */

static void
LineizeString(Pw_Char* str, Pw_Int* lidx)
{
    Pw_Int i = 0, j = 0;
    
    lidx[i++] = 0;
    while (*str != '\0')
    {
        if (*str == '\n')
        {
            lidx[i++] = j+1;
            *str = '\0';
        }   
        str++;
        j++;    
    }
    lidx[i] = -1;
}
    
static void
RootRepaint(Pw_GC* gc)
{
    Pw_Window* win;
    Pw_Coord w, h;
    
    win = Pw_GCGetWindow(gc);

    Pw_WindowGetSize(win, &w, &h);

    Pw_GCSetColor(gc, pw_white_pixel);
    Pw_FillRect(gc, 0, 0, w, h);
}

static void
Win1Repaint(Pw_GC* gc)
{
    Pw_Window* win;
    Pw_Coord   w, h, w2, h2, ew;

    win = Pw_GCGetWindow(gc);

    Pw_WindowGetSize(win, &w, &h);
    
    w2 = (w>>1);
    h2 = (h>>1);
    
    Pw_GCSetColor(gc, pw_white_pixel);
    Pw_FillRect(gc, 0, 0, w, h);
    
    ew = PW_ABS(w2-4 - (to2_time%(2*(w2-4))));
    if (0 == ew) ew = 1;

    Pw_GCSetColor(gc, pw_black_pixel);
    Pw_DrawBitmap(gc, w2-4, h2-4, smile_pwb);
    Pw_DrawRect(gc, 0, 0, w, h);
}

static void
Win2Repaint(Pw_GC* gc)
{
    Pw_Window* win;
    Pw_Coord w, h;
    
    win = Pw_GCGetWindow(gc);

    Pw_WindowGetSize(win, &w, &h);

    Pw_GCSetColor(gc, pw_white_pixel);
    Pw_FillRect(gc, 0, 0, w, h);
    
    Pw_GCSetColor(gc, pw_black_pixel);
    Pw_DrawRect(gc, 0, 0, w, h);
}

static void
Win3Repaint(Pw_GC* gc)
{
    Pw_Window* win;
    Pw_Coord w, h, sx, sy;
    Pw_Int i;
    
    win = Pw_GCGetWindow(gc);

    Pw_WindowGetSize(win, &w, &h);

    Pw_GCSetColor(gc, pw_white_pixel);
    Pw_FillRect(gc, 0, 0, w, h);
    
    Pw_GCSetColor(gc, pw_black_pixel);
    Pw_GCSetFont(gc, slkscr_pwf);

    i = 0;
    sy = Pw_GCGetFontAscent(gc)+h-to3_time;
    while (msg_lidx[i] >= 0)
    {
        sx = (w>>1) - (Pw_GCGetStringWidth(gc, &msg[msg_lidx[i]])>>1);
        Pw_DrawString(gc, sx, sy, &msg[msg_lidx[i]]);
        i++;
        sy += Pw_GCGetFontHeight(gc);
    }
    if (sy < Pw_GCGetFontHeight(gc)) to3_time = 0;
    Pw_DrawRect(gc, 0, 0, w, h);
}

static void
Win4Repaint(Pw_GC* gc)
{
    Pw_Window* win;
    Pw_Coord   w, h, sw;
    Pw_Char    buf[32];
    
    win = Pw_GCGetWindow(gc);

    Pw_WindowGetSize(win, &w, &h);

    Pw_GCSetColor(gc, pw_white_pixel);
    Pw_FillRect(gc, 0, 0, w, h);
    
    Pw_GCSetColor(gc, pw_black_pixel);

    Pw_GCSetFont(gc, F9x15_pwf);
    sprintf(buf, "%d", to1_time);
    sw = Pw_GCGetStringWidth(gc, buf);
    Pw_DrawString(gc, (w >> 1) - (sw >> 1), (h >> 1) + 
                  (Pw_GCGetFontAscent(gc) >> 1), buf);
    Pw_DrawRect(gc, 0, 0, w, h);
}

static Pw_Bool
Win1Events(Pw_Event* ev)
{
    Pw_Window* win;
    Pw_Coord x, y;
    
    win = Pw_EventGetWindow(ev);

    Pw_WindowGetPosition(win, &x, &y);

    switch (Pw_EventGetType(ev))
    {
        case Pw_KeyPressEventType:
        {
            Pw_KeyEvent* kev = Pw_EventCastToKeyEvent(ev);

            switch (Pw_KeyEventGetKeyCode(kev))
            {
                case PW_UP_KEY_CODE:
                    Pw_WindowSetPosition(win, x, y-1);
                    break;
                case PW_DOWN_KEY_CODE:
                    Pw_WindowSetPosition(win, x, y+1);
                    break;
                case PW_LEFT_KEY_CODE:
                    Pw_WindowSetPosition(win, x-1, y);
                    break;
                case PW_RIGHT_KEY_CODE:
                    Pw_WindowSetPosition(win, x+1, y);
                    break;
                case PW_NEXT_KEY_CODE:
                    Pw_WindowLowerOne(win);
                    break;
                case PW_PREV_KEY_CODE:
                    Pw_WindowRaiseOne(win);
                    break;
                default:
                    break;
            }
            break; 
        }  
        default:
            break; 
    }

    return TRUE;
}

static Pw_Bool
To1Cb(Pw_Display* dpy, Pw_Pointer data)
{
    Pw_Window* win = (Pw_Window*)data;

    to1_time++;

    Pw_WindowRepaint(win);
    return(TRUE);
}

static Pw_Bool
To2Cb(Pw_Display* dpy, Pw_Pointer data)
{
    Pw_Window* win = (Pw_Window*)data;

    to2_time++;

    Pw_WindowRepaint(win);
    return(TRUE);
}

static Pw_Bool
To3Cb(Pw_Display* dpy, Pw_Pointer data)
{
    Pw_Window* win = (Pw_Window*)data;

    to3_time++;

    Pw_WindowRepaint(win);
    return(TRUE);
}

/* -------------------------------------------------------------------------- */

Pw_Bool 
Pw_Init(Pw_Display* dpy)
{
    Pw_Window* root_win = Pw_DisplayGetRootWindow(dpy);

    LineizeString(msg, msg_lidx);
        
    Pw_WindowCreate(root_win, 12, 12, 25, 35, &win1);
    Pw_WindowCreate(root_win, 50, 2, 30, 40, &win2);
    Pw_WindowCreate(root_win, 5, 38, 118, 24, &win3);
    Pw_WindowCreate(root_win, 60, 4, 40, 25, &win4);

    Pw_WindowSetRepaintCallback(root_win, RootRepaint);
    Pw_WindowSetRepaintCallback(&win1, Win1Repaint);
    Pw_WindowSetRepaintCallback(&win2, Win2Repaint);
    Pw_WindowSetRepaintCallback(&win3, Win3Repaint);
    Pw_WindowSetRepaintCallback(&win4, Win4Repaint);

    Pw_WindowSetEventCallback(&win1, Win1Events);
    
    Pw_WindowMap(&win1);
    Pw_WindowMap(&win2);
    Pw_WindowMap(&win3);
    Pw_WindowMap(&win4);
    
    Pw_WindowSetFocusAccept(&win1, TRUE);
    Pw_WindowSetFocus(&win1);
    Pw_WindowPlaceOnTop(&win1);

    Pw_TimeoutCreate(dpy, To1Cb, (Pw_Pointer)&win4, 200, &to1);
    Pw_TimeoutCreate(dpy, To2Cb, (Pw_Pointer)&win1, 20, &to2);
    Pw_TimeoutCreate(dpy, To3Cb, (Pw_Pointer)&win3, 100, &to3);
    
    return(TRUE);
}

/* -------------------------------------------------------------------------- */

#include <cyg/hal/hal_arch.h>
#include <cyg/kernel/kapi.h>

extern bool pw_dd_init(void);
extern void pw_dd_main_loop(void);

#define PW_STACKSIZE CYGNUM_HAL_STACK_SIZE_TYPICAL

static cyg_thread    pw_thread_s;
static cyg_handle_t  pw_thread_h;
static unsigned char pw_stack[PW_STACKSIZE];

static void
pw_thread(cyg_addrword_t data)
{
    pw_dd_init();
    pw_dd_main_loop();
}

void
cyg_user_start(void)
{
    cyg_thread_create(4,
                      pw_thread,
                      (cyg_addrword_t) 0,
                      "PW thread",
                      (void *) pw_stack,
                      PW_STACKSIZE,
                      &pw_thread_h,
                      &pw_thread_s);

    cyg_thread_resume(pw_thread_h);
}

/*---------------------------------------------------------------------------
// end of pw_test.c 
//--------------------------------------------------------------------------- */
