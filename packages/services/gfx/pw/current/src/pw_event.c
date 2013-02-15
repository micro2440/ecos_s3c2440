/*===========================================================================
//
//        pw_event.c
//
//        PW graphic library event code 
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

/**
 *  Sends the key pressed event to window @p win or to one of its parents in
 *  case window @p win does't consume it.
 *  @param win     pointer to target window
 *  @param keycode code of the pressed key
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventSendKeyPress(Pw_Window* win, Pw_uInt keycode)
{
    Pw_Event   event;
    Pw_Window* twin;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(win);

    twin = win;
    event.data.type   = Pw_KeyPressEventType;
    event.key.keycode = keycode;
 
    while (PW_NULL != twin)
    {
        IPW_TRACE3M(EV1, p, twin, p, twin->event_cb, d, keycode);

        event.data.win = twin;

        if (PW_NULL != twin->event_cb)
        {
            if (twin->event_cb(&event))
            {
                IPW_TRACE_EXIT_V(EV2, d, TRUE);
                return(TRUE);
            }
        }
        twin = twin->parent;
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
} 

/**
 *  Sends the key released event to window @p win or to one of its parents in
 *  case window @p win does't consume it.
 *  @param win     pointer to target window
 *  @param keycode code of the released key
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventSendKeyRelease(Pw_Window* win, Pw_uInt keycode)
{
    Pw_Event   event;
    Pw_Window* twin;
    
    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(win);

    twin = win;
    event.data.type   = Pw_KeyReleaseEventType;
    event.key.keycode = keycode;
 
    while (PW_NULL != twin)
    {
        IPW_TRACE3M(EV1, p, twin, p, twin->event_cb, d, keycode);

        event.data.win = twin;

        if (PW_NULL != twin->event_cb)
        {
            if (twin->event_cb(&event))
            {
                IPW_TRACE_EXIT_V(EV2, d, TRUE);
                return(TRUE);
            }
        }
        twin = twin->parent;
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
} 

/**
 *  Sends the xinput event to window @p win.
 *  @param win pointer to target window
 *  @param num number of xinput 
 *  @param val value of xinput 
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventSendXInput(Pw_Window* win, Pw_uInt num, Pw_Int val)
{
    Pw_Event event;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(win);
    IPW_TRACE4M(EV1, p, win, p, win->event_cb, d, num, d, val);

    event.data.type   = Pw_XInputEventType;
    event.xinput.num  = num;
    event.xinput.val  = val;
    event.data.win    = win;

    if (PW_NULL != win->event_cb)
    {
        if (win->event_cb(&event))
        {
            IPW_TRACE_EXIT_V(EV2, d, TRUE);
            return(TRUE);
        }
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
} 

/**
 *  Sends the focus in (gained) event to window @p win.
 *  @param win pointer to target window
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventSendFocusIn(Pw_Window* win)
{
    Pw_Event event;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2(EV1, p, win, win->event_cb);
 
    event.data.type = Pw_FocusInEventType;
    event.data.win  = win;

    if (PW_NULL != win->event_cb)
    {
        if (win->event_cb(&event))
        {
            IPW_TRACE_EXIT_V(EV2, d, TRUE);
            return(TRUE);
        }
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
}

/**
 *  Sends the focus out (lost) event to window @p win.
 *  @param win pointer to target window
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventSendFocusOut(Pw_Window* win)
{
    Pw_Event event;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2(EV1, p, win, win->event_cb);
 
    event.data.type = Pw_FocusOutEventType;
    event.data.win  = win;

    if (PW_NULL != win->event_cb)
    {
        if (win->event_cb(&event))
        {
            IPW_TRACE_EXIT_V(EV2, d, TRUE);
            return(TRUE);
        }
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
} 

/**
 *  Sends the window mapped (visible) event to window @p win.
 *  @param win pointer to target window
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventSendMapped(Pw_Window* win)
{
    Pw_Event event;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2(EV1, p, win, win->event_cb);
 
    event.data.type = Pw_MapEventType;
    event.data.win  = win;

    if (PW_NULL != win->event_cb)
    {
        if (win->event_cb(&event))
        {
            IPW_TRACE_EXIT_V(EV2, d, TRUE);
            return(TRUE);
        }
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
}

/**
 *  Sends the window unmapped (invisible) event to window @p win.
 *  @param win pointer to target window
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventSendUnmapped(Pw_Window* win)
{
    Pw_Event event;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2(EV1, p, win, win->event_cb);
 
    event.data.type = Pw_UnmapEventType;
    event.data.win  = win;

    if (PW_NULL != win->event_cb)
    {
        if (win->event_cb(&event))
        {
            IPW_TRACE_EXIT_V(EV2, d, TRUE);
            return(TRUE);
        }
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
}

/**
 *  Sends the window reshaped event to window @p win.
 *  @param win pointer to target window
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventSendReshaped(Pw_Window* win)
{
    Pw_Event event;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2(EV1, p, win, win->event_cb);
 
    event.data.type = Pw_ReshapeEventType;
    event.data.win  = win;

    if (PW_NULL != win->event_cb)
    {
        if (win->event_cb(&event))
        {
            IPW_TRACE_EXIT_V(EV2, d, TRUE);
            return(TRUE);
        }
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
}

/**
 *  Sends the window destroy event to window @p win.
 *  @param win pointer to target window
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventSendDestroy(Pw_Window* win)
{
    Pw_Event event;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2(EV1, p, win, win->event_cb);
 
    event.data.type = Pw_DestroyEventType;
    event.data.win  = win;

    if (PW_NULL != win->event_cb)
    {
        if (win->event_cb(&event))
        {
            IPW_TRACE_EXIT_V(EV2, d, TRUE);
            return(TRUE);
        }
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
} 

/**
 *  Sends the repaint event to window @p win.
 *  @param win pointer to target window
 *  @param gc pointer to graphic context of window @p win
 *  @return true if the event was successfully send
 *  @internal
 */  
