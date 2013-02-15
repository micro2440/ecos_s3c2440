//=============================================================================
//
//      eccwalk.c
//
//      Walks some sample data through the NAND flash controller to
//      check that the ECC computation and repair operate correctly.
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

#define datasize 512
unsigned char buf[datasize];

#define NAND_BASE   CYGHWR_HAL_STM32_FSMC_BANK2_BASE

void ecc(cyg_nand_device *dev, const char *msg, CYG_BYTE *out)
{
    dev->ecc->init(dev);

    // This is the sneaky bit: write data through the NAND controller
    // so the ECC register updates, but not framed by NAND commands i.e.
    // not actually affecting the chip.
    // This is necessarily board-specific ...
    HAL_WRITE_UINT8_VECTOR(NAND_BASE, buf, datasize, 0);

    dev->ecc->calc(dev, 0, out);
    diag_printf("%s: ecc=%02x%02x%02x\n", msg, out[0], out[1], out[2]);
}

const char msg[]="ECC walker";

int cyg_user_start(void)
{
    cyg_nand_device *dev;
    int i;
    CYG_BYTE ecc1[3], ecc2[3];

    CYG_TEST_INIT();
    CYG_TEST_INFO(msg);

    if (cyg_nanddevtab == &cyg_nanddevtab_end)
        CYG_TEST_NA("No NAND devices found");

    CYG_TEST_CHECK(0==cyg_nand_lookup("onboard", &dev),"lookup failed");

    // Plan: Start with a buffer full of 0xFF; then, changing one bit 
    // at a time, compute a fresh ECC and check that the repair code
    // does the right thing.

    for (i=0; i<datasize; i++) buf[i]=0xff;
    ecc(dev, "all-ff", ecc1);


    for (i=0; i<8; i++) {
        int st;
        char msg[] = "bit X";
        buf[0] ^= 1<<i;
        msg[4] = '0' + i;
        ecc(dev, msg, ecc2);

        st = dev->ecc->repair(dev, buf, sizeof buf, ecc1, ecc2);
        CYG_ASSERTC(st==1);
        CYG_ASSERTC(buf[0] == 0xff);

        ecc(dev, msg, ecc2);
        st = dev->ecc->repair(dev, buf, sizeof buf, ecc1, ecc2);
        CYG_ASSERTC(st==1);
    }
    for (i=0; i<9; i++) {
        char msg[] = "bytelog X";
        int st;
        msg[8] = '0' + i;
        buf[1<<i] ^= 1;
        ecc(dev, msg, ecc2);
        st = dev->ecc->repair(dev, buf, sizeof buf, ecc1, ecc2);
        CYG_ASSERTC(st==1);
        CYG_ASSERTC(buf[1<<i] == 0xff);

        ecc(dev, msg, ecc2);
        st = dev->ecc->repair(dev, buf, sizeof buf, ecc1, ecc2);
        CYG_ASSERTC(st==1);
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

