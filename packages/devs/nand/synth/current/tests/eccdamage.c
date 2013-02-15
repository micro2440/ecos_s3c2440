//=============================================================================
//
//      eccdamage.c
//
//      eCos NAND flash: ECC random-loss checks (on the synth device)
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

#ifdef CYGPKG_DEVS_NAND_SYNTH
#include <pkgconf/devs_nand_synth.h>
#endif

#if !defined(CYGPKG_DEVS_NAND_SYNTH) || !defined(CYGSEM_NAND_SYNTH_RANDOMLY_LOSE)

externC void cyg_start( void )
{

    CYG_TEST_INIT();
    CYG_TEST_NA("Requires CYGPKG_DEVS_NAND_SYNTH && CYGSEM_NAND_SYNTH_RANDOMLY_LOSE)");
}

#else


#include <cyg/infra/testcase.h>
#include <cyg/nand/nand.h>
#include <cyg/nand/util.h>
#include <cyg/infra/diag.h>
#include <stdio.h>
#include <stdlib.h>

/* slightly nasty: table defs are normally hidden */
__externC struct _cyg_nand_device_t cyg_nanddevtab[];
__externC struct _cyg_nand_device_t cyg_nanddevtab_end;

#define MUST(what) do { CYG_TEST_CHECK((what), #what); } while(0)

#include "ar4prng.inl"

const char msg[] = "NAND damage loop test";

/* we'll use our text segment as seed to the prng; not crypto-secure but
 * pretty good for running tests */
extern unsigned char _stext[], _etext[];

externC unsigned cyg_nand_synth_get_losscount(void); // from nand_synth.c

ar4ctx rnd;
cyg_nand_device *dev;
cyg_nand_partition *prt;
cyg_nand_block_addr blk;
cyg_nand_page_addr pg;

#define datasize CYGNUM_NAND_PAGEBUFFER

unsigned char buf[datasize], buf2[datasize];
char msgbuf[256];

void cycle(void)
{
    ar4prng_many(&rnd, buf, datasize);

    MUST(0==cyg_nand_write_page(prt, pg, buf, datasize, 0, 0));
    MUST(0==cyg_nand_read_page (prt, pg, buf2,datasize, 0, 0));
    MUST(0==memcmp(buf, buf2, datasize));
    MUST(0==cyg_nand_erase_block(prt, blk));
}

int cyg_user_start(void)
{
    int i,seed;

    CYG_TEST_INIT();
    CYG_TEST_INFO(msg);
    ar4prng_init(&rnd,(unsigned char*)msg,strlen(msg));
    ar4prng_many(&rnd, (void*)&seed, sizeof seed);
    srand(seed); // for synth randomly_lose

    if (cyg_nanddevtab == &cyg_nanddevtab_end)
        CYG_TEST_NA("No NAND devices found");

    snprintf(msgbuf, sizeof(msgbuf)-1, "Using NAND device %s", cyg_nanddevtab->devname);
    CYG_TEST_INFO(msgbuf);
    CYG_TEST_CHECK(0==cyg_nand_lookup(cyg_nanddevtab->devname, &dev),"lookup failed");

    prt = cyg_nand_get_partition(dev, 0);

    blk = prt->first;
    pg = CYG_NAND_BLOCK2PAGEADDR(dev,blk);

    MUST(0==cyg_nand_erase_block(prt, blk));
    for(i=0; i<1000; i++)
        cycle();

    unsigned losses = cyg_nand_synth_get_losscount();
    printf("%u losses happened\n",losses);
    CYG_TEST_PASS_FINISH(msg);
}

#endif
