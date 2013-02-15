/*===========================================================================
//
//        pw_display.c 
//
//        PW graphic library display code 
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

#include <i_pwin.h>

/* -------------------------------------------------------------------------- */

/** White pixel definition. */
static Pw_Color _white_pixel = {211, 239, 194};
/** Black pixel definition. */
static Pw_Color _black_pixel = {  0,   0,  50};

/** Pointer to white pixel. */
Pw_Color* pw_white_pixel = (Pw_Color*)&_white_pixel;
/** Pointer to black pixel. */
Pw_Color* pw_black_pixel = (Pw_Color*)&_black_pixel;

/* -------------------------------------------------------------------------- */

/**
 *  Opens a new display.
 *  @param width  display width
 *  @param height display height
 *  @param dd     pointer to display specific functions
 *  @param dpy    pointer to display to open
 *  @return pointer to newly opened display (same as @p dpy)
 *  @internal
 */ 
Pw_Display* 
IPw_DisplayOpen(Pw_Coord width, Pw_Coord height, Pw_DD* dd, Pw_Display* dpy)
{
    Pw_uInt i;

    IPW_TRACE_ENTER(DP2);
    IPW_CHECK_PTR2(dd, dpy);
    IPW_TRACE3M(DP1, p, dpy, d, width, d, height);
   
    /*
     * initialize display's fields and
     * create its root window with the 
     * width and height of the display
     */
 
    dpy->width  = width; 
    dpy->height = height; 
    dpy->dd     = dd;

    IPw_RootWindowCreate(dpy, width, height);

    dpy->focus_win = PW_NULL;
    dpy->timeouts  = PW_NULL;

    for (i = 0; i < PW_XINPUTS_MAX_NUM; i++)
        dpy->xinput_win[i] = PW_NULL;

    dpy->close = FALSE;

    IPW_TRACE_EXIT_V(DP2, p, dpy);
    return(dpy);
}

/**
 *  Closes the display @p dpy.
 *  @param dpy pointer to display to close
 *  @internal
 */ 
void 
IPw_DisplayClose(Pw_Display* dpy)
{
    IPW_TRACE_ENTER(DP2);
    IPW_CHECK_PTR(dpy);
    IPW_TRACE1(DP1, p, dpy);
 
    /*
     * remove display's timeouts and
     * destroy its root window
     */

    IPw_TimeoutRemoveAll(dpy);
    Pw_WindowDestroy(&dpy->root_win);

    IPW_TRACE_EXIT(DP2);
}

/**
 *  Refreshes the display @p dpy.
 *  Repaint all visible windows of display @p dpy.
 *  @param dpy pointer to display to refresh
 *  @internal
 */ 
void 
IPw_DisplayRefresh(Pw_Display* dpy)
{
    IPW_TRACE_ENTER(DP2);
    IPW_CHECK_PTR(dpy);
    IPW_TRACE1(DP1, p, dpy);
 
    Pw_WindowRepaint(&dpy->root_win);

    IPW_TRACE_EXIT(DP2);
} 

/* -------------------------------------------------------------------------- */

/**
 *  Gets the xinput number @p num value of display @p dpy.
 *  @param dpy pointer to display 
 *  @param num xinput number
 *  @return value of xinput number @p num
 */ 
Pw_Int 
Pw_DisplayGetXInputValue(Pw_Display* dpy, Pw_uInt num)
{
    Pw_Int val;

    IPW_TRACE_ENTER(DP2);
    IPW_CHECK_PTR(dpy);
    IPW_TRACE1(DP1, p, dpy);
 
    val = dpy->dd->get_xinput_value(dpy, num);

    IPW_TRACE_EXIT_V(DP2, d, val);
    return(val);
} 

/*---------------------------------------------------------------------------
// end of pw_display.c 
//--------------------------------------------------------------------------- */
