//=============================================================================
//
//      mini2440_nand.c
//
//      NAND flash support for the MINI2440 board.
//      (This is the version for the eCos HAL for that board.)
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
// Author(s):   Ricky Wu
// Date:        2011-04-14
//
//####DESCRIPTIONEND####
//=============================================================================

#include <pkgconf/hal_arm_arm9_mini2440.h>

#include <cyg/nand/nand_device.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>

#include <cyg/devs/nand/k9_generic.h>

/* Private structs. We need one k9_priv (and one of our privs) per
 * instance of the chip. */

struct _mypriv {
    char dummy; // shush, gcc
};

static struct _mypriv _mini2440_nand_priv;

#define CAST_MYPRIV(x) ((struct _mypriv *) (x))
#define CAST_K9PRIV(x) ((k9_priv *) (x))
#define GET_MYPRIV(dev,var) struct _mypriv * var = CAST_MYPRIV(CAST_K9PRIV((dev)->priv)->plat_priv);

/* We could use 'priv' to supply the NAND addresses. This might be
 * useful if there were multiple chips on a board, but for simplicity
 * we're going to hard-wire it meantime. */


static inline void nand_select()
{
    cyg_uint16 reg;
    HAL_READ_UINT16(NFCONT,reg);
    reg &= ~(1 << 1);
    HAL_WRITE_UINT16(NFCONT,reg);
}

static inline void nand_deselect()
{
    cyg_uint16 reg;
    HAL_READ_UINT16(NFCONT,reg);
    reg |= (1 << 1);
    HAL_WRITE_UINT16(NFCONT,reg);
}

static inline void nand_clear_RnB()
{
    cyg_uint16 reg;
    HAL_READ_UINT16(NFSTAT,reg);
    reg |= (1 << 2);
    HAL_WRITE_UINT16(NFSTAT,reg);
}

static inline void tweak_pin_register(CYG_ADDRWORD reg, cyg_uint32 clearbits, cyg_uint32 setbits)
{
    cyg_uint32 val;
    HAL_READ_UINT32(reg, val);
    val &= ~clearbits;
    val |= setbits;
    HAL_WRITE_UINT32(reg, val);
}

/* CHIP OPERATIONS ================================================= */

/* On this board, the memory controller is well set up, so we don't need
 * to worry about timings for most operations. */

static inline void write_cmd(cyg_nand_device *ctx, unsigned char cmd)
{
    HAL_WRITE_UINT8(NFCMD,cmd);
}

static inline void write_addrbytes(cyg_nand_device *ctx, CYG_BYTE *bytes, size_t n)
{
    HAL_WRITE_UINT8_VECTOR(NFADDR, bytes, n, 0);
}

static inline unsigned char read_data_1(cyg_nand_device *ctx)
{
    unsigned char b;

    HAL_READ_UINT8(NFDATA, b);
    return b;
}

static inline void read_data_bulk(cyg_nand_device *ctx, unsigned char *dp, size_t n)
{
    // Most of the time, we expect to be dealing with word-aligned
    // multiples of 4 bytes, so optimise for that case.

    if ( ((CYG_ADDRWORD)dp&3)==0 && (n%4)==0) {
        cyg_uint32 *ip = (cyg_uint32*) dp;
        cyg_uint32 r;
        n /= 4;
        while (n) {
            HAL_READ_UINT32(NFDATA, r);
            *(ip++) = r;
            --n;
        }
    } else
        HAL_READ_UINT8_VECTOR(NFDATA, dp, n, 0);
}

static inline void write_data_1(cyg_nand_device *ctx, unsigned char b)
{
    HAL_WRITE_UINT8(NFDATA,b);
}

static inline void write_data_bulk(cyg_nand_device *ctx, const unsigned char *dp, size_t n)
{
    if ( ((CYG_ADDRWORD)dp&3)==0 && (n%4)==0) {
        cyg_uint32 *ip = (cyg_uint32*) dp;
        cyg_uint32 r;
        n /= 4;
        while (n) {
            r = *(ip++);
            HAL_WRITE_UINT32(NFDATA, r);
            --n;
        }
    } else
        HAL_WRITE_UINT8_VECTOR(NFDATA, dp, n, 0);
}


/* READY line handling and fallback ================================ */

// Forward defs:
/* Waits either for the !BUSY line to signal that the device is finished,
   or for the prescribed amount of time.
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

#define POLL_INTERVAL 10 /* us */

#define POLLCOUNT_COUNTING_LEVEL 7
#if defined(CYGSEM_IO_NAND_DEBUG_LEVEL) && (CYGSEM_IO_NAND_DEBUG_LEVEL >= POLLCOUNT_COUNTING_LEVEL)
#define REPORT_POLLS
#endif


