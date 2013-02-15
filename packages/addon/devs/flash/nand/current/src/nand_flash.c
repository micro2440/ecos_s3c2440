//==========================================================================
//
//      nand_flash.c
//
//      FLASH memory - Support for NAND flash
//
//==========================================================================
//####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Red Hat, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with eCos; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
// -------------------------------------------
//####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    Alexey Shusharin <mrfinch@mail.ru>
// Contributors: 
// Date:         2007-11-16
// Purpose:      
// Description:  
//              
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/hal.h>
#include <pkgconf/io_flash.h>
#include <pkgconf/devs_flash_nand.h>

#include <cyg/infra/cyg_type.h>

#include <cyg/hal/hal_if.h>
#include <cyg/hal/hal_io.h>

#include <cyg/infra/diag.h>

#include <string.h>

//--------------------------------------------------------------------------
// Flash includes

// This driver supports only one device and only 8-bit
#define CYGNUM_FLASH_INTERLEAVE (1)
#define CYGNUM_FLASH_WIDTH      (8)
#define CYGNUM_FLASH_BLANK      (1)

#define  _FLASH_PRIVATE_
#include <cyg/io/flash.h>
#include <cyg/io/flash_dev.h>
#include <cyg/io/nand_flash.h>

//--------------------------------------------------------------------------
// NAND driver debug level 
//   0 - no debug message
//   1 - only error messages
//   2 - command level messages
//   3 - hardware level messages

#define NAND_DEBUG_LEVEL    3

#if NAND_DEBUG_LEVEL == 3
# define DEBUG_PRINT_ERR    flash_info.pf
# define DEBUG_PRINT_CMD    flash_info.pf
# define DEBUG_PRINT_HDW    flash_info.pf
#elif NAND_DEBUG_LEVEL == 2
# define DEBUG_PRINT_ERR    flash_info.pf
# define DEBUG_PRINT_CMD    flash_info.pf
# define DEBUG_PRINT_HDW(fmt, ...)
#elif NAND_DEBUG_LEVEL == 1
# define DEBUG_PRINT_ERR    flash_info.pf
# define DEBUG_PRINT_CMD(fmt, ...)
# define DEBUG_PRINT_HDW(fmt, ...)
#else
# define DEBUG_PRINT_ERR(fmt, ...)
# define DEBUG_PRINT_CMD(fmt, ...)
# define DEBUG_PRINT_HDW(fmt, ...)
#endif

//--------------------------------------------------------------------------
// NAND data buffer
static cyg_uint8 nand_buffer[CYGNUM_DEVS_FLASH_NAND_BUFFER_SIZE];

#ifdef CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR
// Repair managemment id string
static const char nand_bbm_id[32] = "Repair management table v1.0";
#endif

//--------------------------------------------------------------------------
// Include NAND FLASH parts
#include "nand_flash_parts.inl"

//--------------------------------------------------------------------------
// Include platform NAND FLASH definitions
#include CYGDAT_DEVS_FLASH_NAND_PLF_INL

// These platform functions is not obligatory
#ifndef CYGHWR_FLASH_NAND_PLF_INIT
#define CYGHWR_FLASH_NAND_PLF_INIT()
#endif
#ifndef CYGHWR_FLASH_NAND_PLF_CE
#define CYGHWR_FLASH_NAND_PLF_CE(state)
#endif
#ifndef CYGHWR_FLASH_NAND_PLF_WP
#define CYGHWR_FLASH_NAND_PLF_WP(state)
#endif

//--------------------------------------------------------------------------
// Auxiliary functions
//--------------------------------------------------------------------------

static int nand_ffs(cyg_uint32 data)
{
    int res = 1;
    
    if(!data) return 0;
    
    if(data > 0xFFFF) {res += 16; data >>= 16;}
    if(data > 0xFF) {res += 8; data >>= 8;}
    if(data > 0x0F) {res += 4; data >>= 4;}
    if(data > 0x03) {res += 2; data >>= 2;}
    if(data > 0x01) res += 1;
    
    return res;
}

//--------------------------------------------------------------------------
// Hardware dependent functions
//--------------------------------------------------------------------------

static inline cyg_uint8 nand_read_byte(void)
{
    cyg_uint8 res;
    
    HAL_READ_UINT8(nand_device.addr_r, res);
    
    return res;
}

static inline void nand_read_buf(cyg_uint8* buf, cyg_uint32 len)
{
    cyg_uint32 i;
    
    for(i = 0; i < len; i++)
    {
        HAL_READ_UINT8(nand_device.addr_r, buf[i]);
    }
}

static inline void nand_write_buf(cyg_uint8* buf, cyg_uint32 len)
{
    cyg_uint32 i;
    
    for(i = 0; i < len; i++)
    {
        HAL_WRITE_UINT8(nand_device.addr_w, buf[i]);
    }
}

//--------------------------------------------------------------------------
// ECC functions
//--------------------------------------------------------------------------

#if defined(CYGSEM_DEVS_FLASH_NAND_ECC_SOFT)

static inline int nand_get_parity(int val)
{
    // Calc byte parity
    val ^= (val >> 4);
    val ^= (val >> 2);
    val ^= (val >> 1);
    
    return val & 1;
}

