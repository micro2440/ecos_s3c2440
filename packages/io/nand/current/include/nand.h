#ifndef CYGONCE_NAND_H
# define CYGONCE_NAND_H
//=============================================================================
//
//      nand.h
//
//      Application interface for the eCos NAND flash library
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
// Date:        2009-02-23
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/nand/nand_device.h>
#include <pkgconf/io_nand.h>

/*
 * Convenience diagnostic function.
 * If debugging is enabled, this function is the default output sink.
 * It sends all its output via diag_printf, prefixed with `NAND: '.
 */
__externC int cyg_nand_defaultprintf(const char *fmt, ...) CYGBLD_ATTRIB_PRINTF_FORMAT(1,2);


/* NAND device addressing and lookup ================================= */

/* NAND devices are addressed by name and, where appropriate, by
 * a numeric partition within the device.
 *
 * To look up a device, you must know its name. Names are defined
 * statically by the platform HAL along with the device's function
 * pointers etc.
 *
 * To actually use a NAND device, you must address your requests to 
 * a partition on it. These can be retrieved, given the device struct,
 * by cyg_nand_get_partition. (You are recommended not to directly
 * access the partitions array within the device struct, in case the
 * mechanism should change in future.)
 *
 * Note that partitions are numbered starting from 0, unlike
 * traditional hard drive partitions.
 */

/* Looks up a device, by name. If necessary, the device driver is
 * initialised and the device interrogated as to its capabilities.
 *
 * On success, stores a pointer to the device struct in *dev_o (if non-null) 
 * and returns 0.
 * Otherwise, returns a negative error code.
 */
__externC int cyg_nand_lookup(const char *devname, cyg_nand_device **dev_o);

/* Obtaining information about the device ============================ */

/* Looks up the given partition number for the given nand device.
 * Partition numbers start at 0 and may go up to CYGNUM_NAND_MAX_PARTITIONS.
 * Returns the partition structure, or NULL if the partition is inactive
 * or the requested number is invalid.
 */
__externC
cyg_nand_partition* cyg_nand_get_partition(cyg_nand_device *dev, unsigned partno);

/* Device info helpers */
#define CYG_NAND_BYTES_PER_PAGE(dev)    (1<<(dev)->page_bits)
#define CYG_NAND_SPARE_PER_PAGE(dev)    ((dev)->spare_per_page)
#define CYG_NAND_PAGES_PER_BLOCK(dev)   (1<<(dev)->block_page_bits)
#define CYG_NAND_BLOCKCOUNT(dev)        (1<<(dev)->blockcount_bits)
#define CYG_NAND_CHIPSIZE(dev)          (1<<(dev)->chipsize_log)
#define CYG_NAND_APPSPARE_PER_PAGE(dev) ((dev)->oob->app_size)

/* NAND access functions ============================================= */

/*
 * Pages and blocks are numbered relative to the partition containing
 * them, not to the whole device. In other words, the first page
 * of every partition is considered to be number 0.
 *
 * (Note: This was changed in application interface v2, and the
 *  application-facing functions renamed.
 *  The library uses device-relative addressing internally.
 *  As an aide-memoire, if a function takes a cyg_nand_partition,
 *  it expects a partition-relative address; if it takes a
 *  cyg_nand_device, it expects a device-relative address.)
 */

/* How big is your partition? */
#define CYG_NAND_PARTITION_NBLOCKS(_p) (1 + (_p)->last - (_p)->first)
#define CYG_NAND_PARTITION_NPAGES(_p) (CYG_NAND_PARTITION_NBLOCKS(_p) * CYG_NAND_PAGES_PER_BLOCK((_p)->dev))

/* Reads a single page and/or its spare area from the device.
 * @page@ specifies the page to be read.
 * If @dest@ is not NULL, it will be used to store the page contents.
 * Exactly NAND_BYTES_PER_PAGE(dev) will be written to @dest@.
 * If @spare@ is not NULL, it will be used to store the contents of the 
 * page's spare area (the lesser of @spare_size@ bytes and
 * NAND_APPSPARE_PER_PAGE(dev)).
 *
 * If the page data is read from the chip, it will be ECC-checked
 * and repaired if necessary.
 *
 * Returns 0 for success, otherwise a negative error code.
 * -EIO : The block could not be successfully read due to an I/O error
 *    which the ECC (if configured) was not able to repair.
 *    *dest and *spare contain the best-effort data read from the chip
 *    but should not be relied upon; the application should take
 *    steps to save as much data as needed and erase the block as
 *    soon as possible.
 *
 * -ENOENT : The page address was not valid.
 * -EINVAL : The page address is within a block that is marked bad.
 * -EFBIG  : You asked for more data from the spare-area than is present
 *    in the current chip layout.
 */
