//=============================================================================
//
//      ea_lpc2468_nand.c
//
//      NAND flash support for the Embedded Artists LPC2468 OEM board.
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
// Author(s):   wry
// Date:        2009-03-03
//
//####DESCRIPTIONEND####
//=============================================================================

#include <pkgconf/hal_arm_lpc24xx_ea2468.h>

#include <cyg/nand/nand_device.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>

#include <cyg/devs/nand/k9_generic.h>

/* Private structs. We need one k9_priv (and one of our privs) per
 * instance of the chip. */

struct _mypriv {
#ifdef CYGHWR_HAL_ARM_LPC2XXX_EA_LPC2468_NAND_RDY_USE_INTERRUPT
    cyg_drv_mutex_t cvmux; // Protects CV
    cyg_drv_cond_t  cv;  // Used to sleep on interrupt
    cyg_handle_t    inthdl; // interrupt handle
    cyg_interrupt   intr; // interrupt object

    volatile int ready;  // Set when interrupt fires. Protected by DSR lock.
#else
    char dummy; // shush, gcc
#endif
};

static struct _mypriv _ea_lpc2468_nand_priv;

#define CAST_MYPRIV(x) ((struct _mypriv *) (x))
#define CAST_K9PRIV(x) ((k9_priv *) (x))
#define GET_MYPRIV(dev,var) struct _mypriv * var = CAST_MYPRIV(CAST_K9PRIV((dev)->priv)->plat_priv);

/* We could use 'priv' to supply the NAND addresses. This might be
 * useful if there were multiple chips on a board, but for simplicity
 * we're going to hard-wire it meantime. */
#define NAND_BASE 0x81000000
#define NAND_CMD  (NAND_BASE | (1<<20))
#define NAND_ADDR (NAND_BASE | (1<<19))

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
    HAL_WRITE_UINT8(NAND_CMD,cmd);
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
#if (CYGINT_HAL_ARM_BIGENDIAN == 1)
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

#define POLL_INTERVAL 10 /* us */

#define POLLCOUNT_COUNTING_LEVEL 7
#if defined(CYGSEM_IO_NAND_DEBUG_LEVEL) && (CYGSEM_IO_NAND_DEBUG_LEVEL >= POLLCOUNT_COUNTING_LEVEL)
#define REPORT_POLLS
#endif

/* Case 1: The NAND_RDY line is not connected. ---------------------- */
#ifndef CYGHWR_HAL_ARM_LPC2XXX_EA_LPC2468_USE_NAND_RDY

// fwd def from the k9 driver:
static inline CYG_BYTE read_status(cyg_nand_device *dev);

static void wait_ready_or_time(cyg_nand_device *ctx, size_t initial, size_t fallback)
{
    NAND_CHATTER(8, ctx, "Waiting %d us for operation\n", initial+fallback);
    HAL_DELAY_US(initial+fallback);
}

static void wait_ready_or_status(cyg_nand_device *dev, CYG_BYTE mask)
{
    // The Ready line won't be ready for at least tWB (100ns), so out of 
    // paranoia we'll wait at least that long before reading Status.

    k9_WAIT_tWB(); // this (1us) is overkill

#ifdef REPORT_POLLS
    int polls=0;
#endif
    int sta;
    do {
        sta = read_status(dev);
        HAL_DELAY_US(POLL_INTERVAL);
#ifdef REPORT_POLLS
        ++polls;
#endif
    } while (!(sta & mask));
#ifdef REPORT_POLLS
    NAND_CHATTER(8, dev, "wait_status: pollcount %d\n",polls);
#endif
}

#else // CYGHWR_HAL_ARM_LPC2XXX_EA_LPC2468_USE_NAND_RDY
/* Case 2: The NAND_RDY line *is* connected. ------------------------ */

// Common code

/* Polls the !BUSY line. Returns 1 if ready, 0 if busy. */
static inline int is_chip_ready(cyg_nand_device *ctx)
{
    cyg_uint32 rv;
    HAL_READ_UINT32(
            CYGARC_HAL_LPC24XX_REG_FIO_BASE + CYGARC_HAL_LPC24XX_REG_FIO2PIN,
            rv);
    rv = rv & (1<<12);
    return rv;
}

