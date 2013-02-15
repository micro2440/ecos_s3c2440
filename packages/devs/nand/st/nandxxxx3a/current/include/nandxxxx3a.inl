//=============================================================================
//
//      nandxxxx3a.inl
//
//      Inline include file for the ST Microelectronics NANDxxxx3A family.
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2009 Free Software Foundation.
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
// Author(s):   Simon Kallweit
// Date:        2009-06-30
// Contributors: wry, for updates to the NAND interface
//
//####DESCRIPTIONEND####
//=============================================================================

#include <pkgconf/io_nand.h>
#include <cyg/devs/nand/nandxxxx3a.h>
#include <cyg/infra/cyg_ass.h>

#define LOCK(dev)   nandxxxx3a_devlock(dev)
#define UNLOCK(dev) nandxxxx3a_devunlock(dev)

/* Helpers ===================================================== */

static inline void write_addr(cyg_nand_device *dev, cyg_nand_page_addr page,
                              cyg_nand_column_addr col)
{
    struct nandxxxx3a_priv *priv = dev->priv;
    
    if (priv->mbit <= 256) {
        CYG_BYTE addr[3] = { col & 0xff, page & 0xff, (page >> 8) & 0xff };
        write_addrbytes(dev, addr, 3);
    } else {
        CYG_BYTE addr[4] = { col & 0xff, page & 0xff, (page >> 8) & 0xff,
                             (page >> 16) & 0xff };
        write_addrbytes(dev, addr, 4);
    }
}                               

static inline CYG_BYTE read_status(cyg_nand_device *dev)
{
    write_cmd(dev, 0x70);
    return read_data_1(dev);
}

/* Sends a reset command and waits for the RDY signal (or timeout) before
 * returning.
 * Timeout is tRST:
 *   5us during a Read or idle; 10us during Program; 500us during Erase.
 */
static void nandxxxx3a_reset(cyg_nand_device *dev, size_t howlong)
{
    NAND_CHATTER(8, dev, "Resetting device\n");
    write_cmd(dev, 0xff);
    wait_ready_or_time(dev, 1, howlong);
}

/* Entrypoints ================================================= */

