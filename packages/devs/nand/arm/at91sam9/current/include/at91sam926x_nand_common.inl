//=============================================================================
//
//      at91sam9_nand.inl
//
//      NAND flash support for the AT91SAM926X family
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
// Author(s):   wry
// Date:        2010-03-08
//
//####DESCRIPTIONEND####
//=============================================================================

// NOTE: This package is a bit of a layering violation at present.
// It depends on both the CHIP driver (for change_read_column_lp) and
// the BOARD driver (for read_data_bulk). As such it has to be a
// .inl file at present.

#include <cyg/nand/nand_device.h>
#include <cyg/nand/nand_ecc.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>
#include <cyg/hal/plf_io.h>

#include <cyg/devs/nand/at91sam926x_nand_common.h>

/* Hardware ECC ======================================================== */

static void at91sam9_ecc_init_largepage(cyg_nand_device *dev)
{
    // Re-set the page size in case the ECC unit has been used by
    // something else.
    SAM9_WRITE_ECC(MR, SAM9_ECC_MR_PAGESIZE_2112W);
    SAM9_WRITE_ECC(CR, SAM9_ECC_CR_RST);
}

static void at91sam9_ecc_calc_wr(cyg_nand_device *dev, const CYG_BYTE *dat, CYG_BYTE *ecc)
{
    cyg_uint32 par, npar; // only bottom 16 bits used of each
    SAM9_READ_ECC(PR, par);
    SAM9_READ_ECC(NPR, npar);
    ecc[0] = par & 0xff;
    ecc[1] = (par >> 8) & 0xff;
    ecc[2] = npar & 0xff;
    ecc[3] = (npar >>8) & 0xff;
}

static void at91sam9_ecc_calc_rd(cyg_nand_device *dev, const CYG_BYTE *dat, CYG_BYTE *ecc)
{
    // NB! This only works when ecc_stride == page size!
    // It relies on the fact that read_finish is about to
    // change_read_column_lp() before picking up the ECC.
    unsigned i, nread=0;
    const unsigned need = dev->ecc->ecc_size;
    CYG_BYTE ecctemp[need];

    for (i=0; i<CYG_NAND_OOB_MAX_APP_SLOTS; i++) {
        change_read_column_lp(dev, (1 << dev->page_bits) + dev->oob->ecc[i].pos);
        int n_thisvec = dev->oob->ecc[i].len;
        read_data_bulk(dev, ecctemp, n_thisvec);
        // we ignore the read but the ECC unit picks it up. From the
        // datasheet: "Values in ECC_PR and ECC_NPR are valid and locked
        // until a new start condition occurs."
        nread += n_thisvec;
        if (nread >= need) break;
    }
    at91sam9_ecc_calc_wr(dev, dat, ecc);
    change_read_column_lp(dev, (1 << dev->page_bits));
}

static int at91sam9_ecc_repair(cyg_nand_device *dev,
                            CYG_BYTE *dat, size_t nbytes,
                            CYG_BYTE *read_ecc, const CYG_BYTE *calc_ecc)
{
    /* The ECC unit in does the ECC check on read for us,
     * _provided_ we read strictly in the order (pagedata, ecc bytes).
     * See atmel_k9_read_finish() above.
     */
    cyg_uint32 status, location;
    SAM9_READ_ECC(SR, status);
    status &= SAM9_ECC_SR_ERRMASK;
    if (status==0) return 0; // No errors

    SAM9_READ_ECC(PR, location);
    if (status & SAM9_ECC_SR_MULERR) { // Multiple errors
        // Multiple errors ... or were there? We get a spurious error from
        // a freshly-erased block, which we need to ignore.
        if (location==0xffff) {
            if (    (read_ecc[0] == 0xff) &&
                    (read_ecc[1] == 0xff) &&
                    (read_ecc[2] == 0xff) &&
                    (read_ecc[3] == 0xff))
                return 0;
        }
        return -1;
    }
    if (status & SAM9_ECC_SR_ECCERR) {
        // ECC error. The NAND library throws away the read ECC so we won't bother to correct it.
        return 2;
    }

    // else: single bit correctable error

    unsigned word,bit;

    word= (location & SAM9_ECC_PR_WORDMASK) >> SAM9_ECC_PR_WORDSHIFT;
    bit = location & SAM9_ECC_PR_BITMASK;

#if 0 // TODO
    if (device is 16 bit) {
        // xlate word to byte location.
        byte = (location >> 4) & 0x0fff;
        byte = (byte << 1) + (bit >> 3);
        bit &= 7;
    }
#endif

    if (word < nbytes) dat[word] ^= (1<<bit);
    return 1;
}

CYG_NAND_ECC_ALG_HW2(at91sam9_ecc, 2048, 4,
        at91sam9_ecc_init_largepage, at91sam9_ecc_calc_rd,
        at91sam9_ecc_calc_wr, at91sam9_ecc_repair);

const cyg_nand_oob_layout at91sam9_oob_largepage = {
    .ecc_size = 4,
    .ecc = { { .pos=60, .len=4 } },
    .app_size = 58,
    .app = { { .pos =2, .len=58 } },
};

