//=============================================================================
//
//      k9_generic_lp.inl
//
//      Inline include file for the large page members of the Samsung K9F family
//      NOTE: This file should only be included by k9_generic.inl.
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
// Date:        2010-03-16
//
//####DESCRIPTIONEND####
//=============================================================================

#ifndef CYGFUN_NAND_SAMSUNG_K9_LP
#error k9_generic_lp.inl unexpectedly included
#endif

static inline void write_addr_col_lp(cyg_nand_device *ctx, cyg_nand_column_addr col)
{
    // All supported large-page chips are 8-bit, 2112 bytes per page,
    // column addesses encoded as two bytes.
    CYG_BYTE addr[2] = { col & 0xff, (col>>8) & 0xff };
    write_addrbytes(ctx, &addr[0], 2);
}

static inline void write_addr_col_row_lp(cyg_nand_device *ctx,
        cyg_nand_column_addr c, cyg_uint32 r)
{
    write_addr_col_lp(ctx, c);
    write_addr_row(ctx, r);
}

static inline void change_read_column_lp(cyg_nand_device *dev, cyg_nand_column_addr col)
{
    write_cmd(dev,0x05);
    write_addr_col_lp(dev,col);
    write_cmd(dev,0xe0);
    // wait tREA before reading further. 
    // This is 18ns, less than an instruction.
}

static inline void change_write_column_lp(cyg_nand_device *dev, cyg_nand_column_addr col)
{
    write_cmd(dev,0x85);
    write_addr_col_lp(dev,col);
    // We must wait at least tADL (100ns) before writing further. 
    HAL_DELAY_US(1); // TODO: 1us is a bit wasteful; prefer a tighter sleep duration
}

int nand_k9_read_begin(cyg_nand_device *dev, cyg_nand_page_addr page)
{
    //k9_priv *priv = dev->priv;

    NAND_CHATTER(7,dev,"Reading page %d\n",page);
    LOCK(dev);

    write_cmd(dev,0x00);
    write_addr_col_row_lp(dev, 0, page);
    write_cmd(dev,0x30);
    wait_ready_or_time(dev, 1, 25 /* tR */);
    return 0;
}

int nand_k9_read_stride(cyg_nand_device *dev, void * dest, size_t size)
{
    read_data_bulk(dev, dest, size);
    return 0;
}

int nand_k9_read_finish(cyg_nand_device *dev, void * spare, size_t spare_size)
{

    if (spare && spare_size) {
        change_read_column_lp(dev, 1 << dev->page_bits);
        read_data_bulk(dev, spare, spare_size);
    }
    //k9_reset(dev,5); // Unnecessary.
    UNLOCK(dev);
    return 0;
}

int nand_k9_read_part(cyg_nand_device *dev, void *dest,
                        cyg_nand_page_addr page, size_t offset, size_t length)
{
    //k9_priv *priv = dev->priv;
    NAND_CHATTER(7,dev,"Reading page %d (partial; offset %d, length %d)\n",
                 page, offset, length);
    LOCK(dev);

    write_cmd(dev,0x00);
    write_addr_col_row_lp(dev, offset, page);
    write_cmd(dev,0x30);
    wait_ready_or_time(dev, 1, 25 /* tR */);

    read_data_bulk(dev, dest, length);

    UNLOCK(dev);
    return 0;
}

int nand_k9_write_begin(cyg_nand_device *dev, cyg_nand_page_addr page)
{
    k9_priv *priv = dev->priv;
    // NB: Program with random data input takes a _five_ byte address,
    // according to spec p24. This doesn't make sense - everything else
    // about this chip takes a four-byte address - and is contradicted by p32.

    LOCK(dev);
    priv->pagestash = page;
    write_cmd(dev,0x80);
    write_addr_col_row_lp(dev, 0, page);

    return 0;
}

int nand_k9_write_stride(cyg_nand_device *dev, const void * src, size_t size)
{
    write_data_bulk(dev, src, size);
    return 0;
}

int nand_k9_write_finish(cyg_nand_device *dev, const void * spare, size_t spare_size)
{
    k9_priv *priv = dev->priv;
    int rv = 0;
    if (spare && spare_size) {
        change_write_column_lp(dev, 1 << dev->page_bits);
        write_data_bulk(dev, spare, spare_size);
    }
    write_cmd(dev,0x10);
    wait_ready_or_status(dev, 1<<6);

    if (read_status(dev) & 1) {
        NAND_ERROR(dev, "k9fxx08x0x: Programming failed! Page %u", priv->pagestash);
        rv =-EIO;
        goto done;
    } else {
        NAND_CHATTER(7,dev, "Programmed %u OK\n", priv->pagestash);
    }
done:
    //k9_reset(dev,5); // Unnecessary.
    UNLOCK(dev);
    return rv;
}

#define K9_FACTORY_BAD_COLUMN_ADDR 2048

// The spec sheet says to check the 1st OOB byte (address 2048) in
// both the 1st and 2nd page of the block. If both are 0xFF, the block is OK;
// else it's bad.
// N.B. This is good for at least K9F1G and K9F2G but might need to be tweaked later.
int nand_k9_factorybad_lp(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    cyg_nand_page_addr page,
                       pagebase = blk * (1<<dev->block_page_bits);
    const int col = K9_FACTORY_BAD_COLUMN_ADDR;
    int rv = 0, i;

    LOCK(dev);

    for (i=0; i<2; i++) {
        page = pagebase+i;
        write_cmd(dev,0x00);
        write_addr_col_row_lp(dev, col, page);
        write_cmd(dev,0x30);
        wait_ready_or_time(dev, 1, 25 /* tR */);
        unsigned char t = read_data_1(dev);
        if (t != 0xff) rv=1;
        //k9_reset(dev,5); // Unnecessary.
    }

    UNLOCK(dev);
    return rv;
}

CYG_NAND_FUNS_V2(nand_k9_funs, nand_k9_devinit,
        nand_k9_read_begin, nand_k9_read_stride, nand_k9_read_finish,
        nand_k9_read_part,
        nand_k9_write_begin, nand_k9_write_stride, nand_k9_write_finish,
        nand_k9_erase_block, nand_k9_factorybad_lp);

// key: SUBTYPE( id[1], id[3],   // ReadID response, 2nd and 4th bytes
//              descriptive string,
//              log2(number of blocks),
//              log2(total chip size in BYTES),
//              width of a row address in bytes)

#ifdef CYGFUN_NAND_SAMSUNG_K9_F2G
#define K9F2G08Q0x K9_SUBTYPE(0xaa, 0x15, "K9F2G08Q0x", 11, 28, 3)
#define K9F2G08U0x K9_SUBTYPE(0xda, 0x15, "K9F2G08U0x", 11, 28, 3)
#endif

#ifdef CYGFUN_NAND_SAMSUNG_K9_F1G
#define K9F1G08U0x K9_SUBTYPE(0xf1, 0x95, "K9F1G08U0x", 10, 27, 2)
#endif

// EOF k9_generic_lp.inl

