//=============================================================================
//
//      util.c
//
//      eCos NAND flash library - sundry utilities
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
// Date:        2009-06-01
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/nand/nand.h>
#include <cyg/nand/nand_device.h>
#include CYGBLD_ISO_ERRNO_CODES_HEADER
#include <stdlib.h>
#include <string.h>

#include <cyg/nand/util.h>

/* Resolver from dev name (e.g. onboard/0) to device and partition structs
 * if there's no /, we'll just look up the device name and - if part isn't
 * NULL - set *part to NULL.
 * Reasonably foreseeable returns:
 *   0 - device (and partition if provided) successfully looked up
 *   -ENODEV - device name not found
 *   -ENOENT - device found, partition not found 
 */
int cyg_nand_resolve_device(const char *dev, cyg_nand_device **nand,
                            cyg_nand_partition **part)
{
    if (!dev) return -EINVAL;
    CYG_CHECK_DATA_PTRC(dev);
    const char *sepa = strchr(dev, '/');
    cyg_nand_device *nandx;

    if (sepa) { // Split it out ...
        int len = sepa - dev;
        char devalone[1+len];
        strncpy(devalone, dev, len);
        devalone[len]=0;
        cyg_nand_lookup(devalone, &nandx);
    } else {
        cyg_nand_lookup(dev, &nandx);
    }

    if (nand) *nand = nandx;
    if (part) *part = 0;
    if (!nandx) return -ENODEV;

    if (!sepa) return 0; // All done

    long partno = strtol(sepa+1, 0, 10);
    cyg_nand_partition *partx = cyg_nand_get_partition(nandx, partno);

    if (part) *part = partx;
    if (!partx) return -ENOENT;
    return 0;
}

