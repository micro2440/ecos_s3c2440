#ifndef CYGONCE_AT91SAM926X_NAND_COMMON_H
#define CYGONCE_AT91SAM926X_NAND_COMMON_H
/*=============================================================================
//
//      at91sam926x_nand_common.h
//
//      NAND support common to the at91sam926x family
//
//=============================================================================
//####ECOSPROCOPYRIGHTBEGIN####
//
// This file is part of eCosPro(tm)
// Copyright (C) 2010 eCosCentric Limited
//
// This file is licensed under the eCosPro License v2.0.
// You may not use this file except in compliance with the License.
// A copy of the License can be obtained from:
// http://www.ecoscentric.com/ecospro-license.html
//
//####ECOSPROCOPYRIGHTEND####
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    wry
// Date:         2010-03-12
//
//####DESCRIPTIONEND####
//
//===========================================================================*/

// The OOB layout used on the AT91 by Linux (inter alia)
extern const cyg_nand_oob_layout at91sam9_oob_largepage;
extern const cyg_nand_oob_layout at91sam9_oob_smallpage;

// On-board hardware ECC:
extern cyg_nand_ecc_t at91sam9_ecc;


// Definitions ---------------------------------------------------------

#define AT91_PMC_PCER_PIOA (1<<2)
#define AT91_PMC_PCER_PIOB (1<<3)
#define AT91_PMC_PCER_PIOC (1<<4)
#ifdef CYGPKG_HAL_ARM_ARM9_SAM9263EK
// on the 9263, id 4 covers C, D and E.
#define AT91_PMC_PCER_PIOD (1<<4)
#define AT91_PMC_PCER_PIOE (1<<4)
#endif


// ECC register offsets & bits/flags
#define SAM9_WRITE_ECC(reg, word) HAL_WRITE_UINT32(SAM9_ECC + SAM9_ECC_##reg, word)
#define SAM9_READ_ECC(reg, dest) HAL_READ_UINT32(SAM9_ECC + SAM9_ECC_##reg, dest)

#define SAM9_ECC_CR 0
#define SAM9_ECC_CR_RST 1

#define SAM9_ECC_MR 4
#define SAM9_ECC_MR_PAGESIZE_528W 0
#define SAM9_ECC_MR_PAGESIZE_1056W 1
#define SAM9_ECC_MR_PAGESIZE_2112W 2
#define SAM9_ECC_MR_PAGESIZE_4224W 3

#define SAM9_ECC_SR 8
#define SAM9_ECC_SR_RECERR 1
#define SAM9_ECC_SR_ECCERR 2
#define SAM9_ECC_SR_MULERR 4
#define SAM9_ECC_SR_ERRMASK (SAM9_ECC_SR_RECERR|SAM9_ECC_SR_ECCERR|SAM9_ECC_SR_MULERR)

#define SAM9_ECC_PR 12
#define SAM9_ECC_PR_BITMASK 0xf
#define SAM9_ECC_PR_WORDMASK 0xfff0
#define SAM9_ECC_PR_WORDSHIFT 4

#define SAM9_ECC_NPR 16


#endif // CYGONCE_AT91SAM926X_NAND_COMMON_H
