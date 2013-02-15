//=============================================================================
//
//      k9_generic_sp.inl
//
//      Inline include file for the small page members of the Samsung K9F family
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

#ifndef CYGFUN_NAND_SAMSUNG_K9_SP
#error k9_generic_sp.inl unexpectedly included
#endif

/* Address encoding logic ====================================== */

static inline void write_addr_col_sp(cyg_nand_device *ctx, cyg_nand_column_addr col)
{
    // column addesses encoded as one byte. 
    // Caller is expected to send the correct command ReadA/ReadB/ReadC.
    CYG_BYTE addr[1] = { col & 0xff };
    write_addrbytes(ctx, &addr[0], 1);
}

static inline void write_addr_col_row_sp(cyg_nand_device *ctx,
        cyg_nand_column_addr c, cyg_uint32 r)
{
    write_addr_col_sp(ctx, c);
    write_addr_row(ctx, r);
}


/* Entrypoints ================================================= */

////// Reading       ////////////////////////////////////////////////////

int nand_k9_read_begin_sp(cyg_nand_device *dev, cyg_nand_page_addr page)
{
    k9_priv *priv = dev->priv;

    NAND_CHATTER(7,dev,"Reading page %d\n",page);
    LOCK(dev);

    priv->pagestash = page;

    write_cmd(dev,0x00); // ReadA
    write_addr_col_row_sp(dev, 0, page);
    wait_ready_or_time(dev, 1, 15 /* tR */);
    return 0;
}

int nand_k9_read_stride_sp(cyg_nand_device *dev, void * dest, size_t size)
{
    read_data_bulk(dev, dest, size);
    return 0;
}

int nand_k9_read_finish_sp(cyg_nand_device *dev, void * spare, size_t spare_size)
{
    k9_priv *priv = dev->priv;
    if (spare && spare_size) {
        // Some drivers do funny things for ECC, so we need to explicitly reset ourselves to the spare area.
        write_cmd(dev,0x50); // ReadC
        write_addr_col_row_sp(dev, 0, priv->pagestash);
        wait_ready_or_time(dev, 1, 15 /* tR */);
        read_data_bulk(dev, spare, spare_size);
        // TODO: Determine why a 4us delay is required here.
        // (Symptom: nand_rwbenchmark bulk reads do not read correctly;
        //  its page data is prefixed by six junk bytes.)
        HAL_DELAY_US(4);
    } 
     
    UNLOCK(dev);
    return 0;
}

int nand_k9_read_part_sp(cyg_nand_device *dev, void *dest,
                        cyg_nand_page_addr page, size_t offset, size_t length)
{
    //k9_priv *priv = dev->priv;
    NAND_CHATTER(7,dev,"Reading page %d (partial; offset %d, length %d)\n",
                 page, offset, length);
    LOCK(dev);

    CYG_BYTE cmd = (offset > 255 ? 0x01 : 0x00);
    write_cmd(dev,cmd); // ReadA / ReadB
    write_addr_col_row_sp(dev, offset & 0xff, page);
    wait_ready_or_time(dev, 1, 15 /* tR */);

    read_data_bulk(dev, dest, length);

    UNLOCK(dev);
    return 0;
}

////// Writing       ////////////////////////////////////////////////////

int nand_k9_write_begin_sp(cyg_nand_device *dev, cyg_nand_page_addr page)
{
    k9_priv *priv = dev->priv;

    LOCK(dev);
    priv->pagestash = page;
    write_cmd(dev,0x00);
    write_cmd(dev,0x80);
    write_addr_col_row_sp(dev, 0, page);

    return 0;
}

int nand_k9_write_stride_sp(cyg_nand_device *dev, const void * src, size_t size)
{
    write_data_bulk(dev, src, size);
    return 0;
}

int nand_k9_write_finish_sp(cyg_nand_device *dev, const void * spare, size_t spare_size)
{
    k9_priv *priv = dev->priv;
    int rv = 0;
    if (spare && spare_size) {
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
        (void)priv;
    }
done:
    //k9_reset(dev,5); // Unnecessary.
    UNLOCK(dev);
    return rv;
}

#define K9_FACTORY_BAD_COLUMN_ADDR_SP 517

int nand_k9_factorybad_sp(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    cyg_nand_page_addr page,
                       pagebase = blk * (1<<dev->block_page_bits);
    const int col = K9_FACTORY_BAD_COLUMN_ADDR_SP;
    int rv = 0, i;

    LOCK(dev);

    for (i=0; i<2; i++) {
        page = pagebase+i;
        write_cmd(dev,0x50); // ReadC
        write_addr_col_row_sp(dev, col, page);
        wait_ready_or_time(dev, 1, 15 /* tR */);
        unsigned char t = read_data_1(dev);
        if (t != 0xff) rv=1;
        //k9_reset(dev,5); // Unnecessary.
    }

    UNLOCK(dev);
    return rv;
}

CYG_NAND_FUNS_V2(nand_k9_funs_smallpage, nand_k9_devinit,
        nand_k9_read_begin_sp, nand_k9_read_stride_sp, nand_k9_read_finish_sp,
        nand_k9_read_part_sp,
        nand_k9_write_begin_sp, nand_k9_write_stride_sp, nand_k9_write_finish_sp,
        nand_k9_erase_block, nand_k9_factorybad_sp);

// key: SUBTYPE( id[1], id[3],   // ReadID response, 2nd and 4th bytes
//              descriptive string,
//              log2(number of blocks),
//              log2(total chip size in BYTES),
//              width of a row address in bytes)

#ifdef CYGFUN_NAND_SAMSUNG_K9_F12
#define K9F1208U0x K9_SUBTYPE_SP(0x76, 0xC0, "K9F1208U0x", 12, 26, 3)
#endif

// EOF k9_generic_sp.inl
