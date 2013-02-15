/*===========================================================================
//
//        pw_timeout.c 
//
//        PW graphic library timeout code 
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
 *  Checks each display @p dpy timeout and triggers it
 *  if its sleeping time has run out.
 *  @param dpy      pointer to display
 *  @param time_elp time in milliseconds that has ellapsed 
 *                  between this and the previous call 
 *                  to I_PwTimeoutProcess
 *  @internal
 */ 
void 
IPw_TimeoutProcess(Pw_Display* dpy, Pw_uLong time_elp)
{
    Pw_Timeout* to;
    Pw_Timeout* pto;

    IPW_TRACE_ENTER(TM1);
    IPW_CHECK_PTR(dpy);
    IPW_TRACE3M(TM1, p, dpy, p, dpy->timeouts, d, time_elp);

    /*
     *  Go trough the timeouts list and check the timeout's left time. 
     *  If the time has ellapsed trigger the timeout callback fn - if 
     *  it returns FALSE remove timeout from the list - if it returns 
     *  TRUE set its time left variable to its original timeout time.
     *  Else just decrement the timeout's time left variable.
     *
     *  to  - current timeout
     *  pto - previous (processed) timeout
     */

    to  = dpy->timeouts;
    pto = PW_NULL;

    while (PW_NULL != to)
    {
        IPW_TRACE2M(TM1, p, to, d, to->time_left);

        if (to->time_left <= time_elp)
        {
            IPW_TRACE1(TM1, p, to->cb);

            if (!(to->cb)(dpy, to->data))
            {
                if (PW_NULL == pto)
                {
                    dpy->timeouts = to->next;
                    to = dpy->timeouts;
                }
                else
                {
                    pto->next = to->next;
                    to = pto->next;    
                }
            }
            else
            {
                to->time_left = to->time;
                IPW_TRACE1(TM1, d, to->time_left);
                pto = to;
                to = to->next;
            }
        }
        else
        {
            to->time_left -= time_elp;
            IPW_TRACE1(TM1, d, to->time_left);
            pto = to;
            to = to->next;
        }
    }
    IPW_TRACE_EXIT(TM2);
} 

/**
 *  Removes all timeouts of display @p dpy.
 *  @param dpy pointer to display
 *  @internal
 */ 
void 
IPw_TimeoutRemoveAll(Pw_Display* dpy)
{
    IPW_TRACE_ENTER(TM2);
    IPW_CHECK_PTR(dpy);
    IPW_TRACE2(TM1, p, dpy, dpy->timeouts);

    dpy->timeouts = PW_NULL; 

    IPW_TRACE_EXIT(TM2);
} 

/* -------------------------------------------------------------------------- */

/**
 *  Adds a timeout callback to display @p dpy.
 *  @param dpy  pointer to display
 *  @param cb   callback function
 *  @param data client data (with wich the callback function will be called)
 *  @param time time in milliseconds to wait before the callback function is
 *              called 
 *  @param to   pointer to timeout to be created
 *  @return pointer to created timeout (same as @p to)
 */ 
Pw_Timeout*
Pw_TimeoutCreate(Pw_Display*  dpy, 
                 Pw_TimeoutCb cb, 
                 Pw_Pointer   data, 
                 Pw_uLong     time,
                 Pw_Timeout*  to)
{
    IPW_TRACE_ENTER(TM2);
    IPW_CHECK_PTR(dpy);
    IPW_TRACE5(TM1, p, dpy, dpy->timeouts, cb, data, to);
    IPW_TRACE1(TM1, d, time);

    /*
     *  Initialize the new timeout fields and
     *  insert it into the display timeouts list.
     */

    to->cb        = cb;
    to->data      = data;
    to->time      = time;
    to->time_left = time;
    to->next      = PW_NULL;

    if (PW_NULL == dpy->timeouts)
    {
        dpy->timeouts = to;
    }
    else
    {
        Pw_Timeout* tto = dpy->timeouts;

        IPW_CHECK_COND(to != dpy->timeouts);
      
        while (PW_NULL != tto->next)
        {
            IPW_TRACE6M(TM1, p, tto, p, tto->cb, p, tto->data, 
                        d, tto->time, d, tto->time_left, p, tto->next);
            IPW_CHECK_COND(to != tto);
            tto = tto->next;
        }
        
        tto->next = to;
    }
    IPW_TRACE_EXIT_V(TM2, p, to);
    return(to);
} 

/**
 *  Removes the timeout @p to from the display @p dpy.
 *  NOTE: this function should not be used inside the timeout callback!
 *  @param dpy  pointer to display
 *  @param to   pointer to timeout 
 */ 
void
Pw_TimeoutDestroy(Pw_Display* dpy, Pw_Timeout* to)
{
    Pw_Timeout* tto;
    Pw_Timeout* pto;

    IPW_TRACE_ENTER(TM2);
    IPW_CHECK_PTR2(dpy, to);
    IPW_TRACE3(TM1, p, dpy, dpy->timeouts, to);
    IPW_TRACE2(TM1, d, to->time, to->time_left);

    tto = dpy->timeouts;
    pto = PW_NULL;

    while (PW_NULL != tto && tto != to)
    {
        pto = tto;
        tto = tto->next;
    }

    IPW_CHECK_PTR(tto);

    if (PW_NULL != tto)
    {
        if (PW_NULL == pto)
            dpy->timeouts = to->next;
        else
            pto->next = to->next; 
    }

    IPW_TRACE_EXIT(TM2);
}

/*---------------------------------------------------------------------------
// end of pw_timeout.c 
//--------------------------------------------------------------------------- */