static cyg_uint32 nand_calc_ecc(cyg_uint8* data)
{
    int p_col, p_8, p_16, p_32, p_hi, tmp, block;
    cyg_uint32 ecc;
    int i;
    
    p_col = p_8 = p_16 = p_32 = p_hi = 0;
    
    // Calc bits parity
    for(i = 0; i < 32; i++)
    {
        block = *data++;
        tmp = *data++; block ^= tmp; p_8 ^= tmp;
        tmp = *data++; block ^= tmp;             p_16 ^= tmp;
        tmp = *data++; block ^= tmp; p_8 ^= tmp; p_16 ^= tmp;
        tmp = *data++; block ^= tmp;                          p_32 ^= tmp;
        tmp = *data++; block ^= tmp; p_8 ^= tmp;              p_32 ^= tmp;
        tmp = *data++; block ^= tmp;             p_16 ^= tmp; p_32 ^= tmp;
        tmp = *data++; block ^= tmp; p_8 ^= tmp; p_16 ^= tmp; p_32 ^= tmp;
        
        p_col ^= block;
        if(nand_get_parity(block)) p_hi ^= i;
    }
    
    // Get ecc from parity values
    ecc = p_hi << 6;                            // p_64 - p_1024
    ecc |= nand_get_parity(p_32) << 5;          // p_32
    ecc |= nand_get_parity(p_16) << 4;          // p_16
    ecc |= nand_get_parity(p_8) << 3;           // p_8
    ecc |= nand_get_parity(p_col & 0xF0) << 2;  // p_4
    ecc |= nand_get_parity(p_col & 0xCC) << 1;  // p_2
    ecc |= nand_get_parity(p_col & 0xAA);       // p_1
    
    // If parity of whole array is even, first and second ecc parts are equal
    // If it's odd, second part is equal to inverse of first part
    if(nand_get_parity(p_col)) ecc |= (ecc ^ 0x7FF) << 11;
    else ecc |= ecc << 11;
    
    return ecc;
}

static int nand_check_ecc(cyg_uint8* data, cyg_uint32 ecc)
{
    cyg_uint32 err;
    int err_cnt = 0;
    
    // Calc ecc and get error info
    ecc = (ecc & 0x3FFFFF) ^ nand_calc_ecc(data);
    
    // Count errors
    for(err = ecc; err; err >>= 1) err_cnt += err & 1;
    
    // Check errors
    if(err_cnt < 2)
    {
        // No error or 1 bit error in ecc area => nothing to repair
        return 1;
    }
    else if(err_cnt == 11)
    {
        // This is 1 bit error in data area => repair this bit
        data[(ecc & 0x7FF) >> 3] ^= (1 << (ecc & 7));
        
        return 1;
    }
    
    return 0;
}

#endif

//--------------------------------------------------------------------------
// NAND command
//--------------------------------------------------------------------------

static void nand_command(int cmd, cyg_int32 column, cyg_int32 page_addr)
{
    if(cmd == NAND_CMD_SEQIN)
    {
        // Set data pointer to page beginning
        CYGHWR_FLASH_NAND_PLF_CMD(NAND_CMD_READ0);
    }
    
    // Write NAND command
    CYGHWR_FLASH_NAND_PLF_CMD(cmd);
    
    // Write column number if needed
    if(column >= 0)
    {
        CYGHWR_FLASH_NAND_PLF_ADDR(column);
    }
    
    // Write page address if needed
    if(page_addr >= 0)
    {
		CYGHWR_FLASH_NAND_PLF_ADDR(page_addr & 0xFF); 
		CYGHWR_FLASH_NAND_PLF_ADDR((page_addr >> 11) & 0xFF); 
		CYGHWR_FLASH_NAND_PLF_ADDR((page_addr >> 19) & 0xFF);
		CYGHWR_FLASH_NAND_PLF_ADDR((page_addr >> 27) & 0xFF);	
    }
    
    if(cmd == NAND_CMD_ERASE)
    {
        CYGHWR_FLASH_NAND_PLF_CMD(NAND_CMD_ERASE2);
    }
    
    // Delay after command and address
    CYGACC_CALL_IF_DELAY_US(nand_device.delay_cmd);
}

//--------------------------------------------------------------------------
// NAND erase block
//--------------------------------------------------------------------------

static int nand_erase_block(cyg_int32 page_addr)
{
    int status, res = FLASH_ERR_OK;
    
    DEBUG_PRINT_HDW("Erase block %p - ", page_addr);
    
    CYGHWR_FLASH_NAND_PLF_CE(1);
    CYGHWR_FLASH_NAND_PLF_WP(0);
    
    // Erase block
    nand_command(NAND_CMD_ERASE, -1, page_addr);
    CYGHWR_FLASH_NAND_PLF_WAIT();
    
    // Read operation status and check it
    nand_command(NAND_CMD_STATUS, -1, -1);
    status = nand_read_byte();
    
    CYGHWR_FLASH_NAND_PLF_WP(1);
    CYGHWR_FLASH_NAND_PLF_CE(0);
    
    if(!(status & NAND_STATUS_READY)) res = FLASH_ERR_DRV_TIMEOUT;
    else if(!(status & NAND_STATUS_NOWP)) res = FLASH_ERR_PROTECT;
    else if(status & NAND_STATUS_FAIL) res = FLASH_ERR_ERASE;
    
    if(res == FLASH_ERR_OK) DEBUG_PRINT_HDW("Succeed\n");
    else DEBUG_PRINT_HDW("Failed (status = 0x%X)\n", status);
    
    return res;
}

