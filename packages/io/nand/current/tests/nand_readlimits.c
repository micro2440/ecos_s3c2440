//=============================================================================
//
//      readlimits.c
//
//      eCos NAND flash: Read and partition limit tests
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
// Date:        2009-04-06
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/infra/testcase.h>
#include <cyg/nand/nand.h>
#include <cyg/nand/nand_devtab.h>
#include <cyg/nand/util.h>
#include <cyg/infra/diag.h>
#include "nand_bbt.h"
#include <stdio.h>

/* Functional check of reading data (not that it's sensible!);
 * quasi-unit-test that partition limits work.
 *
 * Requires a NAND device. The first one found will be used.
 */

/* Have we had an over-limit check? */
int tried_bad = 0, passes = 0, fails = 0;
#define TRYREAD(p,g,s) do {     \
    int _r = tryread(p,g,s);    \
    if (!_r) ++passes;          \
    else {                      \
        ++fails;                \
        CYG_TEST_FAIL("partition limiter check");   \
    }   \
} while(0)

unsigned char buf[CYGNUM_NAND_PAGEBUFFER];

/* Try to read a page.
 * Pass 1 if you expect it to work, 0 if you don't.
 * Returns 0 for test pass (i.e. it worked as well as it was supposed to),
 * -1 for test fail (it didn't). */
int tryread(cyg_nand_partition *prt, cyg_nand_page_addr pg, int shouldwork)
{
#if 0
    printf("Trying %d...\n",pg);
#endif
    int rv = cyg_nandp_read_page(prt, pg, buf, NULL, 0);

    if (!shouldwork) ++tried_bad;
    if (rv==0)
        return shouldwork ? 0 : -1;
    else
        return shouldwork ? -1 : 0;
}

const char msg[] = "NAND partition limit read testing";

int cyg_user_start(void)
{
    cyg_nand_device *dev;
    cyg_nand_partition *prt;
    cyg_nand_page_addr pg;
    CYG_TEST_INIT();
    CYG_TEST_INFO(msg);

    if (cyg_nanddevtab == &cyg_nanddevtab_end)
        CYG_TEST_NA("No NAND devices found");

    diag_printf("Using NAND device %s\n", cyg_nanddevtab->devname);
    CYG_TEST_CHECK(0==cyg_nand_lookup(cyg_nanddevtab->devname, &dev),"lookup failed");

    prt = cyg_nand_get_partition(dev, 0);

    /* Check we can read the first page of the partition, and the page
     * before it (which might be off the start of the device) */
    TRYREAD(prt, 0, 1);
    TRYREAD(prt, -1, 0);

    /* Check we can read the last page of the partition (allowing for
     * the BBT perhaps getting in the way) */
    cyg_nand_block_addr blk = prt->last;
#ifdef CYGSEM_IO_NAND_USE_BBT
    while (cyg_nand_bbti_query(dev, blk) != CYG_NAND_BBT_OK)
        blk--;
#else
    CYG_TEST_INFO("Caution, CYGSEM_IO_NAND_USE_BBT disabled - this may go wrong if we hit a bad block.");
#endif
    cyg_nand_block_addr pblk = blk - prt->first;
    pg = CYG_NAND_BLOCK2PAGEADDR(dev, pblk+1) - 1;
    TRYREAD(prt, pg, 1);

    /* Try and read the page after the last page (which might be off
     * the end of the device) */
    pg = CYG_NAND_PARTITION_NPAGES(prt);
    TRYREAD(prt, pg, 0);

    if (!fails) {
        if (!tried_bad)
            CYG_TEST_NA("No expected failure cases; try again with a different partition setup");

        else
            CYG_TEST_PASS_FINISH(msg);
    } else CYG_TEST_FAIL_FINISH(msg);

}

