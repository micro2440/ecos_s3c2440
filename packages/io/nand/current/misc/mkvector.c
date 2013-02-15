//      mkvector.c
//
//      Routine to create ECC known answer vectors for nandunit.c
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

/* This is a quick & dirty hostside harness to create known-answer ECC
 * test vectors.
 * It uses ar4prng.inl to generate its test data.
 * The output is human-readable, intended to be turned into code; the
 * results have been incorporated into nandunit.c. Each output is an
 * arc4 input ("key") string, followed by the ECC output of each of the
 * first eight 256-byte chunks from running arc4 on that key string.
 *
 * An ECC algorithm is required. In its current form, this code
 * slightly nastily #includes Charles Manning's ECC implementation,
 * but any ECC algorithm compatible with the Linux MTD ECC code should do.
 *
 * This is NOT an eCos test case; it is intended to be run host-side. 
 * (Although there's no inherent reason why you couldn't run it on a board.)
 *
 * To compile, first copy this source and ar4prng.inl, along with
 * yaffs_ecc.c and yaffs_ecc.h from a recent yaffs2 tree, into a
 * build directory; then:
 *      gcc mkvector.c -g -o mkvector
 */


#include <stdio.h>
#include <string.h>
#include "ar4prng.inl"
#include "yaffs_ecc.c"

#define CHUNKSIZE 2048
void doit(char *key)
{
    ar4ctx ctx;
    unsigned char chunk[CHUNKSIZE];
    unsigned char ecc[3*CHUNKSIZE/256];
    int i;

    ar4prng_init(&ctx, key, strlen(key));
    ar4prng_many(&ctx, chunk, CHUNKSIZE);

    for (i=0; i< (CHUNKSIZE/256); i++)
        yaffs_ECCCalculate(&chunk[256*i], &ecc[3*i]);

    printf ("Key: %s\nECC: ", key);
    for (i=0; i<sizeof(ecc); i++)
        printf("%02x", ecc[i]);
    printf("\n\n");
}

int main(int argc, char**argv)
{
    doit("key1");
    doit("key2");
    return 0;
}
