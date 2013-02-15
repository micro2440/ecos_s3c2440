#ifndef CYGONCE_ECOS_YAFFS_NAND_H
#define CYGONCE_ECOS_YAFFS_NAND_H
//=============================================================================
//
//      ecos-yaffs-nand.h
//
//      Glue for YAFFS to the eCos NAND library
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2009 eCosCentric Limited.
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
// Date:        2009-05-07
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/nand/nand.h>
#include "yaffs_guts.h" // extendedTags
#include "devextras.h" // __u8 and __u32

// If writing a yaffs_PackedTags1, we store only its eight meaningful bytes 
#define PACKEDTAGS1_OOBSIZE 8
#define PACKEDTAGS2_OOBSIZE(dev) (dev->noTagsECC ?      \
        (unsigned)sizeof(yaffs_PackedTags2TagsPart) :   \
        (unsigned)sizeof(yaffs_PackedTags2))

int eyaffs_eraseBlockInNAND (struct yaffs_DeviceStruct * dev, int blockInNAND);
int eyaffs_initialiseNAND (struct yaffs_DeviceStruct * dev);
// not mandatory:
int eyaffs_deinitialiseNAND (struct yaffs_DeviceStruct * dev);

#ifdef CYGSEM_FS_YAFFS_SMALLPAGE_MODE_YAFFS1
int eyaffs_writeChunkToNAND(struct yaffs_DeviceStruct *dev,
        int chunkInNAND, const __u8 *data,
        const yaffs_Spare *spare);
int eyaffs_readChunkFromNAND(struct yaffs_DeviceStruct *dev,
        int chunkInNAND, __u8 *data,
        yaffs_Spare *spare);
#endif

int eyaffs_writeChunkWithTagsToNAND (struct yaffs_DeviceStruct * dev,
        int chunkInNAND, const __u8 * data,
        const yaffs_ExtendedTags * tags);
int eyaffs_readChunkWithTagsFromNAND (struct yaffs_DeviceStruct * dev,
        int chunkInNAND, __u8 * data,
        yaffs_ExtendedTags * tags);
int eyaffs_markNANDBlockBad (struct yaffs_DeviceStruct * dev, int blockNo);
int eyaffs_queryNANDBlock (struct yaffs_DeviceStruct * dev, int blockNo,
        yaffs_BlockState * state, __u32 *sequenceNumber);

#endif //CYGONCE_ECOS_YAFFS_NAND_H
