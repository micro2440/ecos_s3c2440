//=============================================================================
//
//      eccequiv.c
//
//      Given two software ECC algorithms, confirms their equivalence.
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
// Date:        2009-11-06
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
#include "ar4prng.inl"

#define MUST(what) do { CYG_TEST_CHECK((what), #what); } while(0)

__externC cyg_nand_ecc_t linux_mtd_ecc;

static CYG_NAND_DEVICE(fakenand, 0, 0, 0, &linux_mtd_ecc, 0);
static CYG_NAND_DEVICE(fakenand2, 0, 0, 0, &mtd_ecc256_fast, 0);

void init_fakenand(void)
{
    if (fakenand.is_inited)
        return;

    fakenand.is_inited = 1;
    fakenand.pf = diag_printf;
    fakenand.page_bits = 11; // 2048 byte test pattern.
    fakenand.oob = &nand_mtd_oob_64;

    fakenand2.is_inited = 1;
    fakenand2.pf = diag_printf;
    fakenand2.page_bits = 11; // 2048 byte test pattern.
    fakenand2.oob = &nand_mtd_oob_64;
}


void ecc(cyg_nand_device *dev, const CYG_BYTE *data, CYG_BYTE *out)
{
    if (dev->ecc->init)
        dev->ecc->init(dev);
    dev->ecc->calc_wr(dev, data, out);
}

const char msg[]="ECC equivalence check";

#define ECC_BUFFER_SIZE 10
CYG_BYTE buf[CYGNUM_NAND_PAGEBUFFER];
CYG_BYTE ecc1[ECC_BUFFER_SIZE], ecc2[ECC_BUFFER_SIZE];

ar4ctx rng;
extern unsigned char _stext[], _etext[];

int cyg_user_start(void)
{
    cyg_nand_device *dev = &fakenand, *dev2 = &fakenand2;
    int i, fails=0;

    CYG_TEST_INIT();
    CYG_TEST_INFO(msg);

#define datasize (fakenand.ecc->data_size)
#define eccsize  (fakenand.ecc->ecc_size)

    CYG_ASSERTC(fakenand.ecc->data_size == fakenand2.ecc->data_size);
    CYG_ASSERTC(fakenand.ecc->ecc_size  == fakenand2.ecc->ecc_size);

    CYG_ASSERTC(datasize <= CYGNUM_NAND_PAGEBUFFER);
    CYG_ASSERTC(eccsize <= ECC_BUFFER_SIZE);

    diag_printf("ECC sizes: data %d, ecc %d\n", datasize, eccsize);

    ar4prng_init(&rng,_stext, _etext-_stext);

#define N_RUNS 10000

    for (i=0; i<N_RUNS; i++) {
        ar4prng_many(&rng, buf, datasize);
        ecc(dev,  buf, ecc1);
        ecc(dev2, buf, ecc2);
        if (0 != memcmp(ecc1, ecc2, eccsize)) {
            ++fails;
        }
    }

    if (fails) {
        diag_printf("FAIL: %d mismatches\n", fails);
        CYG_TEST_FAIL_FINISH(msg);
    } else
        CYG_TEST_PASS_FINISH(msg);
}

