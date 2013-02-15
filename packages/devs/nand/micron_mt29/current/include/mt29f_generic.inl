//=============================================================================
//
//      mt29f_generic.inl
//
//      Inline include file for the Micron MT29F family
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2010 eCosCentric Limited.
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
// Date:        2010-04-13
//
//####DESCRIPTIONEND####
//=============================================================================

#include <pkgconf/io_nand.h>
#include <cyg/devs/nand/mt29f_generic.h>
#include <cyg/infra/cyg_ass.h>

#define LOCK(dev)   mt29f_devlock(dev)
#define UNLOCK(dev) mt29f_devunlock(dev)

/* Address encoding logic ====================================== */

static inline void write_addr_row(cyg_nand_device *ctx, cyg_uint32 row)
{
    CYG_BYTE addr[4] = {
         row & 0xff, 
         (row>>8) & 0xff,
         (row>>16) & 0xff,
         0
    };
    mt29f_priv *priv = ctx->priv;
    write_addrbytes(ctx, &addr[0], priv->row_addr_size);
}

/* Helpers ===================================================== */

static inline CYG_BYTE read_status(cyg_nand_device *dev)
{
    write_cmd(dev,0x70);
    HAL_DELAY_US(1); // TODO: Really want a sub-1us delay here. At standard clocking, 3x asm("NOP") does the trick.
    return read_data_1(dev);
}

/* Sends a reset command and waits for the RDY signal (or timeout) before
 * returning.
 * Timeout is tRST:
 *   5us during a Read or idle; 10us during Program; 500us during Erase.
 */
static void mt29f_reset(cyg_nand_device *dev, size_t howlong)
{
    NAND_CHATTER(8,dev, "Resetting device\n");
    write_cmd(dev,0xFF);
    wait_ready_or_time(dev, 1, howlong);
}

/* Entrypoints ================================================= */

