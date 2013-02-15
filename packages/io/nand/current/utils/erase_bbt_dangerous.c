//=============================================================================
//
//      erase_bbt_dangerous.c
//
//      eCos NAND flash: BBT erase utility.
//          This program is DANGEROUS and must NEVER be run unless you
//          are fully aware of and understand the effect it will have.
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2009 eCosCentric Limited.
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
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   wry
// Date:        2009-04-07
//
//####DESCRIPTIONEND####
//=============================================================================

#define DEVICE "onboard"

#include <cyg/nand/nand.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/testcase.h>
#include <stdio.h>
#include <pkgconf/isoinfra.h>
#include CYGBLD_ISO_STRERROR_HEADER

/* Utility code to erase the BBT on a device.
 * This is DANGEROUS, do not run unless you understand the consequences!
 */

#ifdef CYGSEM_IO_NAND_USE_BBT
void rawerase(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    diag_printf("Erasing block %d on %s\n", blk, dev->devname);
    int rv = dev->fns->erase_block(dev, blk);
    if (rv != 0) {
        diag_printf("Error erasing: %s (%d)\n", strerror(-rv), rv);
    }
}

#define MUST(a,b) do { if (!(a)) { diag_printf(b); return; } } while(0)

void cyg_user_start(void)
{
    cyg_nand_device *dev;
    MUST(0==cyg_nand_lookup(DEVICE, &dev),"lookup failed\n");
    if (dev->bbt.primary != 0xFFFFFFFF)
        rawerase(dev,dev->bbt.primary);
    if (dev->bbt.mirror != 0xFFFFFFFF)
    rawerase(dev,dev->bbt.mirror);
    diag_printf("BBT should now be erased.\n");
}

#else
void cyg_user_start(void)
{
    diag_printf("This utility doesn't make sense unless the BBT is enabled.\n");
}
#endif

