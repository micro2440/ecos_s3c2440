#ifndef CYGONCE_ST_NANDXXXX3A_H
#define CYGONCE_ST_NANDXXXX3A_H
//=============================================================================
//
//      nandxxxx3a.h
//
//      Definitions header for the ST Microelectronics NANDxxxx3A family.
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2009 Free Software Foundation.
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
// Author(s):   Simon Kallweit
// Date:        2009-06-30
// Description: Early definitions for the driver.
//              The intention is that the instantiating C file include
//              this file once, define the things it needs to, then
//              include st_nand.inl as many times as necessary
//              (probably just once).
//
//####DESCRIPTIONEND####
//=============================================================================

#include <pkgconf/io_nand.h>
#include <cyg/nand/nand_device.h>
#include <cyg/hal/drv_api.h>
#include <cyg/infra/diag.h>

#define st_nand_wait_for_BUSY() HAL_DELAY_US(1)

/* Our private structure ======================================= */
// Every instance of the chip needs its own copy of this struct.
// N.B. that this is too big to go on the stack in certain
// eCos configurations; it should normally be static.

struct nandxxxx3a_priv {
    void *plat_priv; // For use by the platform HAL, if desired.
    int mbit;
#ifdef CYGSEM_IO_NAND_USE_BBT
    unsigned char *bbt_data;
#endif
    cyg_nand_page_addr pageop; // Protected by device lock
    size_t written; // Protected by device lock
};

#ifdef CYGSEM_IO_NAND_USE_BBT
#define DECLARE_BBT_DATA(_structname_, _mbit_) unsigned char _structname_##_bbt_data[_mbit_ * 8]
#define INIT_BBT_DATA(_structname_) .bbt_data = _structname_##_bbt_data,
#else
#define DECLARE_BBT_DATA(_structname_, _mbit_)
#define INIT_BBT_DATA(_structname_)
#endif


// Macro to instantiate a ST NANDxxxx3A device
#define NANDXXXX3A_DEVICE(_structname_, _devname_, _mbit_, _priv_, _ecc_, _oob_)\
    DECLARE_BBT_DATA(_structname_, _mbit_); \
struct nandxxxx3a_priv _structname_##_priv =                                \
{                                                                           \
    .plat_priv = _priv_,                                                    \
    .mbit = _mbit_,                                                         \
    INIT_BBT_DATA(_structname_) \
};                                                                          \
CYG_NAND_DEVICE(_structname_, _devname_, &nandxxxx3a_funs,                  \
                &_structname_##_priv, _ecc_, _oob_);

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
 
static inline void nandxxxx3a_devlock(cyg_nand_device *ctx);
static inline void nandxxxx3a_devunlock(cyg_nand_device *ctx);

// Initialisation hooks -----------------------------

/* Platform-specific chip initialisation.
 * Set up chip access lines, GPIO config, &c; should also set up
 * locking to guard against concurrent access.
 * Return 0 on success, or a negative error code. */
static int nandxxxx3a_plf_init(cyg_nand_device *ctx);

/* (N.B. There is no deinit hook, as the library does not currently 
 *  support a device being deconfigured.) */

/* Platform-specific in-memory partition table setup.
 * Return 0 on success, or a negative error code.
 * (This is called at the end of devinit, so you can read from the chip
 * if need be.) */
static int nandxxxx3a_plf_partition_setup(cyg_nand_device *dev);

#endif // CYGONCE_ST_NANDXXXX3A_H