static void wait_ready_polled(cyg_nand_device *ctx);

# ifdef CYGHWR_HAL_ARM_LPC2XXX_EA_LPC2468_NAND_RDY_USE_INTERRUPT
/* Case 2A: We are using sleep+interrupt wherever possible. */

#  define NAND_RDY_VECTOR   CYGNUM_HAL_INTERRUPT_EINT2 
#  define NAND_RDY_PRIO     12

/* Sleeps until woken by interrupt on the READY/!BUSY line.
 * Obviously, this doesn't work (in fact is illegal) if the scheduler
 * isn't running, so we fall back to polling the READY line in that case
 * (see wait_ready below). */
static void wait_ready_interrupt(cyg_nand_device *ctx)
{
    GET_MYPRIV(ctx,priv);
    cyg_drv_mutex_lock(&priv->cvmux);
    cyg_drv_dsr_lock();
    priv->ready = 0;
    NAND_CHATTER(8, ctx, "sleep for busy\n");
    cyg_drv_interrupt_unmask(NAND_RDY_VECTOR);
    while (!priv->ready)
        cyg_drv_cond_wait(&priv->cv);
    cyg_drv_interrupt_mask(NAND_RDY_VECTOR);
    cyg_drv_dsr_unlock();
    cyg_drv_mutex_unlock(&priv->cvmux);
}

static
cyg_uint32 nand_rdy_ISR(cyg_vector_t vector, cyg_addrword_t data)
{
    if (vector != NAND_RDY_VECTOR) return 0;
    cyg_drv_interrupt_acknowledge(vector);
    return CYG_ISR_HANDLED|CYG_ISR_CALL_DSR;
}

static
void nand_rdy_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    struct _mypriv *priv = CAST_MYPRIV(data);
    priv->ready=1;
    cyg_drv_cond_broadcast(&priv->cv);
}

static void wait_ready(cyg_nand_device *ctx)
{
    int scheduler_off= (cyg_thread_self() == cyg_thread_idle_thread());
    if (scheduler_off) {
        NAND_CHATTER(7, ctx, "Polling as scheduler is off\n");
        k9_WAIT_tWB();
        wait_ready_polled(ctx);
    } else {
        wait_ready_interrupt(ctx);
    }
}

/* Interrupt mode has high overheads (several hundred us).
 * For quick operations - page reading @ 25us and certain reset cases -
 * it's not worth it on this board, so we just hang around for the
 * prescribed time.
 */
#define WAIT_MIN_INTERRUPTMODE_THRESHOLD 400 /* useconds */

static void wait_ready_or_time(cyg_nand_device *ctx,
                               size_t wait_init, size_t wait_fallback)
{
    /* Page reading is very quick (25us), so it's not worth the
     * overhead of interrupt mode. */
    if (wait_fallback > WAIT_MIN_INTERRUPTMODE_THRESHOLD) {
        wait_ready(ctx);
    } else {
        HAL_DELAY_US(wait_init);
        wait_ready_polled(ctx);
    }
}

static void wait_ready_or_status(cyg_nand_device *ctx, CYG_BYTE mask)
{
    /* These ops are page programming (300-700us) block erase (2-3ms),
     * so if interrupt mode is enabled, we'll use it. */
    wait_ready(ctx);
}