//--------------------------------------------------------------------------
// NAND read page
//--------------------------------------------------------------------------

static int nand_read_page(cyg_int32 page_addr, cyg_uint8* data,
        cyg_uint8* spare)
{
    cyg_nand_dev* dev = &nand_device;
    
    DEBUG_PRINT_HDW("Read page %p - ", page_addr);
    
    CYGHWR_FLASH_NAND_PLF_CE(1);
    
    // Write Read0 command and address
    nand_command(NAND_CMD_READ0, 0x00, page_addr);
    
    // Wait device ready pin
    CYGHWR_FLASH_NAND_PLF_WAIT();
    
    // Read data and spare fields
    nand_read_buf(data, dev->page_size);
    nand_read_buf(spare, dev->spare_size);
    
    CYGHWR_FLASH_NAND_PLF_CE(0);

#if 0
#if defined(CYGSEM_DEVS_FLASH_NAND_ECC_SOFT)
    
    // Don't look at the result of nand_check_ecc(),
    // we should read the page in any case
    if(dev->page_size == 256)
    {
        nand_check_ecc(data, spare[0] | (spare[1] << 8) | (spare[2] << 16));
    }
    else if(dev->page_size == 512)
    {
        nand_check_ecc(data, spare[0] | (spare[1] << 8) | (spare[2] << 16));
        nand_check_ecc(data + 256, spare[3] | (spare[6] << 8) | (spare[7] << 16));
    }
    else
    {
        DEBUG_PRINT_HDW("Unknown page size\n");
        return FLASH_ERR_HWR;
    }
    
#endif
#endif
    
    DEBUG_PRINT_HDW("Succeed\n");
    
    return FLASH_ERR_OK;
}

//--------------------------------------------------------------------------
// NAND write page
//--------------------------------------------------------------------------

static int nand_write_page(cyg_int32 page_addr, cyg_uint8* data,
        cyg_uint8* spare)
{
    cyg_nand_dev* dev = &nand_device;
    int status, res = FLASH_ERR_OK;
    
#if defined(CYGSEM_DEVS_FLASH_NAND_ECC_SOFT)
    cyg_uint32 ecc;
#endif
    
    DEBUG_PRINT_HDW("Program page %p - ", page_addr);
    
    // Fill spare area
    memset(spare, 0xFF, dev->spare_size);
	
#if 0
#if defined(CYGSEM_DEVS_FLASH_NAND_ECC_SOFT)
    
    if(dev->page_size == 256)
    {
        ecc = nand_calc_ecc(data);
        spare[0] = ecc & 0xFF;
        spare[1] = (ecc >> 8) & 0xFF;
        spare[2] = (ecc >> 16) & 0xFF;
    }
    else if(dev->page_size == 512)
    {
        ecc = nand_calc_ecc(data);
        spare[0] = ecc & 0xFF;
        spare[1] = (ecc >> 8) & 0xFF;
        spare[2] = (ecc >> 16) & 0xFF;
        
        ecc = nand_calc_ecc(data + 256);
        spare[3] = ecc & 0xFF;
        spare[6] = (ecc >> 8) & 0xFF;
        spare[7] = (ecc >> 16) & 0xFF;
    }
    else
    {
        DEBUG_PRINT_HDW("Unknown page size\n");
        return FLASH_ERR_HWR;
    }
    
#endif
#endif
    
    CYGHWR_FLASH_NAND_PLF_CE(1);
    CYGHWR_FLASH_NAND_PLF_WP(0);
    
    // Write data and spare fields
    nand_command(NAND_CMD_SEQIN, 0x00, page_addr);
    nand_write_buf(data, dev->page_size);
    nand_write_buf(spare, dev->spare_size);
    
    // Programm page
    nand_command(NAND_CMD_PAGEPROG, -1, -1);
    CYGHWR_FLASH_NAND_PLF_WAIT();
    
    // Read operation status and check it
    nand_command(NAND_CMD_STATUS, -1, -1);
    status = nand_read_byte();
    
    CYGHWR_FLASH_NAND_PLF_WP(1);
    CYGHWR_FLASH_NAND_PLF_CE(0);
    
    if(!(status & NAND_STATUS_READY)) res = FLASH_ERR_DRV_TIMEOUT;
    else if(!(status & NAND_STATUS_NOWP)) res = FLASH_ERR_PROTECT;
    else if(status & NAND_STATUS_FAIL) res = FLASH_ERR_PROGRAM;
    
    if(res == FLASH_ERR_OK) DEBUG_PRINT_HDW("Succeed\n");
    else DEBUG_PRINT_HDW("Failed (status = 0x%X)\n", status);
    
    return res;
}

//--------------------------------------------------------------------------
// Check block
//--------------------------------------------------------------------------

static inline int nand_bbm_isbad(cyg_int32 block)
{
    return nand_device.buf_bbt[block >> 3] & (1 << (block & 0x07));
}

