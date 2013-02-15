//=============================================================================
//
//      stm3210e_eval_nand.c
//
//      Cortex-M3 STM3210E EVAL NAND setup
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2009 Free Software Foundation
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
// Author(s):   Simon Kallweit, Ross Younger
// Date:        2009-06-30
//
//####DESCRIPTIONEND####
//=============================================================================

#include <pkgconf/hal_cortexm_stm32_stm3210e_eval.h>

#include <cyg/infra/cyg_ass.h>

#include <cyg/nand/nand_device.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>

#include <cyg/devs/nand/nandxxxx3a.h>

/* Private structs. We need one nandxxxx3a_priv (and one of our privs) per
 * instance of the chip. */

struct _mypriv {
    char dummy; // not currently used
};

static struct _mypriv _stm3210_nand_priv;

#define CAST_MYPRIV(x) ((struct _mypriv *) (x))
#define CAST_NANDPRIV(x) ((struct nandxxxx3a_priv *) (x))
#define GET_MYPRIV(dev,var) struct _mypriv * var = CAST_MYPRIV(CAST_NANDPRIV((dev)->priv)->plat_priv);

/* We could use 'priv' to supply the NAND addresses. This might be
 * useful if there were multiple chips on a board, but for simplicity
 * we're going to hard-wire it meantime. */
#define NAND_BASE   CYGHWR_HAL_STM32_FSMC_BANK2_BASE
#define NAND_CMD    (NAND_BASE | CYGHWR_HAL_STM32_FSMC_BANK_CMD)
#define NAND_ADDR   (NAND_BASE | CYGHWR_HAL_STM32_FSMC_BANK_ADDR)

/* Where can we read the Ready/!Busy signal? */
#ifdef CYGHWR_HAL_CORTEXM_STM3210E_EVAL_RB_ON_INT2
# define GPIO_NAND_RB_PIN CYGHWR_HAL_STM32_GPIO(G, 6, IN, PULLUP) // if JP7 2-3
#else
# define GPIO_NAND_RB_PIN CYGHWR_HAL_STM32_GPIO(D, 6, IN, PULLUP) // if JP7 1-2
#endif

/* CHIP OPERATIONS ================================================= */

/* On this board, the memory controller is well set up, so we don't need
 * to worry about timings for most operations. */

static inline void write_cmd(cyg_nand_device *ctx, unsigned char cmd)
{
    HAL_WRITE_UINT8(NAND_CMD, cmd);
}

static inline void write_addrbytes(cyg_nand_device *ctx, CYG_BYTE *bytes, size_t n)
{
    HAL_WRITE_UINT8_VECTOR(NAND_ADDR, bytes, n, 0);
}

static inline unsigned char read_data_1(cyg_nand_device *ctx)
{
    unsigned char b;
    HAL_READ_UINT8(NAND_BASE, b);
    return b;
}

static inline void read_data_bulk(cyg_nand_device *ctx, unsigned char *dp, size_t n)
{
    // Most of the time, we expect to be dealing with word-aligned
    // multiples of 4 bytes, so optimise for that case.
    if ( ((CYG_ADDRWORD)dp&3)==0 && (n%4)==0) {
        volatile cyg_uint8 const *a = (cyg_uint8*) NAND_BASE;
        cyg_uint32 *ip = (cyg_uint32*) dp;
        cyg_uint32 r;
        n /= 4;
#if (CYGINT_HAL_CORTEXM_BIGENDIAN == 1)
#define ONEWORD do { r  = (*a) << 24; r |= (*a) << 16; r |= (*a) << 8; r |= *a; } while(0)
#else
#define ONEWORD do { r  = *a; r |= (*a) << 8; r |= (*a) << 16; r |= (*a) << 24; } while(0)
#endif
        while (n) {
            ONEWORD;
            *(ip++) = r;
            --n;
        }
#undef ONEWORD
    } else
        HAL_READ_UINT8_VECTOR(NAND_BASE, dp, n, 0);
}

static inline void write_data_1(cyg_nand_device *ctx, unsigned char b)
{
    HAL_WRITE_UINT8(NAND_BASE,b);
}