Pw_Bool 
IPw_EventSendRepaint(Pw_Window* win, Pw_GC* gc)
{
    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR2(win, gc);
    IPW_TRACE3(EV1, p, win, gc, win->repaint_cb);
 
    if (PW_NULL != win->repaint_cb)
    {
        win->repaint_cb(gc);

        IPW_TRACE_EXIT_V(EV2, d, TRUE);
        return(TRUE);
    }
    IPW_TRACE_EXIT_V(EV2, d, FALSE);
    return(FALSE);
} 

/**
 *  Sends the key pressed event to the window in focus.
 *  @param dpy     pointer to display 
 *  @param keycode code of the pressed key
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventKeyPressed(Pw_Display* dpy, Pw_uInt keycode)
{
    Pw_Bool ret;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(dpy);
    IPW_TRACE2(EV1, p, dpy, dpy->focus_win);
 
    if (PW_NULL == dpy->focus_win)
    {
        IPW_TRACE_EXIT_V(EV2, d, FALSE);
        return(FALSE);
    }

    ret = IPw_EventSendKeyPress(dpy->focus_win, keycode);

    IPW_TRACE_EXIT_V(EV2, d, ret);
    return(ret);
}

/**
 *  Sends the key released event to the window in focus.
 *  @param dpy     pointer to display 
 *  @param keycode code of the released key
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventKeyReleased(Pw_Display* dpy, Pw_uInt keycode)
{
    Pw_Bool ret;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(dpy);
    IPW_TRACE2(EV1, p, dpy, dpy->focus_win);
 
    if (PW_NULL == dpy->focus_win)
    {
        IPW_TRACE_EXIT_V(EV2, d, FALSE);
        return(FALSE);
    }

    ret = IPw_EventSendKeyRelease(dpy->focus_win, keycode);

    IPW_TRACE_EXIT_V(EV2, d, ret);
    return(ret);
} 

/**
 *  Sends the xinput event to listening window (if any).
 *  @param dpy pointer to display 
 *  @param num xinput number 
 *  @param val xinput value 
 *  @return true if the event was successfully send
 *  @internal
 */ 
Pw_Bool 
IPw_EventXInputChanged(Pw_Display* dpy, Pw_uInt num, Pw_Int val)
{
    Pw_Bool ret;

    IPW_TRACE_ENTER(EV2);
    IPW_CHECK_PTR(dpy);
    IPW_TRACE3M(EV1, p, dpy, d, num, d, val);
 
    IPW_FAIL(num < PW_XINPUTS_MAX_NUM, ("XInput out of range (%d)!", num));
    
    IPW_TRACE1(EV1, p, dpy->xinput_win[num]);

    if (PW_NULL == dpy->xinput_win[num])
    {
        IPW_TRACE_EXIT_V(EV2, d, FALSE);
        return(FALSE);
    }

    ret = IPw_EventSendXInput(dpy->xinput_win[num], num, val);

    IPW_TRACE_EXIT_V(EV2, d, ret);
    return(ret);
} 

/*---------------------------------------------------------------------------
// end of pw_event.c
//--------------------------------------------------------------------------- */
