#ifndef CYGONCE_ECOS_YAFFS_DIRITER_H
#define CYGONCE_ECOS_YAFFS_DIRITER_H
//=============================================================================
//
//      ecos-yaffs-diriter.h
//
//      Directory Iterator support for the eCos-YAFFS adaptation layer
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

#include "yaffs_guts.h"

// Directory operations --------------------------------------------------

/* Directory iterator used by readdir().
 * We need to keep track of all live diriters so as to avoid jumping off
 * to Fishkill in the event that our next-object-to-return is deleted.
 * Thankfully, yaffs provides a callback mechanism so we can detect
 * this case. */
typedef struct eyaffs_diriter {
    struct ylist_head iterlist; // doubly linked list, impl by devextras.h
    yaffs_Object * node;
    yaffs_Object * nextret; // next one to return
} eyaffs_diriter;

extern struct ylist_head eyaffs_all_diriters;

void eyaffs_diriter_advance(eyaffs_diriter *ctx);
void eyaffs_diriter_rewind(eyaffs_diriter *ctx);
void eyaffs_diriter_objremove_callback(yaffs_Object *obj);

#endif //CYGONCE_ECOS_YAFFS_DIRITER_H
