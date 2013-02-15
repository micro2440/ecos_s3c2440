//=============================================================================
//
//      nandinit.cxx
//
//      eCos NAND flash library - auto-init magic
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
// Date:        2009-06-02
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/nand/nand.h>
#include <cyg/nand/nand_devtab.h>

// This is a dummy class so we can execute the NAND package initialiser
// automatically at the right priority.

__externC void cyg_nand_initx(cyg_nand_printf pf);

class cyg_nand_init_class {
    public:
        cyg_nand_init_class(void) {
#if defined(CYGSEM_IO_NAND_DEBUG_FN_DEFAULT)
            cyg_nand_initx(CYGSEM_IO_NAND_DEBUG_FN_DEFAULT);
#else
            cyg_nand_initx(NULL);
#endif
        }
};

// And here's an instance of it so that it fires:

static cyg_nand_init_class _cyg_nand_init CYGBLD_ATTRIB_INIT_PRI(CYG_INIT_IO);

/* Device HAL table goes here, in libextras */

CYG_HAL_TABLE_BEGIN(cyg_nanddevtab, cyg_nand_dev);
CYG_HAL_TABLE_END(cyg_nanddevtab_end, cyg_nand_dev);

