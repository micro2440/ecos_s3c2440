#ifndef _CYGONCE_FLASH_NAND_PARTS_INL_
#define _CYGONCE_FLASH_NAND_PARTS_INL_
//==========================================================================
//
//      nand_flash_parts.inl
//
//      FLASH memory - NAND flash parts descriptions
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
    
static const cyg_nand_partinfo nand_supported[] = 
{
    //  Device ID   Page size   Pages count     Blocks count    Options
    {   0x6E,       256,        16,             256,            0           }, //1MB
    {   0x64,       256,        16,             512,            0           }, //2MB
    {   0x6B,       512,        16,             512,            0           }, //4MB
    {   0xE8,       256,        16,             256,            0           }, //1MB
    {   0xEC,       256,        16,             256,            0           }, //1MB
    {   0xEA,       256,        16,             512,            0           }, //2MB
    {   0xD5,       512,        16,             512,            0           }, //4MB
    {   0xE3,       512,        16,             512,            0           }, //4MB
    {   0xE5,       512,        16,             512,            0           }, //4MB
    {   0xD6,       512,        16,            1024,            0           }, //8MB
    {   0x39,       512,        16,            1024,            0           }, //8MB
    {   0xE6,       512,        16,            1024,            0           }, //8MB
    {   0x33,       512,        32,            1024,            0           }, //16MB
    {   0x73,       512,        32,            1024,            0           }, //16MB
    {   0x35,       512,        32,            2048,            0           }, //32MB
    {   0x75,       512,        32,            2048,            0           }, //32MB
    {   0x36,       512,        32,            4096,            0           }, //64MB
    {   0x76,       512,        32,            4096,            0           }, //64MB
    {   0x78,       512,        32,            8192,            0           }, //128MB
    {   0x39,       512,        32,            8192,            0           }, //128MB
    {   0x79,       512,        32,            8192,            0           }, //128MB
    {   0xF1,       2048,       64,           1024,             0           }, //128MB
    {   0x71,       512,        32,           16384,            0           }, //256MB
   
};

#define NAND_SUPPORTED_COUNT    (sizeof(nand_supported) / sizeof(cyg_nand_partinfo))

#endif // _CYGONCE_FLASH_NAND_PARTS_INL_
//--------------------------------------------------------------------------
// EOF nand_flash_parts.inl
