//=============================================================================
//
//      erasenand.c
//
//      eCos NAND flash: NAND erase utility.
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
// Date:        2009-07-28
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/nand/nand.h>
#include <cyg/nand/nand_devtab.h>
#include <cyg/nand/util.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <stdio.h>
#include <string.h>

/* NAND device erase.
 * WARNING: This test will WRITE TO and ERASE data on a partition.
 * Do NOT run it on a device containing live data that you care about!
 */

// Set the device and partition to be erased here.
#define DEVICE "onboard"
#define PARTITION 0

void cyg_user_start(void)
{
    cyg_nand_device *dev;
    cyg_nand_partition *prt;
    cyg_nand_block_addr blk;
    int rv;

    cyg_nand_lookup(DEVICE, &dev);
    CYG_ASSERT(0!=dev,"device lookup failed");
    prt = cyg_nand_get_partition(dev, 0);
    CYG_ASSERT(0!=prt, "partition lookup failed");

    diag_printf("Erasing NAND device %s/%d...\n", DEVICE, PARTITION);

    int progmod = (prt->last - prt->first + 1) / 73 + 1;

    for (blk=0; blk < CYG_NAND_PARTITION_NBLOCKS(prt); blk++) {
#ifdef CYGSEM_IO_NAND_USE_BBT
        int st = cyg_nandp_bbt_query(prt, blk);
        if (st<0) {
            diag_printf("Block %d BBTI error %d\n", blk, -st);
        }
#else
        int st = 0;
#endif
        const char *msg = 0;
        switch(st) {
            case CYG_NAND_BBT_OK:
                rv = cyg_nandp_erase_block(prt, blk);
                if (rv != 0)
                    diag_printf("Block %d: error %d\n", blk, -rv);
                break;
            case CYG_NAND_BBT_WORNBAD:
                msg="worn bad"; break;
            case CYG_NAND_BBT_FACTORY_BAD:
                msg="factory bad"; break;

            case CYG_NAND_BBT_RESERVED:
                // Skip quietly
                break;
        }
        if (msg)
            diag_printf("Skipping block %d (%s)\n", blk, msg);

        if (0==(blk % progmod))
            diag_printf(".");
    }
    diag_printf("\nErase complete.\n");
}