//--------------------------------------------------------------------------
// Mark block as bad
//--------------------------------------------------------------------------

static inline void nand_bbm_markbad(cyg_int32 block)
{
    nand_device.buf_bbt[block >> 3] |= 1 << (block & 0x07);
}

//--------------------------------------------------------------------------
// Check manufacturer bad block marker
//--------------------------------------------------------------------------

static int nand_bbm_checkmarker(cyg_int32 block)
{
    cyg_nand_dev* dev = &nand_device;
    cyg_int32 page_addr;
    int res = 0;
    
    dev->pageinbuf = -1;
    page_addr = block << dev->block_shift;
    
    // Read first page spare area
    CYGHWR_FLASH_NAND_PLF_CE(1);
    
    nand_command(NAND_CMD_READSPARE, 0x00, page_addr);
    CYGHWR_FLASH_NAND_PLF_WAIT();
    nand_read_buf(dev->buf_spare, dev->spare_size);
    
    CYGHWR_FLASH_NAND_PLF_CE(0);
    
    if(dev->buf_spare[0x05] != 0xFF) res = 1;
    else
    {
        // Read second page spare area
        CYGHWR_FLASH_NAND_PLF_CE(1);
        
        nand_command(NAND_CMD_READSPARE, 0x00, page_addr + 1);
        CYGHWR_FLASH_NAND_PLF_WAIT();
        nand_read_buf(dev->buf_spare, dev->spare_size);
        
        CYGHWR_FLASH_NAND_PLF_CE(0);
        
        if(dev->buf_spare[0x05] != 0xFF) res = 1;
    }
    
    return res;
}

//--------------------------------------------------------------------------
// Load bad block management info
// TODO: Work with different endian format
//--------------------------------------------------------------------------

#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)

static int nand_bbm_load(cyg_int32 block)
{
    cyg_nand_dev* dev = &nand_device;
    cyg_int32 page_addr;
    cyg_uint32 info[2];
    cyg_uint32 i, len, size;
    int res;
    
    DEBUG_PRINT_CMD("Load bbm info from %d block\n", block);
    
    dev->pageinbuf = -1;
    page_addr = block << dev->block_shift;
    
    // Read first page in this block
    res = nand_read_page(page_addr++, dev->buf_data, dev->buf_spare);
    if(res != FLASH_ERR_OK)
    {
        DEBUG_PRINT_CMD("Load bbm: Page read error\n");
        return res;
    }
    
    memcpy(info, &dev->buf_data[sizeof(nand_bbm_id)], sizeof(info));
    
    // Check id string, device id and repair area size
    if(strncmp((char*) dev->buf_data, nand_bbm_id, sizeof(nand_bbm_id)) != 0 || 
            info[0] != dev->id || info[1] != NAND_REPAIR_SIZE)
    {
        DEBUG_PRINT_CMD("Load bbm: This is not bbm info block\n");
        return FLASH_ERR_HWR;
    }
    
    // We found management info => Read remap table (next pages)
    i = 0; len = NAND_REPAIR_SIZE * sizeof(cyg_int32);
    while(i < len)
    {
        size  = len - i;
        if(size > dev->page_size) size = dev->page_size;
        
        res = nand_read_page(page_addr++, dev->buf_data, dev->buf_spare);
        if(res != FLASH_ERR_OK)
        {
            DEBUG_PRINT_CMD("Load bbm: Page read error\n");
            return res;
        }
        
        memcpy(((cyg_uint8*) dev->bbm_remap) + i, dev->buf_data, size);
        i += size;
    }
    
    DEBUG_PRINT_CMD("Load bbm: Succeed\n");
    return FLASH_ERR_OK;
}

#endif /*CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR*/

//--------------------------------------------------------------------------
// Save bad block management info
// TODO: Work with different endian format
//--------------------------------------------------------------------------

#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)

static int nand_bbm_save(cyg_int32 block)
{
    cyg_nand_dev* dev = &nand_device;
    cyg_int32 page_addr;
    cyg_uint32 info[2];
    cyg_uint32 i, len, size;
    int res;
    
    DEBUG_PRINT_CMD("Save bbm info to %d block\n", block);
    
    dev->pageinbuf = -1;
    page_addr = block << dev->block_shift;
    
    // Erase block
    res = nand_erase_block(page_addr);
    if(res != FLASH_ERR_OK)
    {
        DEBUG_PRINT_CMD("Save bbm: Erase error\n");
        return res;
    }
    
    // Copy id string, device id and repair area size
    info[0] = dev->id; info[1] = NAND_REPAIR_SIZE;
    memcpy(&dev->buf_data[0], nand_bbm_id, sizeof(nand_bbm_id));
    memcpy(&dev->buf_data[sizeof(nand_bbm_id)], info, sizeof(info));
    
    res = nand_write_page(page_addr++, dev->buf_data, dev->buf_spare);
    if(res != FLASH_ERR_OK)
    {
        DEBUG_PRINT_CMD("Save bbm: Page program error\n");
        return res;
    }
    
    // Save remap table (next pages)
    i = 0; len = NAND_REPAIR_SIZE * sizeof(cyg_int32);
    while(i < len)
    {
        size  = len - i;
        if(size > dev->page_size) size = dev->page_size;
        
        memcpy(dev->buf_data, ((cyg_uint8*) dev->bbm_remap) + i, size);
        memset(&dev->buf_data[size], 0xFF, dev->page_size - size);
        
        res = nand_write_page(page_addr++, dev->buf_data, dev->buf_spare);
        if(res != FLASH_ERR_OK)
        {
            DEBUG_PRINT_CMD("Save bbm: Page program error\n");
            return res;
        }
        
        i += size;
    }
    
    DEBUG_PRINT_CMD("Save bbm: Succeed\n");
    return FLASH_ERR_OK;
}

