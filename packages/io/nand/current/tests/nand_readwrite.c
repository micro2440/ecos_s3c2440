//=============================================================================
//
//      readwrite.c
//
//      eCos NAND flash: Read/write tests
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

#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/testcase.h>
#include <cyg/nand/nand.h>
#include <cyg/nand/nand_devtab.h>
#include <cyg/nand/util.h>
#include <cyg/infra/diag.h>
#include <stdio.h>
#include <string.h>

/* Functional test of reading, writing and erasing data.
 * Requires a NAND device. The first one found will be used.
 * This test is a bit stack-greedy, so don't expect it to work in a 
 * minimal configuration.
 *
 * WARNING: This test will WRITE TO and ERASE data on a partition.
 * Do NOT run it on a device containing live data that you care about!
 */

#define MUST(what) do { CYG_TEST_CHECK((what), #what); } while(0)

#include "ar4prng.inl"

const char msg[] = "NAND read/write testing";

#if defined(CYGSEM_IO_NAND_READONLY)
externC void
cyg_start( void )
{
    CYG_TEST_INIT();
    CYG_TEST_INFO(msg);
    CYG_TEST_NA("Not useful if CYGSEM_IO_NAND_READONLY");
}

#else

/* we'll use our text segment as seed to the prng; not crypto-secure but
 * pretty good for running tests */
extern unsigned char _stext[], _etext[];

#define datasize CYGNUM_NAND_PAGEBUFFER
unsigned char buf[datasize], buf2[datasize], buf3[datasize];
ar4ctx rnd;

int cyg_user_start(void)
{
    cyg_nand_device *dev;
    cyg_nand_partition *prt;
    cyg_nand_block_addr blk;
    cyg_nand_page_addr pg;
    int i;

    CYG_TEST_INIT();
    CYG_TEST_INFO(msg);
    ar4prng_init(&rnd,_stext, _etext-_stext);

    if (cyg_nanddevtab == &cyg_nanddevtab_end)
        CYG_TEST_NA("No NAND devices found");

    CYG_TEST_INFO("readwrite NAND test: using device: ");
    CYG_TEST_INFO(cyg_nanddevtab->devname);
    CYG_TEST_CHECK(0==cyg_nand_lookup(cyg_nanddevtab->devname, &dev),"lookup failed");

    prt = cyg_nand_get_partition(dev, 0);

    /* Now select a usable block in the partition to base ourselves around. */
    blk = 0;
#ifdef CYGSEM_IO_NAND_USE_BBT
    while ( (cyg_nandp_bbt_query(prt, blk) != CYG_NAND_BBT_OK)
            || (cyg_nandp_bbt_query(prt, blk+1) != CYG_NAND_BBT_OK) ) {
        blk++;
        if (blk > CYG_NAND_PARTITION_NBLOCKS(prt))
            CYG_TEST_FAIL_FINISH("Cannot find a usable block to test");
    }
#else
    CYG_TEST_INFO("Caution, CYGSEM_IO_NAND_USE_BBT disabled - this may go wrong if we hit a bad block.");
#endif
    pg = CYG_NAND_BLOCK2PAGEADDR(dev,blk);

    diag_printf("Erasing block %d\n", blk);
    MUST(0==cyg_nandp_erase_block(prt, blk));

    CYG_ASSERTC(CYG_NAND_BYTES_PER_PAGE(dev) <= CYGNUM_NAND_PAGEBUFFER);
    ar4prng_many(&rnd, buf, datasize);

    diag_printf("Read/write to page %d (block %d)\n", pg, blk);
    MUST(0==cyg_nandp_write_page(prt, pg, buf, buf, CYG_NAND_APPSPARE_PER_PAGE(dev)));
    MUST(0==cyg_nandp_read_page (prt, pg, buf2, 0, 0));
    MUST(0==memcmp(buf, buf2, datasize));
    MUST(0==cyg_nandp_read_page (prt, pg, 0, buf3, CYG_NAND_APPSPARE_PER_PAGE(dev)));
    MUST(0==memcmp(buf, buf2, datasize));
    memset(buf2, 0, sizeof buf2);
    MUST(0==cyg_nandp_read_page (prt, pg, buf2, buf3, CYG_NAND_APPSPARE_PER_PAGE(dev)));
    MUST(0==memcmp(buf, buf2, datasize));
    memset(buf3, 0, sizeof buf3);

    diag_printf("Partial-reads of page %d (block %d)\n", pg, blk);
#define TEST(_m,_n,_ecc) do {       \
    memset(buf2, 0, sizeof buf2);   \
    MUST(0==cyg_nandp_read_part_page(prt, pg, buf2, _m, _n, _ecc));  \
    MUST(0==memcmp(&buf[_m], buf2, _n));    \
} while(0)
#define TESTFAIL(_m,_n,_ecc) do {   \
    MUST(0!=cyg_nandp_read_part_page(prt, pg, buf2, _m, _n, _ecc)); \
} while(0)
    // Ordinary whole-page reads:
    TEST(0, CYG_NAND_BYTES_PER_PAGE(dev), 0);
    TEST(0, CYG_NAND_BYTES_PER_PAGE(dev), 1);
    // Partial reads:
    TEST(0, 99, 0);
    TEST(0, 99, 1);
    TEST(42, 99, 0);
    TEST(42, 99, 1);
    // make sure we can address the upper half of small-page devices:
    TEST(258, 99, 1);
    TEST(258, 99, 0);
    // Read right up to the end:
    TEST(43, CYG_NAND_BYTES_PER_PAGE(dev)-43, 0);
    TEST(43, CYG_NAND_BYTES_PER_PAGE(dev)-43, 1);
    TEST(258, CYG_NAND_BYTES_PER_PAGE(dev)-258, 0);
    TEST(258, CYG_NAND_BYTES_PER_PAGE(dev)-258, 1);
    // ... but we should not be able to read off the end of the page:
    TESTFAIL(0, CYG_NAND_BYTES_PER_PAGE(dev)+1, 0);
    TESTFAIL(43, CYG_NAND_BYTES_PER_PAGE(dev)-42, 1);
    // Zero-length reads should work:
    TEST(0, 0, 0);
    TEST(0, 0, 1);
    // Silly offsets should fail:
    TESTFAIL(CYG_NAND_BYTES_PER_PAGE(dev)+1, 1, 0);
    TESTFAIL(CYG_NAND_BYTES_PER_PAGE(dev)+1, 0, 1);

    diag_printf("Erasing adjacent block %d\n", blk+1);
    MUST(0==cyg_nandp_erase_block(prt, blk+1));
    diag_printf("Re-read check..\n");
    MUST(0==cyg_nandp_read_page (prt, pg, buf2, 0, 0));
    MUST(0==memcmp(buf, buf2, datasize));

    diag_printf("Re-erase %d\n", blk);
    MUST(0==cyg_nandp_erase_block(prt, blk));
    diag_printf("Read-back page %d to confirm erase\n", pg);
    MUST(0==cyg_nandp_read_page (prt, pg, buf2, 0, 0));
    for (i=0; i<datasize; i++)
        MUST(buf2[i]==0xff);

    CYG_TEST_PASS_FINISH(msg);
}

#endif

