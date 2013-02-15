#ifndef _CYGONCE_FLASH_NAND_H_
#define _CYGONCE_FLASH_NAND_H_
//==========================================================================
//
//      nand_flash.h
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
//
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

#include <cyg/infra/cyg_type.h>
#include <cyg/io/flash.h>

#ifdef _FLASH_PRIVATE_

//--------------------------------------------------------------------------
// Defines

#define NAND_REPAIR_SIZE    (CYGNUM_DEVS_FLASH_NAND_BBM_REPAIR_SIZE)

//--------------------------------------------------------------------------
// NAND flash commands

typedef enum
{
    NAND_CMD_READ0      = 0x00,
    NAND_CMD_READ1      = 0x01,
    NAND_CMD_PAGEPROG   = 0x10,
    NAND_CMD_READSPARE  = 0x50,
    NAND_CMD_ERASE      = 0x60,
    NAND_CMD_STATUS     = 0x70,
    NAND_CMD_SEQIN      = 0x80,
    NAND_CMD_READID     = 0x90,
    NAND_CMD_ERASE2     = 0xD0,
} cyg_nand_command;

typedef enum
{
    NAND_STATUS_FAIL    = 0x01,
    NAND_STATUS_READY   = 0x40,
    NAND_STATUS_NOWP    = 0x80
} cyg_nand_status;

//--------------------------------------------------------------------------
// 

typedef struct cyg_nand_partinfo_st
{
    cyg_uint32  id;             // Device ID
    
    cyg_uint32  page_size;      // Page size in bytes.
    
    cyg_uint32  pages_count;    // Pages count in one block.
    cyg_uint32  blocks_count;   // Blocks count in chip.
    
    cyg_uint32  options;        // Flash options
    
} cyg_nand_partinfo;

//--------------------------------------------------------------------------
// The device-specific data

typedef struct cyg_nand_dev_st
{
    void*       flash_base;     // Flash base address
    
    void*       addr_r;         // Address to read data from flash
    void*       addr_w;         // Address to write data to flash
    
    cyg_int32   delay_cmd;      // Delay after command or address (in micro-seconds)
    cyg_int32   delay_rst;      // Delay after reset (in micro-seconds)
    
    //-------------------------------
    
    cyg_uint32  id;
    
    cyg_uint32  page_size;
    cyg_uint32  spare_size;
    
    cyg_int32   pages_count;
    cyg_int32   blocks_count;
    
    int         page_shift;
    int         block_shift;
    
    cyg_uint8*  buf_data;       // Pointer to a data buffer
    cyg_uint8*  buf_spare;      // Pointer to a spare buffer
    cyg_uint8*  buf_bbt;        // Pointer to a bad block table
    
    cyg_int32   pageinbuf;      // Buffered page number (-1 if buffer is empty)
    
#if defined(CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR)
    
    cyg_uint32  bbm_info;       // Number of block with bbm info
    cyg_int32   bbm_remap[NAND_REPAIR_SIZE];
    
#endif /*CYGSEM_DEVS_FLASH_NAND_BBM_REPAIR*/
    
} cyg_nand_dev;

#endif // _FLASH_PRIVATE_

#endif // _CYGONCE_FLASH_NAND_H_
//--------------------------------------------------------------------------
// EOF nand_flash.h
