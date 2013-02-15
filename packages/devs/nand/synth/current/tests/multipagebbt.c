//=============================================================================
//
//      multipagebbt.c
//
//      Multi-page BBT functional check (using the synth nand device)
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

#include <cyg/infra/testcase.h>
#include <pkgconf/system.h>
#if !defined(CYGPKG_DEVS_NAND_SYNTH) || !defined(CYGPKG_MEMALLOC) || !defined(CYGPKG_LIBC_STDLIB)

externC void
cyg_start( void )
{
    CYG_TEST_INIT();
    CYG_TEST_NA("Only usable with CYGPKG_DEVS_NAND_SYNTH, CYGPKG_MEMALLOC, CYGPKG_LIBC_STDLIB");
}

#else

# include <pkgconf/devs_nand_synth.h>
# if (CYGNUM_NAND_SYNTH_BLOCK_COUNT <= (8*CYGNUM_NAND_SYNTH_PAGESIZE))

externC void
cyg_start( void )
{
    CYG_TEST_INIT();
    CYG_TEST_NA("Requires: CYGNUM_NAND_SYNTH_BLOCK_COUNT > (8*CYGNUM_NAND_SYNTH_PAGESIZE)\n");
}

# else

#include <string.h>
#include <stdlib.h>
#include <cyg/infra/diag.h>
#include <cyg/nand/nand.h>
#include <pkgconf/io_nand.h>

/* Assert-like ... */
#define MUST(what) do { CYG_TEST_CHECK((what), #what); } while(0)

#define NBLOCKS(dev) (1<<(dev)->blockcount_bits)
#define MYBBTSIZE(dev) (1+NBLOCKS(dev)/8)

/* Forbidden knowledge: functions declared in nand_bbt.h which isn't
 * exposed. */
int cyg_nand_bbti_markany(cyg_nand_device *dev, cyg_nand_block_addr blk,
        cyg_nand_bbt_status_t st);

unsigned char tmp[CYGNUM_NAND_PAGEBUFFER];

void read_bbt(cyg_nand_partition *prt, unsigned char *my_bbt)
{
    cyg_nand_device *dev = prt->dev;
    cyg_nand_block_addr blk;
    memset(my_bbt, 0, MYBBTSIZE(dev));
    for (blk=0; blk < NBLOCKS(dev); blk++) {
        int st = cyg_nand_bbt_query(prt, blk);
        if (st != CYG_NAND_BBT_OK)
            my_bbt[blk/8] |= 1<<(blk%8);
    }
}

int cyg_user_start(void)
{
    cyg_nand_device *dev;
    cyg_nand_partition *prt;
    unsigned char *bbt1, *bbt2;
    cyg_nand_block_addr rblk;

    CYG_TEST_INIT();
    MUST(0==cyg_nand_lookup("synth", &dev));
    prt = cyg_nand_get_partition(dev, 0);
    if (!prt)
        CYG_TEST_NA("Need a plausible partition 0 on synth nand");

    bbt1 = malloc(MYBBTSIZE(dev));
    read_bbt(prt, bbt1);

    /* We need a usable block to work with. */
    rblk = prt->last;
    do --rblk;
    while (cyg_nand_bbt_query(prt, rblk) != CYG_NAND_BBT_OK);
    diag_printf("got blk %d / %d\n", rblk, 8*NAND_BYTES_PER_PAGE(dev));
    if (rblk < (8 * NAND_BYTES_PER_PAGE(dev))) {
        CYG_TEST_NA("Requires partition 0 to extend past the first BBT page; e.g. (CYGNUM_DEVS_NAND_SYNTH_PARTITION_0_BASE + CYGNUM_DEVS_NAND_SYNTH_PARTITION_0_SIZE) >= (8*CYGNUM_NAND_SYNTH_PAGESIZE)");
    }

    cyg_nand_page_addr pg = rblk * NAND_PAGES_PER_BLOCK(dev);
    MUST(0==cyg_nand_read_page(prt, pg, tmp, NAND_BYTES_PER_PAGE(dev), 0, 0));

    /* Mark bad, check the table is as expected, ensure we can't read it */
    MUST(0==cyg_nand_bbt_markbad(prt, rblk));

    bbt2 = malloc(MYBBTSIZE(dev));
    read_bbt(prt, bbt2);
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
    read_bbt(prt, bbt2);
    for (i=0; i<MYBBTSIZE(dev); i++)
        MUST(bbt1[i]==bbt2[i]);
    MUST(0==cyg_nand_read_page(prt, pg, tmp, NAND_BYTES_PER_PAGE(dev), 0, 0));

    CYG_TEST_PASS_FINISH("NAND read/BBT functional check with big BBT");
    return 0;
}

# endif
#endif
