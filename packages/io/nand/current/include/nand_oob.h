#ifndef CYGONCE_NAND_OOB_H
# define CYGONCE_NAND_OOB_H
//=============================================================================
//
//      nand_oob.h
//
//      eCos NAND flash library - Out Of Band (Spare) area layout defs
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
// Date:        2009-03-13
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/infra/cyg_type.h>
#include <pkgconf/isoinfra.h>
#include CYGBLD_ISO_ERRNO_CODES_HEADER

/* The Out Of Band ("spare") area on every page of a NAND array contains
 * the following:
 *   * Somewhere to put the ECC data
 *   * Somewhere to put application data
 *   * (Maybe) a factory bad marker.
 *
 * To make matters more complicated, both the ECC and the application
 * OOB data may be spread across non-contiguous regions.
 *
 * In theory, we can overwrite the position of the factory-bad marker as
 * we've scanned the array to make our own bad-block table.
 * However, should the array be reinitialised by some other OS with 
 * different BBT semantics, it is likely to rescan the array looking 
 * for factory bad markers.
 * Therefore, we should avoid writing to the bad-marker location on
 * general principles. The Linux MTD layer does this in its OOB layout
 * definitions.
 */

/* NOTE: For the sake of reducing code size, we currently use a byte
 * to declare locations. This is fine, provided that a chip's OOB
 * is less than 256 bytes per page.
 * Well, chips with a 2k page size typically have 64 byte OOB area,
 * so we might have to change this typedef if/when pages reach 8k.
 */

typedef CYG_BYTE oobpos_t;

/* Vector component. If 'len'==0, there are no more elements. */
typedef struct {
    const oobpos_t pos, len;
} oob_vector;

/* Increase these maxima as necessary, but not excessively or you'll waste ROM space. */
#define CYG_NAND_OOB_MAX_ECC_SLOTS 2
#define CYG_NAND_OOB_MAX_APP_SLOTS 2

typedef struct {
    const oobpos_t ecc_size; // total ECC _bytes_
    const oob_vector ecc[CYG_NAND_OOB_MAX_ECC_SLOTS];
    // CAUTION! Both ecc and app vectors MUST BE IN ORDER!
    const oobpos_t app_size; // total bytes available for app data
    const oob_vector app[CYG_NAND_OOB_MAX_APP_SLOTS];
} cyg_nand_oob_layout;

/* Packs 'app_size' bytes from 'app' (appdata) and layout->ecc_size bytes
 * from 'ecc' into 'packed'.
 * app_size must be <= layout->app_size and will be SILENTLY TRUNCATED
 * if too big.
 * Bytes not referenced by the layout are not touched; the caller should
 * usually pre-initialise the buffer to 0xFF.
 */
__externC void nand_oob_pack(struct _cyg_nand_device_t *dev,
                             const CYG_BYTE *app, const unsigned app_size,
                             const CYG_BYTE *ecc, CYG_BYTE *packed);

/* Unpacks data from 'packed' into 'ecc' (layout->ecc_size bytes)
 *  and 'app' (no more than 'app_max' bytes).
 *  It's up to the app to decide however many app bytes are useful to it.
 *  If unsure, you could always read out them all (layout->app_size).
 */
__externC void nand_oob_unpack( struct _cyg_nand_device_t *dev,
                                CYG_BYTE *app, const unsigned app_max,
                                CYG_BYTE *ecc, const CYG_BYTE *packed);

/* Writes @len@ bytes of @data@ into an @oobbuf@ such that the data 
 * will end up at the given @rawpos@ in the packed layout.
 * Returns 0 for success or -1 if it's not possible. */
__externC int nand_oob_packed_write(struct _cyg_nand_device_t *dev,
        size_t rawpos, size_t len,
        CYG_BYTE *oobbuf, const CYG_BYTE *data);

/* Opposite of nand_oob_packed_write.
 * Reads @len@ bytes from an application @oobbuf@ such that they
 * came from the given @rawpos@ in the packed layout; copies them
 * to @data@.
 * Returns 0 for success or -1 if it's not possible. */
__externC int nand_oob_packed_read(struct _cyg_nand_device_t *dev,
        size_t rawpos, size_t len,
        const CYG_BYTE *oobbuf, CYG_BYTE *data);

/* And now some layouts from the Linux MTD layer. */

extern const cyg_nand_oob_layout
#if 0
/* Disabled for now - see comments in source */
                                 nand_mtd_oob_8,
#endif
                                 nand_mtd_oob_16,
                                 nand_mtd_oob_64;

/* How much application-usable OOB spare is there per page? */
#define CYG_NAND_APPLICATION_OOB(dev) (dev->oob->app_size)

#endif