int nand_mt29f_devinit(cyg_nand_device *dev)
{
    mt29f_priv *priv = dev->priv;
    CYG_BYTE addr = 0;
    CYG_BYTE id[4];
    int rv = 0;

    // We are protected by the master devinit lock, so we are safe
    // in the knowledge that there's only one of us running right now.
    // (We still need to take out the chip-lock, though, in case there's
    //  another thread trying to access another device which interferes
    //  with us.)

    rv = mt29f_plf_init(dev); // sets up device locking as necessary
    if (rv != ENOERR) return rv; // _not_ ER(), we haven't locked the chip yet

    LOCK(dev); // +++++++++++++++++++++++++++++++++++++++++++++
#define ER(x) do { rv = (x); if (rv != ENOERR) goto err_exit; } while(0)

    mt29f_reset(dev,500); /* We must wait the worst-case reset time - 
                          for the whole supported family */

    write_cmd(dev,0x90);
    write_addrbytes(dev, &addr, 1);
    // wait tWHR.
    read_data_bulk(dev, &id[0], sizeof(id) / sizeof(id[0]));

    NAND_CHATTER(6,dev, "Reading device ID... %02x %02x %02x %02x\n", id[0], id[1], id[2], id[3]);
    if (id[0] != 0x2c) {
        NAND_CHATTER(1,dev, "Unrecognised manufacturer code %02x (expected 2C)\n", id[0]);
        ER(-ENODEV);
    }

    // Now match against the table to find the chip.
    mt29f_subtype * matchlist = priv->supported_chips;
    dev->blockcount_bits = 0;

    while (matchlist && matchlist->descriptor) {
        if (id[1] == matchlist->ident1) {
            if (id[3] == matchlist->ident3) {
                NAND_CHATTER(3,dev, "MICRON %s: %u blocks, total 2^%u bytes, 8-bit data bus\n",
                        matchlist->descriptor,
                        1<<matchlist->blockcount_bits,
                        matchlist->chipsize_log);

                dev->blockcount_bits = matchlist->blockcount_bits;
                dev->chipsize_log = matchlist->chipsize_log;
                priv->row_addr_size = matchlist->row_address_size;
#ifdef CYGSEM_IO_NAND_USE_BBT
                dev->bbt.data = priv->bbt_data;
                dev->bbt.datasize = sizeof(priv->bbt_data);
                // Sanity check: Four blocks take up one byte of RAM in-memory,
                // so subtract two from the log of the blockcount to get the
                // necessary BBT size.
                if (dev->bbt.datasize < (1<<(dev->blockcount_bits-2))) {
                    NAND_ERROR(dev, "In-RAM BBT is not large enough (have %u, need %u)\n", dev->bbt.datasize, 1<<(dev->blockcount_bits-2));
                    ER(-ENODEV);
                }
#endif
                break;
            }
        }
        matchlist++;
    }

    if (!dev->blockcount_bits) {
        NAND_ERROR(dev, "mt29f: no chip driver found for chip with ident %02x %02x %02x %02x\n", id[0], id[1], id[2], id[3]);
        ER(-ENODEV);
    }

    if (matchlist->sp) {
        //dev->page_bits = 9;
    } else {
        dev->page_bits = (id[3] & 3) + 10; // 00 -> 1kbyte; 01 -> 2k.
    }
    int bytes_per_page = 1 << (dev->page_bits);
    CYG_ASSERT(bytes_per_page <= CYGNUM_NAND_PAGEBUFFER, "max pagebuffer not big enough");
    NAND_CHATTER(6,dev, "Page size: %u bytes (2^%u)\n", bytes_per_page, dev->page_bits);

    if (matchlist->sp) {
        //dev->spare_per_page = ;
    } else {
        int spp = (id[3] >> 2) & 3;
        CYG_ASSERT(spp == 1, "unhandled spare area size marker");
        dev->spare_per_page = 64;
        (void) spp;
    }
    NAND_CHATTER(6,dev, "Spare per page: %u bytes\n", dev->spare_per_page);

    size_t log2_blocksize=0;
    if (matchlist->sp) {
        //log2_blocksize = ;
    } else {
        log2_blocksize = 16 /* 64KB */ + ((id[3] >> 4) & 3); // 00 -> 64KB per block, &c.
    }

    dev->block_page_bits = log2_blocksize - dev->page_bits;
    NAND_CHATTER(6,dev, "Pages per block: %u (2^%u)\n", 1<<dev->block_page_bits, dev->block_page_bits);

    if ((!matchlist->sp) && (id[3] & (1<<6))) {
        NAND_ERROR(dev, "NAND Data bus 16 bits is NYI\n");
        ER(-ENODEV);
    }

    if (dev->chipsize_log !=
            dev->page_bits + dev->block_page_bits + dev->blockcount_bits) {
        NAND_ERROR(dev, "NAND device error: Coded chip size (2^%u bytes) does not match computed (2^%u by/pg, 2^%u pg/bl, 2^%u bl/chip)\n", dev->chipsize_log, dev->page_bits, dev->block_page_bits, dev->blockcount_bits);
        ER(-ENODEV);
    }

    // Could read the serial-access timing as well and configure a delay loop?

    ER(mt29f_plf_partition_setup(dev));

err_exit:
    //mt29f_reset(dev,5); // Unnecessary.
    UNLOCK(dev); // ------------------------------------------------
    return rv;
}
#undef ER

int nand_mt29f_erase_block(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    //mt29f_priv *priv = dev->priv;
    unsigned row = blk << (dev->block_page_bits);
    int rv = 0;

    LOCK(dev);
    write_cmd(dev,0x60);
    write_addr_row(dev, row);
    write_cmd(dev,0xd0);
    wait_ready_or_status(dev, 1<<6);
    unsigned stat = read_status(dev);
    if (stat & 1) {
        rv = -EIO;
        NAND_ERROR(dev, "mt29f: Erasing block %u failed.\n",blk);
        goto done;
    }
    NAND_CHATTER(7,dev, "Erased block %u OK\n", blk);
done:
    UNLOCK(dev);
    return rv;
}

//#ifdef CYGFUN_NAND_MICRON_MT29F_SP
//#include "mt29f_generic_sp.inl"
//#endif

#ifdef CYGFUN_NAND_MICRON_MT29F_LP
#include "mt29f_generic_lp.inl"
#endif