#  define INITHOOK ea_plf_init_interrupt
static int ea_plf_init_interrupt(cyg_nand_device *ctx)
{
    GET_MYPRIV(ctx,priv);

    // Direction -> input
    tweak_pin_register(
            CYGARC_HAL_LPC24XX_REG_FIO_BASE + CYGARC_HAL_LPC24XX_REG_FIO2DIR,
            1 << 12, 0);

    // Function -> EINT2
    tweak_pin_register(
            CYGARC_HAL_LPC24XX_REG_PIN_BASE + CYGARC_HAL_LPC24XX_REG_PINSEL4,
            3 << 24, 1 << 24);

    // Mask -> pin active
    tweak_pin_register(
            CYGARC_HAL_LPC24XX_REG_FIO_BASE + CYGARC_HAL_LPC24XX_REG_FIO2MASK,
            1 << 12, 0);

    cyg_drv_mutex_init(&priv->cvmux);
    cyg_drv_cond_init(&priv->cv, &priv->cvmux);

    cyg_interrupt_configure(NAND_RDY_VECTOR, false /* edge triggered */, true /* high */);
    cyg_drv_interrupt_create(NAND_RDY_VECTOR, NAND_RDY_PRIO, (cyg_addrword_t)priv, nand_rdy_ISR, nand_rdy_DSR, &priv->inthdl, &priv->intr);
    cyg_drv_interrupt_attach(priv->inthdl);
    cyg_drv_interrupt_mask(NAND_RDY_VECTOR);
    return 0;
}

# else
/* Case 2B: We are never using sleep+interrupt. */

static void wait_ready_or_time(cyg_nand_device *ctx,
                               size_t wait_init, size_t wait_fallback)
{
    HAL_DELAY_US(wait_init);
    wait_ready_polled(ctx);
}

static void wait_ready_or_status(cyg_nand_device *ctx, CYG_BYTE mask)
{
    k9_WAIT_tWB();
    wait_ready_polled(ctx);
}

#  define INITHOOK ea_plf_init_nointerrupt
static inline int ea_plf_init_nointerrupt(cyg_nand_device *ctx)
{
    // Direction -> input
    tweak_pin_register(
            CYGARC_HAL_LPC24XX_REG_FIO_BASE + CYGARC_HAL_LPC24XX_REG_FIO2DIR,
            1<<12, 0);

    // Function -> GPIO
    tweak_pin_register(
            CYGARC_HAL_LPC24XX_REG_PIN_BASE + CYGARC_HAL_LPC24XX_REG_PINSEL4,
            3<<24, 0);

    // Mask -> pin active
    tweak_pin_register(
            CYGARC_HAL_LPC24XX_REG_FIO_BASE + CYGARC_HAL_LPC24XX_REG_FIO2MASK,
            1<<12, 0);
    return 0;
    (void)ctx;
}

# endif

/* Polling loop, does not return until the chip is READY.
 * Callers should themselves wait for tWB or other initial time 
 * to ensure that READY is deasserted. */
static void wait_ready_polled(cyg_nand_device *ctx)
{
#ifdef REPORT_POLLS
    int polls=0;
#endif
#ifdef CYGHWR_HAL_ARM_LPC2XXX_EA_LPC2468_NAND_RDY_USE_INTERRUPT
    cyg_drv_interrupt_unmask(NAND_RDY_VECTOR);
#endif
    while (0==is_chip_ready(ctx)) {
        HAL_DELAY_US(POLL_INTERVAL);
#ifdef REPORT_POLLS
        ++polls;
#endif
    }
#ifdef CYGHWR_HAL_ARM_LPC2XXX_EA_LPC2468_NAND_RDY_USE_INTERRUPT
    cyg_drv_interrupt_mask(NAND_RDY_VECTOR);
#endif
#ifdef REPORT_POLLS
    NAND_CHATTER(8, ctx, "!BUSY: pollcount %d\n",polls);
#endif
}

#endif // USE_INTERRUPT ?

