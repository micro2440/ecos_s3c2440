#ifndef CYGONCE_NAND_BBT_H
# define CYGONCE_NAND_BBT_H
//=============================================================================
//
//      nand_bbt.h
//
//      Bad Block Table interface for the eCos NAND flash library
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
// Date:        2009-03-11
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/nand/nand_device.h>
#include <cyg/nand/nand.h>
#include <pkgconf/io_nand.h>

/* BEWARE!
 *
 * Applications should not include this file.
 * The functions within do NOT perform device locking.
 * All the contents are internal interfaces and subject to
 * change without notice.
 */

/* Bad block table =================================================== */
#ifdef CYGSEM_IO_NAND_USE_BBT

/* On-chip values (in a 2-bit wide field).
 * Each byte on-chip maps its _least_ significant bits to the _first_
 * block it relates to. (i.e. read with a shifting mask, starting with 0 shift)
 *
 * Each byte in RAM will be stored in the same way, but with values from
 * cyg_nand_bbt_status_t.
 */
typedef enum {
    BBTI_FACTORY_BAD=0,
    BBTI_WORNBAD_1=1,
    BBTI_WORNBAD_2=2,
    BBTI_OK=3,
} nand_bbti_onchip_status_t;

/* Raw unchecked query. Returns a cyg_nand_bbt_status_t enum,
 * or -ve error code. */
int cyg_nand_bbti_query(cyg_nand_device *dev, cyg_nand_block_addr blk);

/* Marks a block as bad. Returns 0 for success, or -ve error code.
 * Caller must have locked the devlock. */
int cyg_nand_bbti_markbad(cyg_nand_device *dev, cyg_nand_block_addr blk);

/* Marks a block arbitrarily. Normal applications should never use this.
 * Returns 0 for success, or -ve error code.
 * Caller must have locked the devlock. */
int cyg_nand_bbti_markany(cyg_nand_device *dev, cyg_nand_block_addr blk,
                          cyg_nand_bbt_status_t st);

/* Looks to find the bad block table(s) on the chip and read them in.
 * Updates dev->bbt.* as appropriate.
 * Caller must have locked the devlock or (more likely) the device not 
 * be accessible to callers yet (as we're running from nand_lookup).
 *
 * Returns: 0 if either or both were found;
 *          -ENOENT if neither; 
 *          something else (-ve error code) if something awful happened.
 */
int cyg_nand_bbti_find_tables(cyg_nand_device *dev);

/* Creates the initial BBTs by running a scan for factory bad blocks.
 * Returns: 0 if OK, else a -ve error code.
 * Caller must have locked the devlock or (more likely) the device not 
 * be accessible to callers yet (as we're running from nand_lookup).
 */
int cyg_nand_bbti_build_tables(cyg_nand_device *dev);

/* TODO: Make BBT search parameterisable to support non-default behaviour.
 * This is probably required in order to support devices with only 8
 * OOB bytes per page.
 *
 * At present we have hard-coded the default parameters from the Linux MTD
 * layer: that the BBT and its mirror can be in one of the last four
 * eraseblocks on the device, their signatures 'Bbt0' and '1ttB', the
 * offsets of the signature and version byte within the OOB area of the
 * first page of the eraseblock.
 */
#endif

/* Raw unchecked NAND access, for use by the BBT ===================== */

/* Takes a DEVICE address. Caller must hold the devlock! */
int nandi_read_whole_page_raw(cyg_nand_device *dev, cyg_nand_page_addr page,
                              CYG_BYTE * dest,
                              CYG_BYTE * spare, size_t spare_size,
                              int check_ecc);

/* Takes a DEVICE address. Caller must hold the devlock! */
int nandi_write_page_raw(cyg_nand_device *dev, cyg_nand_page_addr page,
                         const CYG_BYTE * src,
                         const CYG_BYTE * spare, size_t spare_size);

/* Code outside of the BBT can use the pagebuffer ==================== */
CYG_BYTE* nandi_grab_pagebuf(void);
void nandi_release_pagebuf(void);

/* =================================================================== */

#endif
