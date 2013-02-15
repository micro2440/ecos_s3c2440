//=============================================================================
//
//      bbt.c
//
//      Simple tests of the Bad Block Table layer
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
#include <pkgconf/system.h>
#if !defined(CYGPKG_MEMALLOC) || !defined(CYGPKG_LIBC_STDLIB)
externC void
cyg_start( void )
{
    CYG_TEST_INIT();
    CYG_TEST_NA("Only usable with CYGPKG_MEMALLOC and CYGPKG_LIBC_STDLIB");
}

#else

#include <string.h>
#include <stdlib.h>
#include <cyg/nand/nand.h>
#include <cyg/nand/nand_devtab.h>
#include <cyg/infra/diag.h>

/* Forbidden knowledge: internal BBT functions */
int cyg_nand_bbti_query(cyg_nand_device *dev, cyg_nand_block_addr blk);
int cyg_nand_bbti_markany(cyg_nand_device *dev, cyg_nand_block_addr blk,
                                  cyg_nand_bbt_status_t st);

/* Assert-like ... */
#define MUST(what) do { CYG_TEST_CHECK((what), #what); } while(0)

#define NBLOCKS(dev) (1<<(dev)->blockcount_bits)
#define MYBBTSIZE(dev) (1+NBLOCKS(dev)/8)

void read_bbt(cyg_nand_device *dev, unsigned char *my_bbt)
{
    cyg_nand_block_addr blk;
    memset(my_bbt, 0, MYBBTSIZE(dev));
    for (blk=0; blk < NBLOCKS(dev); blk++) {
        int st = cyg_nand_bbti_query(dev, blk);
        if (st != CYG_NAND_BBT_OK)
            my_bbt[blk/8] |= 1<<(blk%8);
    }
}

unsigned char tmp[CYGNUM_NAND_PAGEBUFFER];

int cyg_user_start(void)
{
    cyg_nand_device *dev;
    cyg_nand_partition *prt;
    unsigned char *bbt1, *bbt2;
    cyg_nand_block_addr rblk;

    CYG_TEST_INIT();
    diag_printf("Using NAND device %s\n", cyg_nanddevtab[0].devname);
    MUST(0==cyg_nand_lookup(cyg_nanddevtab[0].devname, &dev));
    prt = cyg_nand_get_partition(dev, 0);
    if (!prt)
        CYG_TEST_NA("Need a plausible partition 0 on first nand device");

    bbt1 = malloc(MYBBTSIZE(dev));
    read_bbt(dev, bbt1);

    /* We need a usable block to work with. */
    int lim = prt->last - prt->first;
    if (lim < 5)
        CYG_TEST_NA("nand partition 0 isn't big enough");

    do {
        rblk = (rand() % lim) + prt->first;
    } while (cyg_nand_bbti_query(dev, rblk) != CYG_NAND_BBT_OK);

    cyg_nand_page_addr pg = rblk * NAND_PAGES_PER_BLOCK(dev);
    MUST(0==cyg_nand_read_page(prt, pg, tmp, NAND_BYTES_PER_PAGE(dev), 0, 0));

    /* Mark bad, check the table is as expected, ensure we can't read it */
    //MUST(0==cyg_nand_bbti_markbad(dev, rblk));
    //MUST(0==cyg_nand_bbt_markbad(prt, rblk));
    MUST(0==cyg_nand_bbt_markbad_pageaddr(prt, pg));

    bbt2 = malloc(MYBBTSIZE(dev));
    read_bbt(dev, bbt2);
    /* We expect it to match, save for one bit... */
    int differer = rblk/8;
    int i;
    for (i=0; i<MYBBTSIZE(dev); i++) {
        if (i==differer) {
            MUST(bbt1[i]!=bbt2[i]);
            MUST( ( bbt1[i] | 1<<(rblk%8) ) == bbt2[i] );
        } else {
            MUST(bbt1[i]==bbt2[i]);
        }
    }
    MUST(0!=cyg_nand_read_page(prt, pg, tmp, NAND_BYTES_PER_PAGE(dev), 0, 0));

    /* Now revert */
    MUST(0==cyg_nand_bbti_markany(dev, rblk, CYG_NAND_BBT_OK));
    read_bbt(dev, bbt2);
    for (i=0; i<MYBBTSIZE(dev); i++)
        MUST(bbt1[i]==bbt2[i]);
    MUST(0==cyg_nand_read_page(prt, pg, tmp, NAND_BYTES_PER_PAGE(dev), 0, 0));

    CYG_TEST_PASS_FINISH("NAND read/BBT functional check");
    return 0;
}

#endif