static int k9_plf_init(cyg_nand_device *dev)
{
    //GET_MYPRIV(dev,priv);

    // Default platform EMC timings on this board are a bit off for
    // efficient use of the NAND.

    unsigned ns_per_cclk = 1 + 1000000000 / CYGNUM_HAL_ARM_LPC24XX_CLOCK_SPEED;

    // NB! These figures are interpreted from the K9 datasheet.
    // WaitOEN: No delay required
    HAL_WRITE_UINT32(CYGARC_HAL_LPC24XX_REG_EMC_BASE + CYGARC_HAL_LPC24XX_REG_EMCS_WAITO_EN1, 0);

    // WaitRead: 25ns (EMC delays for CCLK * (1+n) )
    unsigned waitread = 25 / ns_per_cclk; // +1 to round up, -1 to account for the implicit CCLK
    HAL_WRITE_UINT32(CYGARC_HAL_LPC24XX_REG_EMC_BASE + CYGARC_HAL_LPC24XX_REG_EMCS_WAITRD1, waitread);

    // WaitWEN: No delay required, use the shortest that the EMC allows
    HAL_WRITE_UINT32(CYGARC_HAL_LPC24XX_REG_EMC_BASE + CYGARC_HAL_LPC24XX_REG_EMCS_WAITW_EN1, 0);
    // WaitWrite: 25ns (EMC delays for CCLK * (2+n) )
    unsigned waitwrite = 25 / ns_per_cclk; // +1 to round up, -2 to account for the implicit 2CCLK, but don't underflow...
    if (waitwrite != 0) --waitwrite;
    HAL_WRITE_UINT32(CYGARC_HAL_LPC24XX_REG_EMC_BASE + CYGARC_HAL_LPC24XX_REG_EMCS_WAITWR1, waitwrite);

    // WaitTurn: 10ns (EMC delays for CCLK * (1+n) )
    unsigned waitturn = 10 / ns_per_cclk; // +1 to round up, -1 to account for the implicit CCLK
    HAL_WRITE_UINT32(CYGARC_HAL_LPC24XX_REG_EMC_BASE + CYGARC_HAL_LPC24XX_REG_EMCS_WAITTURN1, waitturn);


#ifdef INITHOOK
    return INITHOOK(dev);
#else
    return 0;
#endif
}

/* Partition support ===================================================
 * Without Manual config, developer has to set up the partitions list
 * by hand. */
#ifdef CYGSEM_DEVS_NAND_EA_LPC2468_PARTITION_MANUAL_CONFIG

static inline int ea_init_manual_partition(cyg_nand_device *dev)
{
#define PARTITION(i) do {                                               \
    cyg_nand_block_addr                                                 \
        base = CYGNUM_DEVS_NAND_EA_LPC2468_PARTITION_ ## i ## _BASE,    \
        size = CYGNUM_DEVS_NAND_EA_LPC2468_PARTITION_ ## i ## _SIZE;    \
    dev->partition[i].dev = dev;                                        \
    dev->partition[i].first = base;                                     \
    dev->partition[i].last = size ?                                     \
                base + size - 1: (1<<dev->blockcount_bits)-1;           \
} while(0)

#ifdef CYGPKG_DEVS_NAND_EA_LPC2468_PARTITION_0
    PARTITION(0);
#endif
#ifdef CYGPKG_DEVS_NAND_EA_LPC2468_PARTITION_1
    PARTITION(1);
#endif
#ifdef CYGPKG_DEVS_NAND_EA_LPC2468_PARTITION_2
    PARTITION(2);
#endif
#ifdef CYGPKG_DEVS_NAND_EA_LPC2468_PARTITION_3
    PARTITION(3);
#endif
    /* Oh for the ability to write a for-loop in pre-processor,
       or a more powerful libcdl ... */
    return 0;
}
#define PARTITIONHOOK ea_init_manual_partition

#endif // CYGSEM_DEVS_NAND_EA_LPC2468_PARTITION_MANUAL_CONFIG

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
}

static inline void k9_devunlock(cyg_nand_device *dev)
{
}

/* Putting it all together ... ========================================= */

#include <cyg/devs/nand/k9_generic.inl>

static k9_subtype chips[] = { K9F1G08U0x, K9_SUBTYPE_SENTINEL };

static k9_priv _k9_ea_lpc2468_priv = { &chips, &_ea_lpc2468_nand_priv };

CYG_NAND_DEVICE(ea_nand, "onboard", &nand_k9_funs, &_k9_ea_lpc2468_priv,
                &mtd_ecc256_fast, &nand_mtd_oob_64);

