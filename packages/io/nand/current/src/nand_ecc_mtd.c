//=============================================================================
//
//      nand_ecc_mtd.c
//
//      ECC algorithm from the Linux MTD layer for the eCos NAND flash library
//
//=============================================================================
//
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   Steven J. Hill
// Contributors: wry, to patch into eCos NAND library.
// Note:        See copyright comment and original boilerplate below.
//
//####DESCRIPTIONEND####
//=============================================================================

/* Copyright note:
 * I downloaded this file from 
 *     http://git.infradead.org/mtd-cvs.git?a=blob_plain;f=drivers/mtd/nand/nand_ecc.c;hb=e58fa2ce1b32fbc93511b92259c2473e89c463ea
 * To confirm its copyright status, check its history:
 *     http://git.infradead.org/mtd-cvs.git?a=history;f=drivers/mtd/nand/nand_ecc.c
 * Note that the commit changing this file (in this incarnation) to
 * GPL+Library Exception was made by sjhill, its original author.
 *
 * - wry, 2009-03-13
 */

/*
 * This file contains an ECC algorithm from Toshiba that detects and
 * corrects 1 bit errors in a 256 byte block of data.
 *
 * drivers/mtd/nand/nand_ecc.c
 *
 * Copyright (C) 2000-2004 Steven J. Hill (sjhill@realitydiluted.com)
 *                         Toshiba America Electronics Components, Inc.
 *
 * $Id: nand_ecc.c,v 1.11 2004/05/05 14:19:42 sjhill Exp $
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 or (at your option) any
 * later version.
 * 
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this file; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 * 
 * As a special exception, if other files instantiate templates or use
 * macros or inline functions from these files, or you compile these
 * files and link them with other works to produce a work based on these
 * files, these files do not by themselves cause the resulting work to be
 * covered by the GNU General Public License. However the source code for
 * these files must still be made available in accordance with section (3)
 * of the GNU General Public License.
 * 
 * This exception does not invalidate any other reasons why a work based on
 * this file might be covered by the GNU General Public License.
 */

#include <cyg/infra/cyg_type.h>
#include <cyg/nand/nand_ecc.h>
typedef CYG_BYTE u_char;

/*
 * Pre-calculated 256-way 1 byte column parity
 */
static const u_char nand_ecc_precalc_table[] = {
    0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
    0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
    0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
    0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
    0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
    0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
    0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
    0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
    0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
    0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
    0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
    0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
    0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
    0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
    0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
    0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00
};


/*
 * Creates non-inverted ECC code from line parity
 */
void nand_trans_result(u_char reg2, u_char reg3,
    u_char *ecc_code)
{
    u_char a, b, i, tmp1, tmp2;
    
    /* Initialize variables */
    a = b = 0x80;
    tmp1 = tmp2 = 0;
    
    /* Calculate first ECC byte */
    for (i = 0; i < 4; i++) {
        if (reg3 & a)       /* LP15,13,11,9 --> ecc_code[0] */
            tmp1 |= b;
        b >>= 1;
        if (reg2 & a)       /* LP14,12,10,8 --> ecc_code[0] */
            tmp1 |= b;
        b >>= 1;
        a >>= 1;
    }
    
    /* Calculate second ECC byte */
    b = 0x80;
    for (i = 0; i < 4; i++) {
        if (reg3 & a)       /* LP7,5,3,1 --> ecc_code[1] */
            tmp2 |= b;
        b >>= 1;
        if (reg2 & a)       /* LP6,4,2,0 --> ecc_code[1] */
            tmp2 |= b;
        b >>= 1;
        a >>= 1;
    }
    
    /* Store two of the ECC bytes */
    ecc_code[0] = tmp1;
    ecc_code[1] = tmp2;
}

/*
 * Calculate 3 byte ECC code for 256 byte block
 */
static void mtd_calculate_ecc(struct _cyg_nand_device_t *dev,
        const u_char *dat, u_char *ecc_code)
{
    u_char idx, reg1, reg2, reg3;
    int j;
    
    /* Initialize variables */
    reg1 = reg2 = reg3 = 0;
    ecc_code[0] = ecc_code[1] = ecc_code[2] = 0;
    
    /* Build up column parity */ 
    for(j = 0; j < 256; j++) {
        u_char d = dat[j];
        
        /* Get CP0 - CP5 from table */
        idx = nand_ecc_precalc_table[d];
        reg1 ^= (idx & 0x3f);
        
        /* All bit XOR = 1 ? */
        if (idx & 0x40) {
            reg3 ^= (u_char) j;
            reg2 ^= ~((u_char) j);
        }
    }
    
    /* Create non-inverted ECC code from line parity */
    nand_trans_result(reg2, reg3, ecc_code);
    
    /* Calculate final ECC code */
    ecc_code[0] = ~ecc_code[0];
    ecc_code[1] = ~ecc_code[1];
    ecc_code[2] = ((~reg1) << 2) | 0x03;
}

/*
 * Detect and correct a 1 bit error for 256 byte block
 */
static int mtd_correct_data(struct _cyg_nand_device_t *dev,
        u_char *dat, size_t nbytes, u_char *read_ecc, const u_char *calc_ecc)
{
    u_char a, b, c, d1, d2, d3, add, bit, i;
    
    /* Do error detection */ 
    d1 = calc_ecc[0] ^ read_ecc[0];
    d2 = calc_ecc[1] ^ read_ecc[1];
    d3 = calc_ecc[2] ^ read_ecc[2];
    
    if ((d1 | d2 | d3) == 0) {
        /* No errors */
        return 0;
    }
    else {
        a = (d1 ^ (d1 >> 1)) & 0x55;
        b = (d2 ^ (d2 >> 1)) & 0x55;
        c = (d3 ^ (d3 >> 1)) & 0x54;
        
        /* Found and will correct single bit error in the data */
        if ((a == 0x55) && (b == 0x55) && (c == 0x54)) {
            c = 0x80;
            add = 0;
            a = 0x80;
            for (i=0; i<4; i++) {
                if (d1 & c)
                    add |= a;
                c >>= 2;
                a >>= 1;
            }
            c = 0x80;
            for (i=0; i<4; i++) {
                if (d2 & c)
                    add |= a;
                c >>= 2;
                a >>= 1;
            }
            bit = 0;
            b = 0x04;
            c = 0x80;
            for (i=0; i<3; i++) {
                if (d3 & c)
                    bit |= b;
                c >>= 2;
                b >>= 1;
            }
            b = 0x01;

            if (add < nbytes) {
                a = dat[add];
                a ^= (b << bit);
                dat[add] = a;
            }
            return 1;
        }
        else {
            i = 0;
            while (d1) {
                if (d1 & 0x01)
                    ++i;
                d1 >>= 1;
            }
            while (d2) {
                if (d2 & 0x01)
                    ++i;
                d2 >>= 1;
            }
            while (d3) {
                if (d3 & 0x01)
                    ++i;
                d3 >>= 1;
            }
            if (i == 1) {
                /* ECC Code Error Correction */
                read_ecc[0] = calc_ecc[0];
                read_ecc[1] = calc_ecc[1];
                read_ecc[2] = calc_ecc[2];
                return 2;
            }
            else {
                /* Uncorrectable Error */
                return -1;
            }
        }
    }
    
    /* Should never happen */
    return -1;
}

CYG_NAND_ECC_ALG_SW(linux_mtd_ecc, 256, 3, NULL, mtd_calculate_ecc, mtd_correct_data);