static inline void write_data_bulk(cyg_nand_device *ctx, const unsigned char *dp, size_t n)
{
    if ( ((CYG_ADDRWORD)dp&3)==0 && (n%4)==0) {
        volatile cyg_uint8 *a = (cyg_uint8*) NAND_BASE;
        cyg_uint32 *ip = (cyg_uint32*) dp;
        cyg_uint32 r;
        n /= 4;
#ifdef CYGHWR_HAL_ARM_BIGENDIAN
#define ONEWORD do { *a = (r>>24)&0xff; *a = (r>>16)&0xff; (*a) = (r>>8) & 0xff; (*a) = r & 0xff; } while(0)
#else
#define ONEWORD do { *a = r & 0xff; *a = (r>>8) & 0xff; *a = (r>>16)&0xff; *a = (r>>24)&0xff; } while(0)
#endif
        while (n) {
            r = *(ip++);
            ONEWORD;
            --n;
        }
#undef ONEWORD
    } else
        HAL_WRITE_UINT8_VECTOR(NAND_BASE, dp, n, 0);
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

/* Case 1: The NAND_RDY line is not connected. ---------------------- */
#ifndef CYGHWR_HAL_CORTEXM_STM3210E_EVAL_RB_ON_INT2

// fwd def from the chip driver:
static inline CYG_BYTE read_status(cyg_nand_device *dev);

static void wait_ready_or_time(cyg_nand_device *ctx, size_t initial, size_t fallback)
{
    // With the jumper set to route the BUSY line to FSMC_WAIT,
    // the FSMC will stall any attempted accesses to the NAND until the
    // chip signals READY.
    // On a read operation, the chip asserts BUSY tWHBL (100ns) or tWHALL
    // (10ns) after we latch in the last address byte.
    // Therefore we will wait the initial time (1us is overkill) to ensure
    // it is asserted, but don't have to do anything further here.
    cyg_int32 r;

    HAL_DELAY_US(initial);

    CYGHWR_HAL_STM32_GPIO_IN(GPIO_NAND_RB_PIN, &r);
    CYG_ASSERTC(r==1);
}

static void wait_ready_or_status(cyg_nand_device *ctx, CYG_BYTE mask)
{
    // With the jumper set to route the BUSY line to FSMC_WAIT,
    // the FSMC will stall any attempted accesses to the NAND until the
    // chip signals READY.
    //
    // Therefore, we should just be able to read_status once and find
    // that we stall until the operation has completed; IOW, `polls'
    // should be 1 every time.  This has been confirmed experimentally.

    st_nand_wait_for_BUSY(); // this (1us) is overkill
    int polls=0;
    int sta;
    do {
        sta = read_status(ctx);
        HAL_DELAY_US(10);
        ++polls;
    } while (!(sta & mask));
    NAND_CHATTER(8, ctx, "wait_status: pollcount %d status 0x%02x\n", polls, sta);
}

#else // CYGHWR_HAL_CORTEXM_STM3210E_EVAL_RB_ON_INT2
/* Case 2: The RB line is connected to INT2. ------------------------ */

// Common code

/* Polls the !BUSY line. Returns 1 if ready, 0 if busy. */
static inline int is_chip_ready(struct _mypriv *ctx)
{
    cyg_int32 rv;
    CYGHWR_HAL_STM32_GPIO_IN(GPIO_NAND_RB_PIN, &rv);
    return rv;
}

/* Polling loop, does not return until the chip is READY */
static void wait_ready_polled(cyg_nand_device *ctx)
{
    int polls=0;
    GET_MYPRIV(ctx, priv);
    while (0==is_chip_ready(priv)) {
        HAL_DELAY_US(CYGNUM_HAL_CORTEXM_STM3210E_EVAL_POLL_INTERVAL);
        ++polls;
    }
    NAND_CHATTER(8, ctx, "!BUSY: pollcount %d\n",polls);
}

/* TODO: Code to achieve interrupt-driven sleep, as opposed to polling,
 * would go here, enabled by its own CDL option.
 * At the time of writing we have been unable to get this to work
 * properly; the interrupt controller doesn't seem to detect the
 * movement of the NAND RB line. */

static void wait_ready_or_time(cyg_nand_device *ctx,
                               size_t wait_init, size_t wait_fallback)
{
    wait_ready_polled(ctx);
}

static void wait_ready_or_status(cyg_nand_device *ctx, CYG_BYTE mask)
{
    wait_ready_polled(ctx);
}

#endif // ..._RB_ON_INT2

static int nandxxxx3a_plf_init(cyg_nand_device *dev)
{
    CYG_ADDRESS base;
    cyg_uint32 reg;

    // Setup CLE, ALE, D0..D3, NOE, NWE, NCE2 pins
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(D, 0, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(D, 1, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(D, 4, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(D, 5, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(D, 7, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(D,11, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(D,12, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(D,14, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(D,15, OUT_50MHZ, AOPP));

    // Setup D4..D7 pins
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(E, 7, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(E, 8, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(E, 9, OUT_50MHZ, AOPP));
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_GPIO(E,10, OUT_50MHZ, AOPP));

    // Setup the relevant Ready/!Busy inputs as GPIO
    CYGHWR_HAL_STM32_GPIO_SET(GPIO_NAND_RB_PIN);

    // Setup FSMC for NAND
    base = CYGHWR_HAL_STM32_FSMC;
    reg = 0x01020301;
    //reg = 0x0f0f0f0f;
    HAL_WRITE_UINT32(base + CYGHWR_HAL_STM32_FSMC_PMEM2, reg);
    HAL_WRITE_UINT32(base + CYGHWR_HAL_STM32_FSMC_PATT2, reg);
    reg =
          CYGHWR_HAL_STM32_FSMC_PCR_PWAITEN |
          CYGHWR_HAL_STM32_FSMC_PCR_PTYP_NAND |
          CYGHWR_HAL_STM32_FSMC_PCR_PWID_8 |
          CYGHWR_HAL_STM32_FSMC_PCR_TCLR(0) |
          CYGHWR_HAL_STM32_FSMC_PCR_TAR(0) |
          CYGHWR_HAL_STM32_FSMC_PCR_ECCPS_512;
    HAL_WRITE_UINT32(base + CYGHWR_HAL_STM32_FSMC_PCR2, reg);
    reg |= CYGHWR_HAL_STM32_FSMC_PCR_PBKEN;
    HAL_WRITE_UINT32(base + CYGHWR_HAL_STM32_FSMC_PCR2, reg);

    /* Having just twiddled the FSMC, if we immediately try to talk to
     * the NAND we sometimes (pretty reliably via certain code paths
     * when compiled with -O2, it seems) die with a SIGBUS.
     * This seemingly ineffectual talisman (which, incidentally,
     * first manifested itself as "volatile int i = 42;" is all that's
     * needed to perturb it away. */
    asm("nop");

#ifdef INITHOOK
    INITHOOK(dev);
#endif

    return 0;
}

/* Partition support ===================================================
 * Without Manual config, developer has to set up the partitions list
 * by hand. */
#ifdef CYGSEM_DEVS_NAND_STM3210E_EVAL_PARTITION_MANUAL_CONFIG

static inline int stm3210e_init_manual_partition(cyg_nand_device *dev)
{
#define PARTITION(i) do {                                               \
    cyg_nand_block_addr                                                 \
        base = CYGNUM_DEVS_NAND_STM3210E_EVAL_PARTITION_ ## i ## _BASE, \
        size = CYGNUM_DEVS_NAND_STM3210E_EVAL_PARTITION_ ## i ## _SIZE; \
    dev->partition[i].dev = dev;                                        \
    dev->partition[i].first = base;                                     \
    dev->partition[i].last = size ?                                     \
                base + size - 1: (1<<dev->blockcount_bits)-1;           \
} while(0)

#ifdef CYGPKG_DEVS_NAND_STM3210E_EVAL_PARTITION_0
    PARTITION(0);
#endif
#ifdef CYGPKG_DEVS_NAND_STM3210E_EVAL_PARTITION_1
    PARTITION(1);
#endif
#ifdef CYGPKG_DEVS_NAND_STM3210E_EVAL_PARTITION_2
    PARTITION(2);
#endif
#ifdef CYGPKG_DEVS_NAND_STM3210E_EVAL_PARTITION_3
    PARTITION(3);
#endif
    /* Oh for the ability to write a for-loop in pre-processor,
       or a more powerful libcdl ... */
    return 0;
}

#endif // CYGSEM_DEVS_NAND_STM3210E_EVAL_PARTITION_MANUAL_CONFIG

static int nandxxxx3a_plf_partition_setup(cyg_nand_device *dev)
{
#ifdef CYGSEM_DEVS_NAND_STM3210E_EVAL_PARTITION_MANUAL_CONFIG
    return stm3210e_init_manual_partition(dev);
#else
    return 0;
#endif
}

/* Concurrent access protection ======================================== */

// ... is not required on this platform.
// (On some boards, chips might share a chip-select line, CPLD or similar.)

static inline void nandxxxx3a_devlock(cyg_nand_device *dev)
{
}

static inline void nandxxxx3a_devunlock(cyg_nand_device *dev)
{
}

/* Hardware ECC ======================================================== */
// TODO This could live in the variant HAL?

static void stm3210e_ecc_init(cyg_nand_device *dev)
{
    cyg_uint32 cur;
    CYG_ADDRWORD reg = CYGHWR_HAL_STM32_FSMC + CYGHWR_HAL_STM32_FSMC_PCR2;

    HAL_READ_UINT32(reg, cur);
    cur |= CYGHWR_HAL_STM32_FSMC_PCR_ECCEN;
    HAL_WRITE_UINT32(reg, cur);
}

static void stm3210e_ecc_calc(cyg_nand_device *dev, const CYG_BYTE *dat, CYG_BYTE *ecc)
{
    cyg_uint32 code,set;
    CYG_ADDRWORD eccr= CYGHWR_HAL_STM32_FSMC + CYGHWR_HAL_STM32_FSMC_ECCR2,
                 ctrr= CYGHWR_HAL_STM32_FSMC + CYGHWR_HAL_STM32_FSMC_PCR2;
    HAL_READ_UINT32(eccr, code);
    code = ~code; // So an all-0xFF page has an ECC of 0xFF

    ecc[0] = code & 0xFF;
    ecc[1] = (code >> 8) & 0xFF;
    ecc[2] = (code >>16) & 0xFF;
    // The resultant ordering is therefore:
    // byte 0: P8, P4, P2, P1 pairs (P1 at LSB)
    // byte 1: P128, P64, P32, P16
    // byte 2: P2048, P1024, P512, P256

    HAL_READ_UINT32(ctrr, set);
    set &= ~CYGHWR_HAL_STM32_FSMC_PCR_ECCEN;
    HAL_WRITE_UINT32(ctrr, set);
}

static int stm3210e_ecc_repair(cyg_nand_device *dev,
                            CYG_BYTE *dat, size_t nbytes,
                            CYG_BYTE *read_ecc, const CYG_BYTE *calc_ecc)
{
    unsigned d1, d2, d3;

    d1 = calc_ecc[0] ^ read_ecc[0];
    d2 = calc_ecc[1] ^ read_ecc[1];
    d3 = calc_ecc[2] ^ read_ecc[2];

    if((d1|d2|d3) == 0)
        return 0; // Nothing to do.

    // Can we fix it? 12 bits should be set, and as they're stored
    // in adjacent pairs within each byte we can use this neat XOR
    // trick to make sure that precisely one bit of each pair is set.
    unsigned a, b, c;
    a = (d1 ^ (d1 >> 1)) & 0x55;
    b = (d2 ^ (d2 >> 1)) & 0x55;
    c = (d3 ^ (d3 >> 1)) & 0x55;

    if ((a==b)&&(b==c)&&(c==0x55)) {
        // Yes we can ! Figure out the offset.
        unsigned byte, bit; 

        bit = ( (d1 >> 1) & 1) |        // P1
              ( (d1 >> 2) & 2) |        // P2
              ( (d1 >> 3) & 4);         // and P4.

        byte = ( (d1 >> 7) & 1) |       // P8
               ( (d2 >> 0) & 2) |       // P16
               ( (d2 >> 1) & 4) |       // P32
               ( (d2 >> 2) & 8) |       // P64
               ( (d2 >> 3) &16) |       // P128
               ( (d3 << 4) &32) |       // P256
               ( (d3 << 3) &64) |       // P512
               ( (d3 << 2)&128) |       // P1024
               ( (d3 << 1)&256);        // P2048

        if (byte < nbytes)
            dat[byte] ^= (1<<bit);

        return 1;
    }

    // No? Is it an ECC dropout? Count the bits...
    unsigned i=0;
    while (d1) {
        if (d1 & 1) ++i;
        d1 >>= 1;
    }
    while (d2) {
        if (d2 & 1) ++i;
        d2 >>= 1;
    }
    while (d3) {
        if (d3 & 1) ++i;
        d3 >>= 1;
    }
    if (i==1) {
        read_ecc[0] = calc_ecc[0];
        read_ecc[1] = calc_ecc[1];
        read_ecc[2] = calc_ecc[2];
        return 2;
    }
    // Alas, it is uncorrectable.
    return -1;
}

static CYG_NAND_ECC_ALG_HW(stm3210e_ecc, 512, 3, stm3210e_ecc_init, stm3210e_ecc_calc, stm3210e_ecc_repair);

// Need to use a slightly modified OOB layout to satisfy nand_lookup's paranoia checks
static const cyg_nand_oob_layout stm3210e_oob = {
    .ecc_size = 3,
    .ecc = { { .pos=0, .len=3 } },
    /* 3, 6, 7 would be ECC in the Linux MTD world. 4/5 are avoided. */
    .app_size = 8,
    .app = { { .pos=3, .len=1 }, { .pos=6, .len=10} },
};

/* Putting it all together ... ========================================= */

#include <cyg/devs/nand/nandxxxx3a.inl>

NANDXXXX3A_DEVICE(stm3210e_nand, "onboard", 512, &_stm3210_nand_priv,
                  &stm3210e_ecc, &stm3210e_oob);

