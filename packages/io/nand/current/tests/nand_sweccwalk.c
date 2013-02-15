//=============================================================================
//
//      sweccwalk.c
//
//      Walks some sample data through the software ECC algorithm to
//      check that computation and repair operate correctly.
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
// Date:        2009-10-27
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

#define MUST(what) do { CYG_TEST_CHECK((what), #what); } while(0)

static CYG_NAND_DEVICE(fakenand, 0, 0, 0, &mtd_ecc256_fast, 0);

void init_fakenand(void)
{
    if (fakenand.is_inited)
        return;

    fakenand.is_inited = 1;
    fakenand.pf = diag_printf;
    fakenand.page_bits = 11; // 2048 byte test pattern.
    fakenand.oob = &nand_mtd_oob_64;
}


void ecc(cyg_nand_device *dev, const CYG_BYTE *data, const char *msg, CYG_BYTE *out)
{
    if (dev->ecc->init)
        dev->ecc->init(dev);
    dev->ecc->calc_wr(dev, data, out);
    diag_printf("%s: ecc=%02x%02x%02x\n", msg, out[0], out[1], out[2]);
    // TODO adapt this display for ECCs longer than 3 bytes
}

const char msg[]="software ECC walker";

#define ECC_BUFFER_SIZE 10
CYG_BYTE buf[CYGNUM_NAND_PAGEBUFFER];
CYG_BYTE ecc1[ECC_BUFFER_SIZE], ecc2[ECC_BUFFER_SIZE];

int cyg_user_start(void)
{
    cyg_nand_device *dev = &fakenand;
    int i;

    CYG_TEST_INIT();
    CYG_TEST_INFO(msg);

#if 0
    // This code can be adapted to test a hardware ECC algorithm but
    // this usually requires some cunning in ecc() to cause the test
    // data to pass through the hardware.
    if (cyg_nanddevtab == &cyg_nanddevtab_end)
        CYG_TEST_NA("No NAND devices found");

    CYG_TEST_CHECK(0==cyg_nand_lookup("onboard", &dev),"lookup failed");
#endif

#define datasize (fakenand.ecc->data_size)
#define eccsize  (fakenand.ecc->ecc_size)

    CYG_ASSERTC(datasize <= CYGNUM_NAND_PAGEBUFFER);
    CYG_ASSERTC(eccsize <= ECC_BUFFER_SIZE);

    diag_printf("ECC sizes: data %d, ecc %d\n", datasize, eccsize);

    // Plan: Start with a buffer full of 0xFF; then, changing one bit 
    // at a time, compute a fresh ECC and check that the repair code
    // does the right thing. This code can also be used to help work
    // out what's going on with an unclearly-documented ECC algorithm.

    memset(buf, 255, sizeof(buf));
    ecc(dev, buf, "all-ff", ecc1);

    for (i=0; i<8; i++) {
        int st;
        char msg[] = "bit X";
        buf[0] ^= 1<<i;
        msg[4] = '0' + i;
        ecc(dev, buf, msg, ecc2);

        st = dev->ecc->repair(dev, buf, sizeof buf, ecc1, ecc2);
        CYG_ASSERTC(st==1);
        CYG_ASSERTC(buf[0] == 0xff);

        ecc(dev, buf, msg, ecc2);
        st = dev->ecc->repair(dev, buf, sizeof buf, ecc1, ecc2);
        CYG_ASSERTC(st==0);
    }
    int max;
    switch(datasize) {
        case 256: max = 8; break;
        case 512: max = 9; break;
        // Add further cases here for different ECC data sizes.
        default:
                  diag_printf("Unhandled case datasize=%d\n",datasize);
                  CYG_TEST_FAIL_EXIT("Unhandled case!");
    }
    for (i=0; i<max; i++) {
        char msg[256];
        int st;
        diag_snprintf(msg, sizeof msg, "bytelog %d", i);

        buf[1<<i] ^= 1;
        ecc(dev, buf, msg, ecc2);
        st = dev->ecc->repair(dev, buf, sizeof buf, ecc1, ecc2);
        CYG_ASSERTC(st==1);
        CYG_ASSERTC(buf[1<<i] == 0xff);

        ecc(dev, buf, msg, ecc2);
        st = dev->ecc->repair(dev, buf, sizeof buf, ecc1, ecc2);
        CYG_ASSERTC(st==0);
    }

    int fail=0;
    for (i=0; i<datasize; i++) {
        if (buf[i] != 0xff) {
            ++fail;
            diag_printf("buf[%d] == %02x != 0xff\n", i, buf[i]);
        }
    }

    if (fail)
        CYG_TEST_FAIL_FINISH(msg);
    else
        CYG_TEST_PASS_FINISH(msg);
}