// fwd def from the k9 driver:
static inline CYG_BYTE read_status(cyg_nand_device *dev);

static void wait_ready_or_time(cyg_nand_device *ctx, size_t initial, size_t fallback)
{
    NAND_CHATTER(8, ctx, "Waiting %d us for operation\n", initial+fallback);
    HAL_DELAY_US(initial+fallback);
}

unsigned short nand_read_id()
{
	unsigned short res = 0;
	unsigned short res1 = 0;
	HAL_WRITE_UINT8(NFCMD,0x90);
	HAL_WRITE_UINT32(NFADDR,0x0);

	HAL_READ_UINT8(NFDATA,res);
	HAL_READ_UINT8(NFDATA,res1);
	res = (res << 8) | res1;
	return res;
}


static void wait_ready_or_status(cyg_nand_device *dev, CYG_BYTE mask)
{
    // The Ready line won't be ready for at least tWB (100ns), so out of 
    // paranoia we'll wait at least that long before reading Status.
    cyg_uint8 r_status;

    HAL_READ_UINT8(NFSTAT,r_status);
    while(!(r_status & (1<<0)))
    HAL_READ_UINT8(NFSTAT,r_status);
}

#define TACLS 7
#define TWRPH0 7
#define TWRPH1 7
static int k9_plf_init(cyg_nand_device *dev)
{
#if 0
    cyg_uint32 reg;

    HAL_READ_UINT32(CLKCON, reg);
    reg |= (1<<4);
    HAL_WRITE_UINT32(CLKCON, reg);

    HAL_WRITE_UINT32(NFCONF,(TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4)|(0<<0));
    HAL_WRITE_UINT16(NFCONT,(0<<13)|(0<<12)|(0<<10)|(0<<9)|(0<<8)|(1<<6)|(1<<5)|(1<<4)|(1<<1)|(1<<0));

#ifdef INITHOOK
    return INITHOOK(dev);
#else
#endif
    return 0;
#endif
}

/* Partition support ===================================================
 * Without Manual config, developer has to set up the partitions list
 * by hand. */
#ifdef CYGSEM_DEVS_NAND_MINI2440_PARTITION_MANUAL_CONFIG

static inline int mini2440_init_manual_partition(cyg_nand_device *dev)
{
#define PARTITION(i) do {                                               \
    cyg_nand_block_addr                                                 \
        base = CYGNUM_DEVS_NAND_MINI2440_PARTITION_ ## i ## _BASE,    \
        size = CYGNUM_DEVS_NAND_MINI2440_PARTITION_ ## i ## _SIZE;    \
    dev->partition[i].dev = dev;                                        \
    dev->partition[i].first = base;                                     \
    dev->partition[i].last = size ?                                     \
                base + size - 1: (1<<dev->blockcount_bits)-1;           \
} while(0)

#ifdef CYGPKG_DEVS_NAND_MINI2440_PARTITION_0
    PARTITION(0);
#endif
#ifdef CYGPKG_DEVS_NAND_MINI2440_PARTITION_1
    PARTITION(1);
#endif
#ifdef CYGPKG_DEVS_NAND_MINI2440_PARTITION_2
    PARTITION(2);
#endif
#ifdef CYGPKG_DEVS_NAND_MINI2440_PARTITION_3
    PARTITION(3);
#endif
    /* Oh for the ability to write a for-loop in pre-processor,
       or a more powerful libcdl ... */
    return 0;
}
#define PARTITIONHOOK mini2440_init_manual_partition

#endif // CYGSEM_DEVS_NAND_MINI2440_PARTITION_MANUAL_CONFIG

static int k9_plf_partition_setup(cyg_nand_device *dev)
{
#ifdef PARTITIONHOOK
    return PARTITIONHOOK(dev);
#else
    return 0;
#endif
}

/* Concurrent access protection ======================================== */

// ... is not required on this platform.
// (On some boards, chips might share a chip-select line, CPLD or similar.)

static inline void k9_devlock(cyg_nand_device *dev)
{
    nand_select();
    nand_clear_RnB();
}

static inline void k9_devunlock(cyg_nand_device *dev)
{
    nand_deselect();
}

/* Putting it all together ... ========================================= */

#include <cyg/devs/nand/k9_generic.inl>

static k9_subtype chips[] = { K9F1G08U0x, K9_SUBTYPE_SENTINEL };

static k9_priv _k9_mini2440_priv = { &chips, &_mini2440_nand_priv };

CYG_NAND_DEVICE(mini2440_nand, "onboard", &nand_k9_funs, &_k9_mini2440_priv,
                &mtd_ecc256_fast, &nand_mtd_oob_64);

