//      ar4test.c
//
//      Meta-test for the arc4 PRNG algorithm in ar4prng.inl
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

/* This is a rough and ready meta-test of the PRNG using the known-answer 
 * vectors found on http://en.wikipedia.org/wiki/RC4.
 *
 * This is NOT an eCos test case; it is intended to be run host-side. 
 * (Although there's no inherent reason why you couldn't run it on a board.)
 * Compile with:   gcc ar4test.c -g -o ar4test
 */

#include "../tests/ar4prng.inl"

#include <alloca.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char *key;
    char *plaintext;
    char *ct_hex;
} vector;

vector master[] = {
    { "Key", "Plaintext", "BBF316E8D940AF0AD3" },
    { "Wiki", "pedia", "1021BF0420" },
    { "Secret", "Attack at dawn", "45A01F645FC35B383552544B9BF5" },
    { 0,0,0 }
};

// ascii nybble to binary
unsigned char N2B(char p)
{
    if (p>='0' && p<='9')
        return p-'0';
    if (p>='A' && p<='F')
        return 10+p-'A';
    if (p>='a' && p<='f')
        return 10+p-'a';
    assert(0=="bad hex");
}

void test (vector *v)
{
    ar4ctx ctx;
    int textlen = strlen(v->plaintext), i;
    unsigned char *ct = alloca(textlen),
                  *ct_comp = alloca(textlen);

    for (i=0; i<textlen; i++) {
        ct[i] = ( N2B(v->ct_hex[i*2]) << 4 )
                | N2B(v->ct_hex[i*2+1]);
    }

    ar4prng_init(&ctx, v->key, strlen(v->key));
    ar4prng_many(&ctx, ct_comp, textlen);
    for (i=0; i<textlen; i++)
        ct_comp[i] ^= v->plaintext[i];

    int pass = (0==memcmp(ct, ct_comp, textlen));
    printf("Test %s: result %s\n", v->key, pass ? "PASS" : "FAIL");
#if 0
    for (i=0; i<textlen; i++)
        printf("%02x", ct_comp[i]);
    printf("\n");   
#endif
}

int main(int argc, char**argv)
{
    vector *vv = master;
    while (vv->key) {
        test(vv);
        ++vv;
    }
    return 0;
}

