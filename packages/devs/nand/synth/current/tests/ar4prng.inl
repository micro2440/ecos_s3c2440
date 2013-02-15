//=============================================================================
//
//      ar4prng.inl
//
//      Inline providing ARC4-based PRNG
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

#define _SWAP(_a,_b) do { int tmp = (_a); (_a) = (_b); (_b) = tmp; } while(0)

typedef struct {
    unsigned char S[256];
    int i,j;
} ar4ctx;

void ar4prng_init(ar4ctx *c, unsigned char *seed, unsigned seedlen)
{
    int i,j=0;
    for (i=0; i<256; i++) c->S[i] = i;
    for (i=0; i<256; i++) {
        j = (j + c->S[i] + seed[i%seedlen]) %256;
        _SWAP(c->S[i], c->S[j]);
    }
    c->i = c->j = 0;
}

unsigned char ar4prng_next(ar4ctx *c)
{
    c->i = (c->i + 1) %256;
    c->j = (c->j + c->S[c->i]) %256;
    _SWAP(c->S[c->i], c->S[c->j]);
    return c->S[(c->S[c->i] + c->S[c->j])%256];
}

void ar4prng_many(ar4ctx *c, unsigned char *optr, unsigned len)
{
    while (len--) *(optr++) = ar4prng_next(c);
}

#undef _SWAP