#endif /*CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR*/

//--------------------------------------------------------------------------
// Remap bad block
//--------------------------------------------------------------------------

#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)

static cyg_int32 nand_bbm_remap(cyg_int32 block)
{
    cyg_nand_dev* dev = &nand_device;
    cyg_int32 first, virtual;
    int i;
    
    DEBUG_PRINT_CMD("Remap %d block\n", block);
    
    // Mark block as bad in bad block table
    nand_bbm_markbad(block);
    
    // Number of first block in repair area
    first = dev->blocks_count - NAND_REPAIR_SIZE;
    
    // Bad block in repair area?
    if(block >= first)
    {
        // Get virtual block number and mark block as bad in remap table 
        virtual = dev->bbm_remap[block - first];
        dev->bbm_remap[block - first] = block;
    }
    else virtual = block;
    
    // Find free block in remap area
    for(i = 0; i < NAND_REPAIR_SIZE; i++)
        if(dev->bbm_remap[i] < 0) break;
    
    if(i >= NAND_REPAIR_SIZE)
    {
        DEBUG_PRINT_CMD("Remap: No free blocks in repair area\n");
        return -1;
    }
    
    // Remap bad block
    dev->bbm_remap[i] = virtual;
    
    // Return repair block number
    block = first + i;
    
    DEBUG_PRINT_CMD("Remap: Remapped to %d block\n", block);
    return block;
}

#endif /*CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR*/

//--------------------------------------------------------------------------
// Init bad block management data
//--------------------------------------------------------------------------

static int nand_bbm_init(void)
{
    cyg_nand_dev* dev = &nand_device;
    
#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_INIT)
    
    cyg_int32 block;
    int bbcount = 0;
    
    DEBUG_PRINT_CMD("Scan manufacture marked bad blocks\n");
    
    // Mark all blocks as good
    memset(dev->buf_bbt, 0x00, dev->blocks_count / 8);
    
    // Scan manufacture marked bad blocks
    for(block = 0; block < dev->blocks_count; block++)
        if(nand_bbm_checkmarker(block))
        {
            nand_bbm_markbad(block);
            bbcount++;
        }
    
    DEBUG_PRINT_CMD("Scan: found %d bad blocks\n", bbcount);
    
    return FLASH_ERR_OK;
    
#elif defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)
    
    cyg_int32 block, first;
    int i, res;
    
    DEBUG_PRINT_CMD("Init bbm info\n");
    
    // Mark all blocks as good
    memset(dev->buf_bbt, 0x00, dev->blocks_count / 8);
    
    // Find last not bad block in the flash
    dev->bbm_info = dev->blocks_count - 1;
    for(i = 0; i < NAND_REPAIR_SIZE; i++, dev->bbm_info--)
        if(!nand_bbm_checkmarker(dev->bbm_info)) break;
    
    // If there are no good blocks => return false
    if(i >= NAND_REPAIR_SIZE)
    {
        DEBUG_PRINT_CMD("Init bbm: No good blocks in repair area\n");
        return FLASH_ERR_HWR;
    }
    
    first = dev->blocks_count - NAND_REPAIR_SIZE;
    
    // Try to load bad block management info
    if(nand_bbm_load(dev->bbm_info) == FLASH_ERR_OK)
    {
        // Restore bad block table from remap table:
        // If block in remap area is free, it is less than 0
        // else it contains bad block number which it remaps
        // If this block is bad, it contains its own number
        for(i = 0; i < NAND_REPAIR_SIZE; i++)
            if(dev->bbm_remap[i] >= 0)
            {
                DEBUG_PRINT_CMD("Remap table: Block %d is remapped to %d\n",
                        dev->bbm_remap[i], first + i);
                nand_bbm_markbad(dev->bbm_remap[i]);
            }
        
        DEBUG_PRINT_CMD("Init bbm: Succeed\n");
        return FLASH_ERR_OK;
    }
    
    // Load failed, try to create bad block management info
    DEBUG_PRINT_CMD("Init bbm: Load failed. Try to create bbm info.\n");
    
    // Check blocks in repair area
    block = first;
    for(i = 0; i < NAND_REPAIR_SIZE; i++, block++)
    {
        if(!nand_bbm_checkmarker(block)) dev->bbm_remap[i] = -1;
        else dev->bbm_remap[i] = block;
    }
    
    // Mark block with management info as bad
    dev->bbm_remap[dev->bbm_info - first] = dev->bbm_info;
        
    // Scan bad blocks and remap its
    for(block = 0; block < first; block++)
        if(nand_bbm_checkmarker(block))
            if(nand_bbm_remap(block) < 0)
            {
                DEBUG_PRINT_CMD("Init bbm: Repair area is too small\n");
                return FLASH_ERR_HWR;
            }
    
    // Save bad block management info
    res = nand_bbm_save(dev->bbm_info);
    
    if(res == FLASH_ERR_OK) DEBUG_PRINT_CMD("Init bbm: Succeed\n");
    else DEBUG_PRINT_CMD("Init bbm: Cannot save bbm info\n");
    
    return res;
    
#else
    #error Unknown bad block management type
#endif
}

