#ifndef CYGONCE_MT29F_GENERIC_H
#define CYGONCE_MT29F_GENERIC_H
//=============================================================================
//
//      mt29f_generic.h
//
//      Definitions header for the Micron MT29F family
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2010 eCosCentric Limited.
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
// Date:        2010-04-13
// Description: Early definitions for the driver.
//
//####DESCRIPTIONEND####
//=============================================================================

#include <pkgconf/io_nand.h>
#include <pkgconf/devs_nand_micron_mt29f.h>
#include <cyg/nand/nand_device.h>
#include <cyg/hal/drv_api.h>
#include <cyg/infra/diag.h>

#define mt29f_WAIT_tWB() HAL_DELAY_US(1) // really 150ns

/* Our private structure ======================================= */
// Every instance of the chip needs its own copy of this struct.
// N.B. that this is too big to go on the stack in certain
// eCos configurations; it should normally be static.

struct _mt29f_subtype;
typedef struct _mt29f_subtype mt29f_subtype;

struct _mt29f_priv {
    mt29f_subtype *supported_chips; // Must be set by the platform HAL. Terminate with a zero-entry! See mt29f_SUPPORT_LIST below.
    void *plat_priv; // For use by the platform HAL, if desired.
    // ------------ //
    cyg_nand_page_addr pagestash; // Guarded by dev lock.
    CYG_BYTE row_addr_size; // Set up by devinit after chip detection.
#ifdef CYGSEM_IO_NAND_USE_BBT
// The in-RAM BBT for a device is fixed at 2 bits per eraseblock.
// See nand_bbt.c for more.
    unsigned char bbt_data[CYGNUM_DEVS_NAND_MICRON_MT29F_BBT_DATASIZE];
#endif
};

typedef struct _mt29f_priv mt29f_priv;

/* Prototypes for functions to be provided by the platform driver ==== */

// Low-level chip access functions ------------------

static inline void write_cmd(cyg_nand_device *ctx, unsigned char cmd);

static inline void write_addrbytes(cyg_nand_device *ctx, CYG_BYTE *bytes, size_t n);

static inline unsigned char read_data_1(cyg_nand_device *ctx);
static inline void read_data_bulk(cyg_nand_device *ctx, unsigned char *dp, size_t n);
static inline void write_data_1(cyg_nand_device *ctx, unsigned char b);
static inline void write_data_bulk(cyg_nand_device *ctx, const unsigned char *dp, size_t n);

// Chip BUSY line access / fallback -----------------

/* Waits either for the !BUSY line to signal that the device is finished,
   or for the prescribed amount of time, depending on what is available
   on the platform.
 * wait_init specifies the time in microseconds to wait to ensure that
   BUSY is asserted.
 * wait_fallback specifies the worst-case time to wait (from the spec
   sheet; again in microseconds) for the operation to complete. */
static void wait_ready_or_time(cyg_nand_device *ctx,
                               size_t wait_init, size_t wait_fallback);

/* Waits either for the !BUSY line to signal that the device is finished,
 * or (if not available) polls the chip by sending the Read Status command
 * and waits for (response & mask) to be non-zero. */
static void wait_ready_or_status(cyg_nand_device *ctx, CYG_BYTE mask);

// Chip concurrent-access protection ----------------
// (This need not do anything, if the library-provided
//  per-device locking is sufficient.)
 
static inline void mt29f_devlock(cyg_nand_device *ctx);
static inline void mt29f_devunlock(cyg_nand_device *ctx);

// Initialisation hooks -----------------------------

/* Platform-specific chip initialisation.
 * Set up chip access lines, GPIO config, &c; should also set up
 * locking to guard against concurrent access.
 * Return 0 on success, or a negative error code. */
static int mt29f_plf_init(cyg_nand_device *ctx);

/* (N.B. There is no deinit hook, as the library does not currently 
 *  support a device being deconfigured.) */

/* Platform-specific in-memory partition table setup.
 * Return 0 on success, or a negative error code.
 * (This is called at the end of devinit, so you can read from the chip
 * if need be.) */
static int mt29f_plf_partition_setup(cyg_nand_device *dev);

// Internal entrypoints (used by some large page ECC drivers)
#ifdef CYGFUN_NAND_MICRON_MT29F_LP
static inline void change_read_column_lp(cyg_nand_device *dev, cyg_nand_column_addr col);
#endif

// Family support -----------------------------------

// Basic definitions for mt29f subtypes

struct _mt29f_subtype {
    CYG_BYTE ident1, ident3;
    const char *descriptor;
    CYG_BYTE blockcount_bits;
    CYG_BYTE chipsize_log;
    CYG_BYTE row_address_size;
    CYG_BYTE sp; // set non-0 if device is small-page
};

// Callers should not use MT29F_SUBTYPE directly.
// Instead the known subtypes are defined in mt29f_generic.inl.
#define MT29F_SUBTYPE(id1,id3,desc,bcb,csl,ras) { id1, id3, desc, bcb, csl, ras, 0 }
#define MT29F_SUBTYPE_SP(id1,id3,desc,bcb,csl,ras) { id1, id3, desc, bcb, csl, ras, 1 }

// A list of supported subtypes must be zero-terminated!
#define MT29F_SUBTYPE_SENTINEL { 0, 0, 0, 0, 0, 0, 0 }

// For a list of implemented subtypes, refer to the bottom of mt29f_generic.inl.

#endif
