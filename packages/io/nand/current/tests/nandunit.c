//=============================================================================
//
//      nandunit.c 
//
//      Sundry unit tests for various parts of the NAND library
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
// Date:        2009-03-18
//
//####DESCRIPTIONEND####
//=============================================================================

//#define CYG_COMPOUND_TEST
#include <cyg/infra/testcase.h>
#include <cyg/nand/nand.h>
#include <cyg/nand/nand_oob.h>
#include <cyg/infra/diag.h>
#include <string.h>
#include <stdio.h>

/* This one's a bit sneaky, this unit test is independent of
 * any NAND device. So we'll construct a fake plastic cyg_nand_device
 * out of whole cloth. */

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

/* OOB packing =================================================== */

#define CANARY 0xaa
#define canary_subcheck(buf,x,y) do {               \
    int i;                                          \
    for (i=x; i<y; i++)                             \
        if (buf[i] != CANARY)                       \
            CYG_TEST_FAIL_FINISH("canary died");    \
} while(0)

#define canary_check(buf) do {      \
    canary_subcheck(buf,0,32);      \
    canary_subcheck(buf,96,128);    \
} while(0)

int xcanary_check(CYG_BYTE *buf)
{
    int i;
    for (i=0; i<32; i++) {
        if (buf[i] != CANARY) 
            CYG_TEST_FAIL_FINISH("canary died");
    }
    for (i=96; i<128; i++) {
        if (buf[i] != CANARY) 
            CYG_TEST_FAIL_FINISH("canary died");
    }
    return 1;
}

CYG_BYTE oob_buf[128];
CYG_BYTE testpattern[64], recvbuf[128];

int oob_packing(void)
{
    /* We are testing the mechanism, not the specifics of any given layout.
     * We are interested in whether the application data is correctly
     * stored and unpacked, without overrunning anything. */
    init_fakenand();
    CYG_BYTE *oob = &oob_buf[32];
    memset(oob_buf, CANARY, sizeof oob_buf); // To act as a simple canary
    canary_check(oob_buf);

    CYG_BYTE *rcv = &recvbuf[32];
    memset(recvbuf, CANARY, sizeof recvbuf);
    int i;
    for (i=0; i<64; i++) 
        testpattern[i] = i^0xf4;

    canary_check(oob_buf);
    nand_oob_pack(&fakenand, testpattern, sizeof testpattern, testpattern/*ecc*/, oob);
    canary_check(oob_buf);

    canary_check(recvbuf);
    nand_oob_unpack(&fakenand, rcv, 64, 0/*ecc*/, oob);

    //recvbuf[96] = 0; // uncomment to test the canary mechanism..
    canary_check(oob_buf);
    canary_check(recvbuf);

    if (0 == memcmp(rcv, testpattern, fakenand.oob->app_size)) 
        CYG_TEST_PASS("oob pack/unpack unit test");
    else
        CYG_TEST_FAIL_FINISH("oob pack/unpack unit test mismatch");

    return 1;
}

/* ECC known answer tests ======================================== */

/* An arc4 PRNG gives us known-answer tests. We'll take
 * a few 2048 byte chunks from it and compute their ECCs
 * with an independent implementation. */

#include "ar4prng.inl"

#define CHUNKSIZE 2048
#define ECCSIZE 3*(CHUNKSIZE/256)

typedef struct {
    char *seed;
    unsigned char ecc[ECCSIZE];
} eccvector;

/* These vectors were independently computed using Charles Manning's
 * ECC implementation. See mkvector.c. */
eccvector vec[] =
{
    { "key1", { 0xa5, 0x65, 0xab, 0xff, 0x00, 0xcf, 0xf0, 0xf3,
                0x0f, 0xcc, 0xcc, 0xf3, 0xa9, 0x55, 0x57, 0x30,
                0x0f, 0xcf, 0x30, 0xc3, 0xf3, 0x30, 0xcf, 0xc3, } },
    { "key2", { 0x96, 0x9a, 0x57, 0x55, 0x66, 0xa7, 0x55, 0xa5,
                0x6b, 0xcc, 0x0f, 0xcf, 0x30, 0x03, 0x33, 0x96,
                0x9a, 0x5b, 0xa5, 0x69, 0x6b, 0x0c, 0x00, 0x33, } },
    { 0 }
};

unsigned char chunk[CHUNKSIZE],
              ecc[ECCSIZE];

ar4ctx ctx;

int do_ecc_known_answer(eccvector *v)
{
    ar4prng_init(&ctx, (unsigned char*)v->seed, strlen(v->seed));
    ar4prng_many(&ctx, chunk, CHUNKSIZE);
    int i;
    const int nblocks = (1<<fakenand.page_bits) / fakenand.ecc->data_size;
    unsigned char *page = &chunk[0];
    CYG_BYTE *ecc_o = &ecc[0];

    for (i=0; i<nblocks; i++) {
        if (fakenand.ecc->init) fakenand.ecc->init(&fakenand);
        fakenand.ecc->calc_wr(&fakenand, page, ecc_o);
        page += fakenand.ecc->data_size;
        ecc_o += fakenand.ecc->ecc_size;
    }

    int pass = (0 == memcmp(ecc, v->ecc, ECCSIZE));
    if (!pass)
        diag_printf("Test with key %s mismatched\n", v->seed);
#if 0
    printf("Test key %s:\nECC: ",v->seed);
    int i;
    for (i=0; i<ECCSIZE; i++)
        printf("%02x", ecc[i]);
    printf("\n");
#endif

    return pass;
}

int ecc_known_answers(void)
{
    init_fakenand();
    eccvector *v = vec;
    int fails = 0;
    while (v->seed) {
        if (1 != do_ecc_known_answer(v))
            ++fails;
        ++v;
    }
    if (fails) CYG_TEST_FAIL_FINISH("ECC known answer check");
    else CYG_TEST_PASS("ECC known answer check");

    return 0;
}


int cyg_user_start(void)
{
    CYG_TEST_INIT();
    CYG_TEST_INFO("NAND unit tests");

    oob_packing();
    ecc_known_answers();

    CYG_TEST_FINISH("Run complete.");
    return 0;
}
