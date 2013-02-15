#ifndef CYGONCE_FS_ECOS_YAFFS_H
#define CYGONCE_FS_ECOS_YAFFS_H
//=============================================================================
//
//      ecos-yaffs.h
//
//      Options for the eCos interface to the YAFFS filesystem
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
// Date:        2009-05-05
//
//####DESCRIPTIONEND####
//=============================================================================

#include <pkgconf/system.h>
#include <pkgconf/fs_yaffs.h>

/* Options from yaffs moduleconfig.h */
#define CONFIG_YAFFS_FS

#ifdef CYGSEM_FS_YAFFS_SMALLPAGE_MODE_YAFFS1
# define CONFIG_YAFFS_YAFFS1
# define CONFIG_YAFFS_YAFFS2
#else // CYGSEM_FS_YAFFS_SMALLPAGE_MODE_YAFFS2
# define CONFIG_YAFFS_NO_YAFFS1
# define CONFIG_YAFFS_YAFFS2
#endif

/* eCos does not currently supply DT_REG &c. and ATTR_MODE &c., so we'll
 * use YAFFS's: */
#define CONFIG_YAFFS_PROVIDE_DEFS

/* Our NAND layer does ECC, so we won't use YAFFS's - but we do need
 * to use its internal ECC on the tags */
//#define YAFFS_IGNORE_TAGS_ECC
//#define CONFIG_YAFFS_DOES_ECC
//#define CONFIG_YAFFS_ECC_WRONG_ORDER

/* Do not test whether chunks are erased before writing to them */
#define CONFIG_YAFFS_DISABLE_CHUNK_ERASED_CHECK

/* Cache short names, taking more RAM, but faster look-ups */
#ifdef CYGSEM_FS_YAFFS_CACHE_SHORT_NAMES
#define CONFIG_YAFFS_SHORT_NAMES_IN_RAM
#endif

/* The count of blocks to reserve for checkpointing */
#define CONFIG_YAFFS_CHECKPOINT_RESERVED_BLOCKS 10

/* Use older-style on-NAND data format with pageStatus byte?
 * This probably requires work in the NAND layer in order to support. */
//#define CONFIG_YAFFS_9BYTE_TAGS

#endif /* CYGONCE_FS_ECOS_YAFFS_H */
