//==========================================================================
//
//      mini2440_evalfb_init.cxx
//
//      Instantiate a framebuffer device for stm32 TFT LCD.
//      Cloned from synth initialization.
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2008, 2009 Free Software Foundation, Inc.                        
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
// Author(s):     Ricky Wu
//
// Date:          2011-08-17
//
//
//###DESCRIPTIONEND####
//========================================================================

#include <pkgconf/devs_framebuf_mini2440.h>
#include <cyg/io/framebuf.h>

class cyg_mini2440_fb_init {

public:
    cyg_mini2440_fb_init(void)
    {
#ifdef CYGPKG_DEVS_FRAMEBUF_MINI2440_FB0
        cyg_mini2440_fb0_instantiate(&CYG_FB_fb0_STRUCT);
#endif
    }
};

static cyg_mini2440_fb_init cyg_mini2440_fb_init_object CYGBLD_ATTRIB_INIT_PRI(CYG_INIT_DEV_CHAR);