//--------------------------------------------------------------------------
// Get real page number (or return -1 if there are no such page)
//--------------------------------------------------------------------------

static cyg_int32 nand_bbm_getpage(cyg_int32 page_addr)
{
    cyg_nand_dev* dev = &nand_device;
    
#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_INIT)
    
    cyg_int32 block;
    
    // If there are no such block or it's bad => return -1
    block = page_addr >> dev->block_shift;
    if((block >= dev->blocks_count) || nand_bbm_isbad(block)) return -1;
    
    return page_addr;
    
#elif defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)
    
    int i;
    cyg_int32 block, first;
    
    // First block in repair area
    first = dev->blocks_count - NAND_REPAIR_SIZE;
    
    // If there are no such block => return -1
    block = page_addr >> dev->block_shift;
    if(block >= first) return -1;
    
    // If block is not bad return initial page address
    if(!nand_bbm_isbad(block)) return page_addr;
    
    // Block is bad => look up to remap table
    for(i = 0; i < NAND_REPAIR_SIZE; i++)
        if(dev->bbm_remap[i] == block)
            return ((first + i) << dev->block_shift) |
                (page_addr & (dev->pages_count - 1));
    
    // This bad block is not remapped
    return -1;
    
#else
    #error Unknown bad block management type
#endif
}

//--------------------------------------------------------------------------
// Erase failed handler
//--------------------------------------------------------------------------

static cyg_int32 nand_bbm_erasefailed(cyg_int32 page_addr)
{
    cyg_nand_dev* dev = &nand_device;
    
#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_INIT)
    
    DEBUG_PRINT_CMD("Erase fail handler\n");
    
    // Mark this block as bad
    nand_bbm_markbad(page_addr >> dev->block_shift);
    return -1;
    
#elif defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)
    
    cyg_int32 bad, replacer;
    int res;
    
    DEBUG_PRINT_CMD("Erase fail handler\n");
    
    // Get bad block
    replacer = bad = page_addr >> dev->block_shift;
    
    // Do while there are blocks in repair area
    while((replacer = nand_bbm_remap(replacer)) >= 0)
    {
        // Try to erase gotten block
        res = nand_erase_block(replacer << dev->block_shift);
        if(res == FLASH_ERR_ERASE) continue;
        else if(res != FLASH_ERR_OK) return -1;
        
        // Save changed bad block managenent info
        nand_bbm_save(dev->bbm_info);
        
        // Return new page address
        return (replacer << dev->block_shift) | 
            (page_addr & (dev->pages_count - 1));
    }
    
    // There are no blocks in repair area
    // Don't save bbm info, because it could be caused from hardware error
    return -1;
    
#else
    #error Unknown bad block management type
#endif
}

//--------------------------------------------------------------------------
// Program failed handler
//--------------------------------------------------------------------------

static cyg_int32 nand_bbm_progfailed(cyg_int32 page_addr)
{
    cyg_nand_dev* dev = &nand_device;
    
#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_INIT)
    
    DEBUG_PRINT_CMD("Program fail handler\n");
    
    // Mark this block as bad
    nand_bbm_markbad(page_addr >> dev->block_shift);
    return -1;
    
#elif defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)
    
    cyg_int32 bad, replacer;
    cyg_int32 page_err, page_i;
    cyg_uint32 i;
    int res;
    
    DEBUG_PRINT_CMD("Program fail handler\n");
    
    // Get bad block
    replacer = bad = page_addr >> dev->block_shift;
    
    // Do while there are blocks in repair area
    while((replacer = nand_bbm_remap(replacer)) >= 0)
    {
        // Try to erase gotten block
        res = nand_erase_block(replacer << dev->block_shift);
        if(res == FLASH_ERR_ERASE) continue;
        else if(res != FLASH_ERR_OK) return -1;
        
        // Reset page buffer
        dev->pageinbuf = -1;
        
        // Try to copy all not 0xFF pages to gotten block
        page_err = page_addr & (dev->pages_count - 1);
        for(page_i = 0; page_i < dev->pages_count; page_i++)
        {
            // Don't copy error page
            if(page_i == page_err) continue;
            
            // Read next page from bad block. If error => skip the page
            res = nand_read_page((bad << dev->block_shift) | page_i,
                    dev->buf_data, dev->buf_spare);
            if(res != FLASH_ERR_OK) continue;
            
            // Check page content. If all bytes are 0xFF => skip the page
            for(i = 0; i < dev->page_size; i++)
                if(dev->buf_data[i] != 0xFF) break;
            
            if(i >= dev->page_size) continue;
            
            // Write page to replace block. If error => return error
            res = nand_write_page((replacer << dev->block_shift) | page_i,
                    dev->buf_data, dev->buf_spare);
            if(res != FLASH_ERR_OK) return -1;
        }
        
        // Save changed bad block managenent info
        nand_bbm_save(dev->bbm_info);
        
        // Return new page address
        return (replacer << dev->block_shift) | page_err;
    }
    
    // There are no blocks in repair area
    // Don't save bbm info, because it could be caused from hardware error
    return -1;
    
#else
    #error Unknown bad block management type
#endif
}