static int nandxxxx3a_devinit(cyg_nand_device *dev)
{
    struct nandxxxx3a_priv *priv = dev->priv;
    CYG_BYTE addr = 0;
    CYG_BYTE id[2];
    int rv = 0;
    int size_log;
    const char *typ;

    // We are protected by the master devinit lock, so we are safe
    // in the knowledge that there's only one of us running right now.
    // (We still need to take out the chip-lock, though, in case there's
    //  another thread trying to access another device which interferes
    //  with us.)

    rv = nandxxxx3a_plf_init(dev); // Sets up device locking as necessary
    if (rv != ENOERR)
        return rv; // _not_ ER(), we haven't locked the chip yet

    LOCK(dev); // +++++++++++++++++++++++++++++++++++++++++++++
#define ER(x) do { rv = (x); if (rv != ENOERR) goto err_exit; } while(0)

    nandxxxx3a_reset(dev, 500); // We must wait the worst-case reset time

    write_cmd(dev, 0x90);
    write_addrbytes(dev, &addr, 1);
    read_data_bulk(dev, id, sizeof(id));

    NAND_CHATTER(6, dev, "Reading device ID... %02x %02x\n", id[0], id[1]);
    if (id[0] != 0x20) {
        NAND_CHATTER(1, dev, "Unrecognised manufacturer code %02x (expected 20)\n", id[0]);
        ER(-ENODEV);
    }
    
    switch (id[1]) {
    case 0x33: // NAND128R3A
        typ = "128R"; size_log = 24; break;
    case 0x73: // NAND128W3A
        typ = "128W"; size_log = 24; break;
    case 0x35: // NAND256R3A
        typ = "256R"; size_log = 25; break;
    case 0x75: // NAND256W3A
        typ = "256W"; size_log = 25; break;
    case 0x36: // NAND512R3A
        typ = "512R"; size_log = 26; break;
    case 0x76: // NAND512W3A
        typ = "512W"; size_log = 26; break;
    case 0x39: // NAND01GR3A
        typ = "01GR"; size_log = 27; break;
    case 0x79: // NAND01GW3A
        typ = "01GW"; size_log = 27; break;
    default:
        NAND_CHATTER(1, dev, "Unrecognised NAND device\n");
        ER(-ENODEV);
        break;
    }
    
    NAND_CHATTER(3, dev, "ST Microelectronics NAND%s3A: %dMbit, 8-bit data bus\n", typ, 1 << (size_log - 17));
    if (1 << (size_log - 17) != priv->mbit) {
        NAND_ERROR(dev, "NAND device error: Chip size does not match configured size\n");
        ER(-ENODEV);
    }
    
    dev->page_bits = 9;                     // Pages are 512 bytes
    dev->spare_per_page = 16;               // OBB area is 16 bytes
    dev->block_page_bits = 5;               // 32 pages per block 
    dev->blockcount_bits = size_log - 14;
    dev->chipsize_log = size_log;
#ifdef CYGSEM_IO_NAND_USE_BBT
    dev->bbt.data = priv->bbt_data;
    dev->bbt.datasize = priv->mbit * 8;
#endif

    int bytes_per_page = 1 << (dev->page_bits);
    CYG_ASSERT(bytes_per_page <= CYGNUM_NAND_PAGEBUFFER, "max pagebuffer not big enough");
    NAND_CHATTER(6, dev, "Page size: %u bytes (2^%u)\n", bytes_per_page, dev->page_bits);
    NAND_CHATTER(6, dev, "Spare per page: %u bytes\n", dev->spare_per_page);
    NAND_CHATTER(6, dev, "Pages per block: %u (2^%u)\n", 1 << dev->block_page_bits, dev->block_page_bits);

    if (dev->chipsize_log !=
            dev->page_bits + dev->block_page_bits + dev->blockcount_bits) {
        NAND_ERROR(dev, "NAND device error: Coded chip size (2^%u bytes) does not match computed (2^%u by/pg, 2^%u pg/bl, 2^%u bl/chip)\n", dev->chipsize_log, dev->page_bits, dev->block_page_bits, dev->blockcount_bits);
        ER(-ENODEV);
    }

    // Could read the serial-access timing as well and configure a delay loop?

    ER(nandxxxx3a_plf_partition_setup(dev));

err_exit:
    UNLOCK(dev); // ------------------------------------------------
    return rv;
}
#undef ER


static int nandxxxx3a_read_begin(cyg_nand_device *dev, cyg_nand_page_addr page)
{
    struct nandxxxx3a_priv *priv = dev->priv;
    NAND_CHATTER(7, dev, "Reading page %d\n",page);
    LOCK(dev);
    priv->pageop = page;

    write_cmd(dev, 0x00);
    write_addr(dev, priv->pageop, 0);
    wait_ready_or_time(dev, 1, 12);
    return 0;
}

static int nandxxxx3a_read_stride(cyg_nand_device *dev, void *dest, size_t size)
{
    //struct nandxxxx3a_priv *priv = dev->priv;
    read_data_bulk(dev, dest, size);
    return 0;
}

static int nandxxxx3a_read_finish(cyg_nand_device *dev,
                                  void *spare, size_t spare_size)
{
    struct nandxxxx3a_priv *priv = dev->priv;
    if (spare && spare_size) {
        write_cmd(dev, 0x50);
        write_addr(dev, priv->pageop, 0);
        wait_ready_or_time(dev, 1, 12);
        read_data_bulk(dev, spare, spare_size);
    }
    
    UNLOCK(dev);
    return 0;
}

static int nandxxxx3a_read_part(cyg_nand_device *dev, void *dest,
                        cyg_nand_page_addr page, size_t offset, size_t length)
{
    //struct nandxxxx3a_priv *priv = dev->priv;

    NAND_CHATTER(7,dev,"Reading page %d (partial; offset %d, length %d)\n",
                 page, offset, length);

    cyg_nand_column_addr col = offset & 0xff;
    CYG_BYTE cmd = offset & 256 ? 1 /* READ_B */ : 0 /* READ_A */;
    LOCK(dev);

    write_cmd(dev, cmd);
    write_addr(dev, page, col);
    wait_ready_or_time(dev, 1, 12);
    read_data_bulk(dev, dest, length);

    UNLOCK(dev);
    return 0;
}


