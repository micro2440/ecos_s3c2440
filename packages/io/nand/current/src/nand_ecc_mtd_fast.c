//=============================================================================
//
//      nand_ecc_mtd_fast.c
//
//      ECC algorithm compatible with the Linux MTD layer
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

/* Note on development:
 *
 * Inspired by the useful discussion of Frans Meulenbroeks' efforts to speed
 * up software ECC[0], I set about rewriting our code from scratch. 
 * I have not referred to his source (drivers/mtd/nand/nand_ecc.c) which
 * is GPL.
 * - wry
 *
 * [0] Recently committed to the Linux MTD tree. See 
 * http://git.infradead.org/mtd-2.6.git/blob/HEAD:/Documentation/mtd/nand_ecc.txt
 */

#include <cyg/infra/cyg_type.h>
#include <cyg/nand/nand_ecc.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* This is a precomputed inverse-parity table for single bytes.
 * If your camel's back is broken, you could push it into RAM
 * and precompute it at boot time, or even fall back to software
 * parity calculation. (However, you will also have to fix up
 * anything which uses it. Beware!)
 *
 * You might find Sean Eron Anderson's collection of "Bit Twiddling Hacks"
 * useful - see http://graphics.stanford.edu/~seander/bithacks.html .
 * That's where this (public domain) macro to generate the table comes from.
 */

const CYG_BYTE ecc256_ParityTable256[256] =
{
#   define P2(n) n, n^1, n^1, n
#   define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
#   define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)
        P6(1), P6(0), P6(0), P6(1)
};
#define BYTEPAR(i) ecc256_bytepar(i)

static void ecc256_fast(struct _cyg_nand_device_t *dev, const CYG_BYTE *data,
                        CYG_BYTE *ecc)
{
    cyg_uint32 c; // current data word
    cyg_uint32 cp,tp; // column and loop-temporary accumulators
    // row accumulators
    cyg_uint32 p8,  p16,  p32,  p64,  p128,  p256,  p512,  p1024;
    // alternate-row results holders
    cyg_uint32 p8p, p16p, p32p, p64p, p128p, p256p, p512p, p1024p;
    unsigned i;
    cyg_uint32 *d = (cyg_uint32*)data;

    cp=p8=p16=p32=p64=p128=p256=p512=p1024=0;

    for (i=0; i<8; i++) {
#define NEXT() (c=*(d++)) // do as a macro so we can endian fix later if need be

        tp = NEXT();
        tp^= NEXT(); p32  ^= c;
        tp^= NEXT(); p64  ^= c;
        tp^= NEXT(); p32  ^= c; p64 ^= c;
        p128 ^= tp;

        tp^= NEXT();
        tp^= NEXT(); p32  ^= c;
        tp^= NEXT(); p64  ^= c;
        tp^= NEXT(); p32  ^= c; p64  ^= c;
        p128 ^= tp; // (0--3) (+) (0--7) = (4--7) : saves a couple more XORs :-)

        switch(i) {
            case 7:
                p256 ^= tp;
            case 6:
                p512 ^= tp;
            case 4:
                p1024^= tp;
                break;
            case 5:
                p1024^= tp;
            case 1:
                p256 ^= tp;
                break;
            case 3:
                p256 ^= tp;
            case 2:
                p512 ^= tp;
            case 0: ;
        }
        cp^= tp;
    }
#define DOALL(M) do { M(32); M(64); M(128); M(256); M(512); M(1024); } while(0)

    // de-bitslice p32 through p1024
#define SQUASH1(vv) { vv ^= (vv>>16); vv = (vv ^ (vv>>8)) & 0xff; }
#define SQUASH(n) do { SQUASH1(p##n) } while(0)
    DOALL(SQUASH);

    // de-bitslice p16, p8 and column parity
    p16 = (cp >> 16) & 0xffff;
    p16 = (p16 ^ (p16 >>8))& 0xff;

    cp ^= cp>>16;
    p8 = cp>>8;

    cp ^= cp>>8;

    // Now, finally, we can work out p8' through p1024' ...
#define COMPUTE(n) do { p##n##p = cp ^ p##n; } while(0)
    DOALL(COMPUTE);
    COMPUTE(16);
    COMPUTE(8);

    ecc[0] = BYTEPAR(p1024) << 7
           | BYTEPAR(p1024p)<< 6
           | BYTEPAR(p512)  << 5
           | BYTEPAR(p512p) << 4
           | BYTEPAR(p256)  << 3
           | BYTEPAR(p256p) << 2
           | BYTEPAR(p128)  << 1
           | BYTEPAR(p128p);

    ecc[1] = BYTEPAR(p64)  << 7
           | BYTEPAR(p64p) << 6
           | BYTEPAR(p32)  << 5
           | BYTEPAR(p32p) << 4
           | BYTEPAR(p16)  << 3
           | BYTEPAR(p16p) << 2
           | BYTEPAR(p8)   << 1
           | BYTEPAR(p8p);

    ecc[2] = BYTEPAR(cp & 0xf0) << 7 // p4
           | BYTEPAR(cp & 0x0f) << 6 // p4'
           | BYTEPAR(cp & 0xcc) << 5 // p2
           | BYTEPAR(cp & 0x33) << 4 // p2'
           | BYTEPAR(cp & 0xaa) << 3 // p1
           | BYTEPAR(cp & 0x55) << 2 // p1'
           | 3;
}

static int ecc256_repair(struct _cyg_nand_device_t *dev,
                         CYG_BYTE *dat, size_t nbytes,
                         CYG_BYTE *read_ecc, const CYG_BYTE *calc_ecc)
{
    cyg_uint32 x = (calc_ecc[0] << 16) | (calc_ecc[1] << 8) | calc_ecc[2];
    cyg_uint32 y = (read_ecc[0] << 16) | (read_ecc[1] << 8) | read_ecc[2];

    x ^= y;
    if (x == 0) return 0; // Nothing to do

    // Can we fix it? 11 bits should be set, and as they're stored
    // in adjacent pairs within each byte we can use this neat XOR
    // trick to make sure that precisely one bit of each pair is set.

    if (( (x ^ (x>>1)) & 0x555554 ) == 0x555554 ) {
        // Yes we can ! Figure out the offset.
        unsigned byte, bit; 

        byte = ( (x >>16) & 0x80) |  // P1024 = 0x80 byte addr
               ( (x >>15) & 0x40) |
               ( (x >>14) & 0x20) |
               ( (x >>13) & 0x10) |
               ( (x >>12) & 8) |
               ( (x >>11) & 4) |
               ( (x >>10) & 2) |
               ( (x >> 9) & 1);      // ... P8

        bit = ( (x >> 3) & 1) |      // P1
              ( (x >> 4) & 2) |      // P2
              ( (x >> 5) & 4);       // and P4.

        if (byte < nbytes)
            dat[byte] ^= (1<<bit);

        return 1;
    }

    // No? Is it an ECC dropout? Count the bits...
    unsigned i=0;
    while (x) {
        if (x & 1) ++i;
        x >>= 1;
    }
    if (i==1) {
        read_ecc[0] = calc_ecc[0];
        read_ecc[1] = calc_ecc[1];
        read_ecc[2] = calc_ecc[2];
        return 2;
    }
    // Alas, it is uncorrectable.
    return -1;
}

CYG_NAND_ECC_ALG_SW(mtd_ecc256_fast, 256, 3, 0, ecc256_fast, ecc256_repair);