//--------------------------------------------------------------------------
// Flash Query
//--------------------------------------------------------------------------

void flash_query(void* data)
{
    cyg_uint32* ids = (cyg_uint32*) data;
    
    DEBUG_PRINT_CMD("Query NAND flash ids\n");
    
    CYGHWR_FLASH_NAND_PLF_CE(1);
    
    // TODO: Reset flash
    
    
    // Get manuf and device ids
    nand_command(NAND_CMD_READID, 0x00, -1);
    ids[0] = nand_read_byte();
    ids[1] = nand_read_byte();
    
    CYGHWR_FLASH_NAND_PLF_CE(0);
    
    DEBUG_PRINT_CMD("Query: Manuf ID = 0x%X, Device ID = 0x%X\n", ids[0], ids[1]);
}

//--------------------------------------------------------------------------
// Initialize driver
//--------------------------------------------------------------------------

int flash_hwr_init(void)
{
    cyg_nand_dev* dev = &nand_device;
    int i, buflen, total;
    cyg_uint32 ids[2];
    
    DEBUG_PRINT_CMD("Initialize NAND flash\n");
    
    // Call platform nand init function
    CYGHWR_FLASH_NAND_PLF_INIT();
    
    // Get flash ids
    flash_query((void*) ids);
    
    // Look up supported device
    for(i = 0; i < NAND_SUPPORTED_COUNT; i++)
        if(nand_supported[i].id == ids[1]) break;
    
    // if we didn't find the device return error
    if(i >= NAND_SUPPORTED_COUNT)
    {
        DEBUG_PRINT_ERR("NAND_ERR: driver doesn't support "
                "0x%X device ID\n", ids[1]);
        return FLASH_ERR_DRV_WRONG_PART;
    }
    
    // Copy device info
    dev->id = ids[1];
    dev->page_size = nand_supported[i].page_size;
    dev->spare_size = dev->page_size / 32;
    dev->pages_count = nand_supported[i].pages_count;
    dev->blocks_count = nand_supported[i].blocks_count;
    
    dev->page_shift = nand_ffs(dev->page_size - 1);
    dev->block_shift = nand_ffs(dev->pages_count - 1);
    
    // Check data buffer length
    buflen = dev->page_size + dev->spare_size + dev->blocks_count / 8;
    if(buflen > sizeof(nand_buffer))
    {
        DEBUG_PRINT_ERR("NAND_ERR: driver doesn't have enough buffer. "
                "This flash needs at least %d bytes buffer\n", buflen);
        return FLASH_ERR_DRV_WRONG_PART;
    }
    
    // Allocate buffers
    dev->buf_data = &nand_buffer[0];
    dev->buf_spare = &nand_buffer[dev->page_size];
    dev->buf_bbt = &nand_buffer[dev->page_size + dev->spare_size];
    
    // No page in buffer
    dev->pageinbuf = -1;
    
    // Fill in device details
#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_INIT)
    flash_info.blocks = dev->blocks_count;
#elif defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)
    flash_info.blocks = dev->blocks_count - NAND_REPAIR_SIZE;
#else
    #error Unknown bad block management type
#endif
    
    flash_info.block_size = dev->page_size * dev->pages_count;
    total = flash_info.block_size * flash_info.blocks;
    flash_info.start = dev->flash_base;
    flash_info.end = (void *)(((unsigned int) dev->flash_base) + total);
    
    // Init bad block management
   // if(nand_bbm_init() != FLASH_ERR_OK)
    //{
        //DEBUG_PRINT_ERR("NAND_ERR: Bad block management "
             //   "initialisation is failed\n");
        //return FLASH_ERR_HWR;
    //}
    
    DEBUG_PRINT_CMD("Init NAND: Succeed (found %dMB flash)\n",
            flash_info.block_size * dev->blocks_count / 0x100000);
    return FLASH_ERR_OK;
}

//--------------------------------------------------------------------------
// Erase Block
//--------------------------------------------------------------------------

int flash_erase_block(void* block, unsigned int size)
{
    cyg_nand_dev* dev = &nand_device;
    cyg_int32 flash_addr, page_addr;
    int res;
    
    DEBUG_PRINT_CMD("Erase %d bytes from %p\n", size, block);
    
    // Get flash addresses
    flash_addr = (unsigned int) block - (unsigned int) dev->flash_base;
    page_addr = nand_bbm_getpage(flash_addr >> dev->page_shift);
    if(page_addr < 0) return FLASH_ERR_INVALID;
    
    res = nand_erase_block(page_addr);
    if(res != FLASH_ERR_ERASE) return res;
    
    // Handle bad block error
    if(nand_bbm_erasefailed(page_addr) < 0) return res;
    
    return FLASH_ERR_OK;
}

//--------------------------------------------------------------------------
// Program Buffer
//--------------------------------------------------------------------------

