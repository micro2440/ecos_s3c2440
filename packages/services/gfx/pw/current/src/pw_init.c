/*===========================================================================
//
//        pw_init.c
//
//        PW graphic library init code 
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
 *  Inits internal structures.
 *  @internal
 */ 
void 
IPw_InitInternals(void)
{
    IPW_TRACE_ENTER(IN1);

    IPw_RegionInit();
    IPw_BitmapInit();
 
    IPW_TRACE_EXIT(IN1);
}

/**
 *  Calls the PW application init function.
 *  @param dpy display of application
 *  @return true if the initialization was successfull
 *  @internal
 */ 
Pw_Bool 
IPw_InitComponents(Pw_Display* dpy)
{
    Pw_Bool ret;

    IPW_TRACE_ENTER(IN1);
    IPW_CHECK_PTR(dpy);
    
    ret = dpy->dd->init_components(dpy);

    IPW_TRACE_EXIT_V(IN1, d, ret);
    return(ret);
}

/*---------------------------------------------------------------------------
// end of pw_init.c 
//--------------------------------------------------------------------------- */