__externC
int cyg_nandp_read_page(cyg_nand_partition *ctx, cyg_nand_page_addr page, 
        void * dest, void * spare, size_t spare_size);

/* Reads a partial page from the device, to @dest@.
 * @page@ specifies the page to be read, @offset@ the address within the
 * page (the column address) to start from, and @length@ the number of
 * bytes.
 *
 * If @check_ecc@ is zero, no attempt will be made to check or repair ECC;
 * the caller accepts responsibility for error detection and correction!
 *
 * Where the device driver supports it, the underlying device may use
 * relevant chip commands to avoid reading the whole page. However,
 * this is not usually compatible with checking the ECC, which requires
 * the whole page to be read; in other words, setting @check_ecc@ tends
 * to not give you the speedup you might otherwise have hoped for by
 * making a part-page read.
 *
 * Returns 0 for success, otherwise a negative error code.
 * -EIO : You requested ECC checking, but an error was found which
 *    the ECC was not able to repair.
 *    *dest and *spare contain the best-effort data read from the chip
 *    but should not be relied upon; the application should take
 *    steps to save as much data as needed and erase the block as
 *    soon as possible.
 *
 * -ENOENT : The page address was not valid.
 * -EINVAL : The page address is within a block that is marked bad.
 * -EFBIG  : You have asked for too much data: offset+length extends
 *    past the end of the page.
 */
__externC
int cyg_nandp_read_part_page(cyg_nand_partition *ctx, cyg_nand_page_addr page, 
        void * dest, size_t offset, size_t length, int check_ecc);

/* Writes a single page and/or its spare area to the device.
 * ECC will be automatically computed and written.
 *
 * @page@ specifies the page to be written.
 * If @src@ is not NULL, the page contents - a whole page - will be
 * read from it.
 * If @spare@ is not NULL, the spare area contents 
 * (the lesser of @spare_size@ bytes and NAND_APPSPARE_PER_PAGE(dev))
 * will be read from it.
 *
 * Returns 0 for success, otherwise a negative error code.
 * -EIO : The page could not be successfully written due to an I/O error.
 *   In this case, the application should copy out data from the preceding 
 *   pages of the eraseblock, then call cyg_nand_bbt_markbad() to mark the
 *   block as bad.
 *
 * -ENOENT : The page address was not valid.
 * -EINVAL : The page address is within a block that is marked bad.
 */
__externC
int cyg_nandp_write_page(cyg_nand_partition *ctx, cyg_nand_page_addr page, 
        const void * src, const void * spare, size_t spare_size);

/* Erases an eraseblock.
 * @blk@ specifies the block to be erased.
 *
 * Returns:
 *   0 for success
 *   -EINVAL : the block was already known to be bad
 *   -EIO : the erase failed; the block has been marked as bad
 *   -ENOENT : the block address was not valid.
 */
__externC
int cyg_nandp_erase_block(cyg_nand_partition *ctx, cyg_nand_block_addr blk);

/* Bad block table functions ======================================= */

typedef enum {
    CYG_NAND_BBT_OK=0,
    CYG_NAND_BBT_WORNBAD=1,
    CYG_NAND_BBT_RESERVED=2,
    CYG_NAND_BBT_FACTORY_BAD=3
} cyg_nand_bbt_status_t;

/* Queries the Bad Block Table for what it knows about a given eraseblock.
 * Returns an enum from cyg_nand_bbt_status_t, or a negative errno value.
 * -ENOENT : the block address was not valid.
 * -EIO : something awful happened with the bad block table.
 */
__externC
int cyg_nandp_bbt_query(cyg_nand_partition *ctx, cyg_nand_block_addr blk);

/* These functions mark a block as worn-bad in the Bad Block Table.
 * (The only difference is in the addressing: markbad_pageaddr takes 
 * a page address and internally converts it to the correct block address.)
 * If a page write fails, the application should salvage data from
 * any still-needed pages in the failing block to another block,
 * then call _markbad to put the block out of use.
 * (If a block erase fails, the block is automatically marked bad.)
 * After this has been called, attempts to use the block fail with -EPERM.
 * Returns 0 on success or a negative errno value.
 * -ENOENT : the block address was not valid.
 * -EIO : something awful happened with the bad block table.
 */
__externC
int cyg_nandp_bbt_markbad(cyg_nand_partition *ctx, cyg_nand_block_addr blk);

__externC
int cyg_nandp_bbt_markbad_pageaddr(cyg_nand_partition *ctx, cyg_nand_page_addr pg);

#endif