int flash_program_buf(void* addr, void* data, int len)
{
    cyg_nand_dev* dev = &nand_device;
    cyg_int32 flash_addr, column, page_addr;
    int count, res;
    
    DEBUG_PRINT_CMD("Program %d bytes to %p\n", len, addr);
    
    // Get flash addresses
    flash_addr = (unsigned int) addr - (unsigned int) dev->flash_base;
    column = flash_addr & (dev->page_size - 1);
    page_addr = nand_bbm_getpage(flash_addr >> dev->page_shift);
    if(page_addr < 0) return FLASH_ERR_INVALID;
    
    while(len > 0)
    {
        // Should we program the whole page?
        if(column == 0 && len >= dev->page_size)
        {
            // Reset buffer if we program the same page
            if(page_addr == dev->pageinbuf) dev->pageinbuf = -1;
            
            // Write from user buffer, skip unnecessary copying
            // It is useful then we are writing a large block of data
            count = dev->page_size;
            res = nand_write_page(page_addr, data, dev->buf_spare);
        }
        else
        {
            // Reset buffer
            dev->pageinbuf = -1;
            
            count = dev->page_size - column;
            if(count > len) count = len;
            
            // Copy data to the device buffer
            memset(dev->buf_data, 0xFF, dev->page_size);
            memcpy(&dev->buf_data[column], data, count);
            
            // Write page
            res = nand_write_page(page_addr, dev->buf_data, dev->buf_spare);
        }
        
        // Check program result
        if(res == FLASH_ERR_PROGRAM)
        {
            // Handle bad block error
            page_addr = nand_bbm_progfailed(page_addr);
            if(page_addr < 0) return FLASH_ERR_PROGRAM;
            
            // Try program this data to new page
            // Other pages in the block has been copied by nand_bbm_progfailed()
            continue;
        }
        else if(res != FLASH_ERR_OK) return res;
        
        // Next page
        len -= count; page_addr++; column = 0;
        data = (void*) ((cyg_uint8*) data + count);
    }
    
    return FLASH_ERR_OK;
}

//--------------------------------------------------------------------------
// Read data into buffer
//--------------------------------------------------------------------------

int flash_read_buf(void* addr, void* data, int len)
{
    cyg_nand_dev* dev = &nand_device;
    cyg_int32 flash_addr, column, page_addr;
    int count, res;
    
    DEBUG_PRINT_CMD("Read %d bytes from %p\n", len, addr);
    
    // Get flash addresses
    flash_addr = (unsigned int) addr - (unsigned int) dev->flash_base;
    column = flash_addr & (dev->page_size - 1);
    page_addr = nand_bbm_getpage(flash_addr >> dev->page_shift);
    if(page_addr < 0) return FLASH_ERR_INVALID;
    
    while(len > 0)
    {
        if(page_addr != dev->pageinbuf && column == 0 && len >= dev->page_size)
        {
            // We should read the whole page not from buffer
            // Read to user buffer, skip unnecessary copying
            // It is useful then we are reading a large block of data
            res = nand_read_page(page_addr, data, dev->buf_spare);
            if(res != FLASH_ERR_OK) return res;
            
            count = dev->page_size;
        }
        else
        {
            if(page_addr != dev->pageinbuf)
            {
                // Read page to the device buffer
                res = nand_read_page(page_addr, dev->buf_data, dev->buf_spare);
                if(res != FLASH_ERR_OK) return res;
                
                dev->pageinbuf = page_addr;
            }
            
            count = dev->page_size - column;
            if(count > len) count = len;
            
            // Copy data from device buffer to user buffer
            memcpy(data, &dev->buf_data[column], count);
            
            // Next read operation start in beginning of page
            column = 0;
        }
        
        len -= count; page_addr++;
        data = (void*) ((cyg_uint8*) data + count);
    }
    
    return FLASH_ERR_OK;
}

//--------------------------------------------------------------------------
// Format flash
//--------------------------------------------------------------------------

int flash_format(int all)
{
    cyg_nand_dev* dev = &nand_device;
    cyg_int32 i;
    
    if(all)
    {
        DEBUG_PRINT_CMD("Format NAND Flash. Erase all blocks.\n");
        
        // Erase all blocks except manufacture marked bad blocks
        for(i = 0; i < dev->blocks_count; i++)
            if(!nand_bbm_checkmarker(i))
                nand_erase_block(i << dev->block_shift);
    }
#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)
    else
    {
        DEBUG_PRINT_CMD("Format NAND Flash. Erase only bbm info block.\n");
        
        // Erase only bbm info block
        nand_erase_block(dev->bbm_info << dev->block_shift);
    }
#endif
    
    // Init bbm info
    return nand_bbm_init();
}

//--------------------------------------------------------------------------
// Map a hardware status to a package error
//--------------------------------------------------------------------------

int flash_hwr_map_error(int e)
{
    return e;
}

//--------------------------------------------------------------------------
// See if a range of FLASH addresses overlaps currently running code
//--------------------------------------------------------------------------

bool flash_code_overlaps(void *start, void *end)
{
    // Currently running code cannot be in NAND FLASH
    return 0;
}

//--------------------------------------------------------------------------
// EOF nand_flash.c