static int nandxxxx3a_write_begin(cyg_nand_device *dev, cyg_nand_page_addr page)
{
    struct nandxxxx3a_priv *priv = dev->priv;
    LOCK(dev);
    priv->pageop = page;
    priv->written = 0;

    write_cmd(dev, 0x00);
    write_cmd(dev, 0x80);
    write_addr(dev, page, 0);
    return 0;
}

static int nandxxxx3a_write_stride(cyg_nand_device *dev,
                                   const void *src, size_t size)
{
    struct nandxxxx3a_priv *priv = dev->priv;
    write_data_bulk(dev, src, size);
    priv->written += size;
    return 0;
}

static int nandxxxx3a_write_finish(cyg_nand_device *dev, 
                                   const void *spare, size_t spare_size)
{
    struct nandxxxx3a_priv *priv = dev->priv;
    int rv = 0;

    if (priv->written) {
        write_cmd(dev, 0x10);
        wait_ready_or_status(dev, 1 << 6);
        if (read_status(dev) & 1)
            goto fail;
    }

    if (spare && spare_size) {
        write_cmd(dev, 0x50);
        write_cmd(dev, 0x80);
        write_addr(dev, priv->pageop, 0);
        write_data_bulk(dev, spare, spare_size);
        write_cmd(dev, 0x10);
        wait_ready_or_status(dev, 1 << 6);
    }
    
    if (read_status(dev) & 1) {
fail:
        NAND_ERROR(dev, "NAND: Programming page %u failed!", priv->pageop);
        rv = -EIO;
        goto done;
    } else {
        NAND_CHATTER(7, dev, "Programmed %u OK\n", priv->pageop);
    }

done:
    UNLOCK(dev);
    return rv;
}

static int nandxxxx3a_erase_block(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    struct nandxxxx3a_priv *priv = dev->priv;
    cyg_nand_page_addr page = blk << dev->block_page_bits;
    int rv = 0;

    LOCK(dev);

    write_cmd(dev, 0x60);
    if (priv->mbit <= 256) {
        CYG_BYTE addr[2] = { page & 0xff, (page >> 8) & 0xff };
        write_addrbytes(dev, addr, 2);
    } else {
        CYG_BYTE addr[3] = { page & 0xff, (page >> 8) & 0xff, (page >> 16) & 0xff };
        write_addrbytes(dev, addr, 3);
    }
    write_cmd(dev, 0xd0);
    wait_ready_or_status(dev, 1 << 6);
    if (read_status(dev) & 1) {
        rv = -EIO;
        NAND_ERROR(dev, "NAND: Erasing block %u failed.\n", blk);
        goto done;
    }
    NAND_CHATTER(7, dev, "Erased block %u OK\n", blk);
    
done:
    UNLOCK(dev);
    return rv;
}

// The spec sheet says to check the 6th OOB byte (address 6) in the first
// page of the block. If it is 0xFF, the block is OK, otherwise it's bad.
// (N.B. Chips with a 16-bit data bus, check the 1st OOB word.)
#define NANDXXXX3A_FACTORY_BAD_COLUMN_ADDR 6
static int nandxxxx3a_factorybad(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    //struct nandxxxx3a_priv *priv = dev->priv;
    cyg_nand_page_addr page = blk * (1 << dev->block_page_bits);
    const int col = NANDXXXX3A_FACTORY_BAD_COLUMN_ADDR;
    int rv = 0;
    
    LOCK(dev);

    write_cmd(dev, 0x50);
    write_addr(dev, page, col);
    wait_ready_or_time(dev, 1, 12);
    unsigned char t = read_data_1(dev);
    if (t != 0xff)
        rv = 1;

    UNLOCK(dev);
    return rv;
}

CYG_NAND_FUNS_V2(nandxxxx3a_funs, nandxxxx3a_devinit,
                 nandxxxx3a_read_begin, nandxxxx3a_read_stride,
                 nandxxxx3a_read_finish,
                 nandxxxx3a_read_part,
                 nandxxxx3a_write_begin,
                 nandxxxx3a_write_stride, nandxxxx3a_write_finish,
                 nandxxxx3a_erase_block,
                 nandxxxx3a_factorybad);
