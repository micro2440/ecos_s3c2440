/*===========================================================================
//
//        pw_diag.c
//
//        PW graphic library diag code 
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

#include <stdarg.h>
#include <i_pwin.h>

/* -------------------------------------------------------------------------- */

extern int  diag_printf(const char *format, ...);
extern void exit(int status);

/* -------------------------------------------------------------------------- */

void 
IPw_Printf(Pw_Char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    diag_printf(format, ap);
    va_end(ap);
}

void 
IPw_TraceStart(Pw_Char* func, Pw_Char* file, Pw_Int line)
{
    IPw_Printf("TRC<%s(%s:%d)>: ", func, file, line); 
}

void
IPw_TraceEnd(void)
{
    IPw_Printf("\n");
}

void 
IPw_AssertStart(Pw_Char* func, Pw_Char* file, Pw_Int line)
{
    IPw_Printf("ASSERT<%s(%s:%d)>: ", func, file, line); 
}

void 
IPw_AssertEnd(void)
{
    IPw_Printf("\n"); 
}

void 
IPw_NoteStart(Pw_Char* func, Pw_Char* file, Pw_Int line)
{
    IPw_Printf("NOTE: "); 
}

void 
IPw_NoteEnd(void)
{
    IPw_Printf("\n"); 
}

void 
IPw_WarnStart(Pw_Char* func, Pw_Char* file, Pw_Int line)
{
    IPw_Printf("WARN<%s(%s:%d): ", func, file, line); 
}

void 
IPw_WarnEnd(void)
{
    IPw_Printf("\n"); 
}

void 
IPw_FailStart(Pw_Char* func, Pw_Char* file, Pw_Int line)
{
    IPw_Printf("FAIL<%s(%s:%d): ", func, file, line); 
}

void 
IPw_FailEnd(void)
{
    IPw_Printf("\n");
    exit(1);
}

/*---------------------------------------------------------------------------
// end of pw_diag.c 
//--------------------------------------------------------------------------- */
