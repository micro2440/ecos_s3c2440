//=============================================================================
//
//      nand.c
//
//      Main application interface for the eCos NAND flash library
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
// Date:        2009-03-02
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/hal/drv_api.h> // mutexes
#include <cyg/infra/diag.h>
#include <cyg/infra/cyg_ass.h>

#include <cyg/nand/nand.h>
#include <cyg/nand/nand_device.h>
#include <cyg/nand/nand_devtab.h>
#include <cyg/nand/util.h>
#include "nand_bbt.h"
#include CYGBLD_ISO_ERRNO_CODES_HEADER
#include <string.h>

/* ============================================================ */
// Timing instrumentation hooks; or "where is all the time going?"
// timetag_to_csv.pl will turn gdb output ("p tagslist") into CSV,
// for ease of analysis.

#ifdef CYGSEM_IO_NAND_INSTRUMENT_TIMING
# ifndef HAL_CLOCK_READ
#  error HAL_CLOCK_READ required
# endif
# define TAGSIZE 1024
cyg_uint32 tagslist[TAGSIZE];
cyg_uint32 tags_next;
static cyg_int64        rtc_resolution[] = CYGNUM_KERNEL_COUNTERS_RTC_RESOLUTION;
static cyg_int64        rtc_period = CYGNUM_KERNEL_COUNTERS_RTC_PERIOD;

# define TAG(_x) do {                           \
    tagslist[tags_next++] = _x;                 \
    if (tags_next >= TAGSIZE) tags_next = 0;    \
} while(0)

# define TIMETAG() do {     \
    cyg_uint64 tick;        \
    cyg_uint32 haltick;     \
    tick=cyg_current_time();\
    HAL_CLOCK_READ(&haltick);\
    TAG(__LINE__);          \
    TAG(tick&0xFFFFFFFF);   \
    TAG(haltick);           \
} while(0)

# define TIMETAG_INIT() do {        \
    int _i;                         \
    for (_i=0; _i<TAGSIZE; _i++)    \
        tagslist[_i]=0;             \
    tags_next = 0;                  \
    cyg_uint32 _tt, _tu;            \
    HAL_CLOCK_READ(&_tt);           \
    _tu = _tt;                      \
    while (_tt == _tu)              \
        HAL_CLOCK_READ(&_tu);       \
    TIMETAG();                      \
    (void) rtc_period;              \
    (void) rtc_resolution;          \
} while(0)

#else // ! CYGSEM_IO_NAND_INSTRUMENT_TIMING

# define TIMETAG_INIT() CYG_EMPTY_STATEMENT
# define TIMETAG() CYG_EMPTY_STATEMENT

#endif

/* ============================================================ */

/* We have a global ("devinit") lock, protects the nanddevtab and all 
 * writes to device->isInited.
 *
 * Each device also has its own lock, which protects the rest of
 * the structure, and the hardware itself.
 *
 * Rules:
 * 1. Nothing can use a device until it has been fully initialised 
 * (i.e. they must check dev->isInited, which is not set until
 * initialisation is complete);
 * 2. All calls to a device which has been initialised must acquire
 * the device's lock, to prevent multiple access;
 * 3. A repeated call to initialise a device is a safe no-op (under the
 * global lock, see that isInited is set, so do nothing);
 * 4. It is not possible for multiple threads to simultaneously 
 * initialise the same device [via the lookup function]
 * (they are protected by the global lock - the second blocks
 * until the first has completed, at which point the second thread 
 * sees that the device has been inited and falls out into a no-op).
 *
 * Therefore, an operation in devinit which uses the chip need not
 * assert the per-device lock, as nothing else can use the chip until
 * it has completed and set isInited to 1.
 */

static cyg_drv_mutex_t devinit_lock;

#define LOCK_devinit() cyg_drv_mutex_lock(&devinit_lock)
#define UNLOCK_devinit() cyg_drv_mutex_unlock(&devinit_lock)

/* Per-device lock.
 * NB that if multiple devices might interact (e.g. sharing a CPLD),
 * their drivers must act in concert to prevent this, usually at the 
 * platform level. */
#define LOCK_DEV(dev)   cyg_drv_mutex_lock(&dev->devlock)
#define UNLOCK_DEV(dev) cyg_drv_mutex_unlock(&dev->devlock)

/* ============================================================ */
/* Initialisation and lookup */

__externC void cyg_nand_bbt_initx(void);
static cyg_nand_printf nand_default_pf;

// This is called only by the C++ static constructor. Applications
// never need to call this themselves.
__externC void cyg_nand_initx(cyg_nand_printf pf)
{
    if (pf) CYG_CHECK_FUNC_PTRC(pf);
    cyg_drv_mutex_init(&devinit_lock);
    cyg_nand_bbt_initx();
    nand_default_pf = pf;
}

__externC
cyg_nand_partition* cyg_nand_get_partition(cyg_nand_device *dev, unsigned partno)
{
    if ((partno < 0) || (partno >= CYGNUM_NAND_MAX_PARTITIONS))
        return NULL;
    LOCK_DEV(dev);
    cyg_nand_partition *rv = &(dev->partition[partno]);
    UNLOCK_DEV(dev);
    if (!rv->dev) return NULL; /* partition inactive */
    return rv;
}

__externC
int cyg_nand_lookup(const char *devname, cyg_nand_device **dev_o)
{
    int rv = -ENOENT;
    if (dev_o) {
        CYG_CHECK_DATA_PTRC(dev_o);
        *dev_o = 0;
    }
    if (!devname) return -EINVAL;

    LOCK_devinit(); // ++++++++++++++++++++++++++++++++++++++++++++
    cyg_nand_device *dev;
    for (dev = &cyg_nanddevtab[0]; dev != &cyg_nanddevtab_end; dev++) {
        if (0==strcmp(devname, dev->devname)) {
            rv = 0;
            break;
        }
    }
    if (!rv) {
        if (!dev->is_inited) {
            int i;

            if (dev->version != 2) {
                NAND_ERROR(dev, "Device %s declares incompatible version %d (expected 2)", devname, dev->version);
                goto done;
            }
            CYG_CHECK_DATA_PTRC(dev->fns);
            CYG_CHECK_FUNC_PTRC(dev->fns->devinit);

            dev->pf = nand_default_pf;
            for (i=0; i<CYGNUM_NAND_MAX_PARTITIONS; i++)
                dev->partition[i].dev = 0;
#ifdef CYGSEM_IO_NAND_USE_BBT
            dev->bbt.data = 0; // Paranoia, ensure devinit sets up
#endif

            rv = dev->fns->devinit(dev);

            if (rv) {
                NAND_ERROR(dev,"Could not initialise NAND device \"%s\": code %d\n", devname, rv);
                goto done;
            }

            // Now check that we have everything we need
            CYG_CHECK_FUNC_PTRC(dev->fns->read_begin);
            CYG_CHECK_FUNC_PTRC(dev->fns->read_stride);
            CYG_CHECK_FUNC_PTRC(dev->fns->read_finish);
            CYG_CHECK_FUNC_PTRC(dev->fns->write_begin);
            CYG_CHECK_FUNC_PTRC(dev->fns->write_stride);
            CYG_CHECK_FUNC_PTRC(dev->fns->write_finish);

            CYG_CHECK_FUNC_PTRC(dev->fns->erase_block);
            CYG_CHECK_FUNC_PTRC(dev->fns->is_factory_bad);

#ifdef CYGSEM_IO_NAND_USE_BBT
            CYG_CHECK_DATA_PTRC(dev->bbt.data);
#endif
            CYG_CHECK_DATA_PTRC(dev->ecc);
            CYG_CHECK_DATA_PTRC(dev->oob);
            if (!dev->chipsize_log ||
                    !dev->blockcount_bits ||
                    !dev->block_page_bits ||
                    !dev->spare_per_page ||
                    !dev->page_bits ||
#ifdef CYGSEM_IO_NAND_USE_BBT
                    !dev->bbt.data ||
#endif
                    !dev->ecc ||
                    !dev->oob) {
                NAND_ERROR(dev,"BUG: NAND driver devinit did not fill in all required fields - disabling device\n");
                rv = -ENOSYS;
                goto done;
            }

#ifdef CYGSEM_IO_NAND_USE_BBT
            if (dev->bbt.datasize < (1 << (dev->blockcount_bits-2)) ) {
                NAND_ERROR(dev,"BUG: NAND driver declared bbt.data_size isn't big enough (got %lu, want %u) - disabling device\n", (unsigned long) dev->bbt.datasize, (1 << (dev->blockcount_bits-2)));
                rv = -ENOSYS;
                goto done;
            }
#endif

            if ( dev->oob->ecc_size != CYG_NAND_ECCPERPAGE(dev) ) {
                NAND_ERROR(dev,"BUG: NAND driver has inconsistent ECC size declaration (oob says %d, ecc says %d) - disabling device\n", dev->oob->ecc_size, CYG_NAND_ECCPERPAGE(dev));
                rv = -ENOSYS;
                goto done;
            }

            // NOW we are ready to read from the device !

#ifdef CYGSEM_IO_NAND_USE_BBT
            rv = cyg_nand_bbti_find_tables(dev);
            if (rv == -ENOENT) {
                NAND_CHATTER(1,dev, "Creating initial bad block table on device %s\n", devname);
                rv = cyg_nand_bbti_build_tables(dev);
            }
            if (rv != 0) {
                NAND_ERROR(dev,"Cannot find or build BBT (%d)\n", -rv);
                goto done;
            }
#endif

            cyg_drv_mutex_init(&dev->devlock);
            dev->is_inited = 1;

            int live_partitions = 0;
            for (i=0; i<CYGNUM_NAND_MAX_PARTITIONS; i++)
                if (dev->partition[i].dev) ++live_partitions;
            if (live_partitions)
                NAND_CHATTER(1,dev, "%s devinit complete, %u partition%c configured\n", devname, live_partitions, live_partitions==1 ? ' ' : 's' );
            else
                NAND_CHATTER(1,dev, "%s devinit complete, NO partitions configured!\n", devname); // hope they know what they're doing.
        }
        if (dev_o) *dev_o = dev;
    }
done:
    UNLOCK_devinit(); // ------------------------------------------
    return rv;
}

/* ============================================================ */
/* Device access */

#define DEV_INIT_CHECK(dev) do { if (!dev->is_inited) return -ENXIO; } while(0)
#define PARTITION_CHECK(p) do { if (!p->dev) return -ENXIO; } while(0)

// Partition-to-Device and Device-to-Partition address xlation
#define BLOCK_P_TO_D(_part,_block) ((_block) + (_part)->first)
#define BLOCK_D_TO_P(_part,_block) ((_block) - (_part)->first)

#define PAGE_P_TO_D(_part,_page) ((_page) + (_part)->first * CYG_NAND_PAGES_PER_BLOCK((_part)->dev) )
#define PAGE_D_TO_P(_part,_page) ((_page) - (_part)->first * CYG_NAND_PAGES_PER_BLOCK((_part)->dev) )

/* Sanity check helpers: these take a partition and a DEVICE address */
static inline int valid_block_addr(cyg_nand_partition *part, cyg_nand_block_addr block)
{
    return ( (block < part->first) || (block > part->last) ) ? -ENOENT : 0;
}

static int valid_page_addr(cyg_nand_partition *part, cyg_nand_page_addr page)
{
    cyg_nand_block_addr block = CYG_NAND_PAGE2BLOCKADDR(part->dev,page);
    int rv = valid_block_addr(part, block);
    if (rv != 0)
        NAND_CHATTER(1,part->dev, "Invalid attempted access to page %d\n", page);
    return rv;
}

#define EG(what) do { rv = (what); if (rv != 0) goto err_exit; } while(0)

__externC
int cyg_nandp_read_page(cyg_nand_partition *prt, cyg_nand_page_addr ppage,
                void * dest, void * spare, size_t spare_size)
{
    int rv, locked=0;
    TIMETAG_INIT();
    PARTITION_CHECK(prt);
    cyg_nand_device *dev = prt->dev;
    DEV_INIT_CHECK(dev);
    cyg_nand_page_addr page = PAGE_P_TO_D(prt,ppage);

    if (spare_size > dev->spare_per_page) return -EFBIG;

    EG(valid_page_addr(prt, page));

#ifdef CYGSEM_IO_NAND_USE_BBT
    cyg_nand_block_addr blk = CYG_NAND_PAGE2BLOCKADDR(dev,page);
    if (cyg_nand_bbti_query(dev, blk) != CYG_NAND_BBT_OK) {
        NAND_CHATTER(1,dev,"Asked to read page %u in bad block %u\n", page, blk);
        EG(-EINVAL);
    }
#endif

    LOCK_DEV(dev);
    locked = 1;
    EG(nandi_read_whole_page_raw(dev, page, dest, spare, spare_size, 1));

err_exit:
    if (locked) UNLOCK_DEV(dev);
    TIMETAG();
    return rv;
}



__externC
int cyg_nandp_read_part_page(cyg_nand_partition *prt, cyg_nand_page_addr ppage,
                void * dest, size_t offset, size_t length, int check_ecc)

{
    int rv;
    PARTITION_CHECK(prt);
    cyg_nand_device *dev = prt->dev;
    DEV_INIT_CHECK(dev);
    cyg_nand_page_addr page = PAGE_P_TO_D(prt,ppage);
    CYG_BYTE *pagebuffer;
    int got_pagebuf = 0, locked = 0;

    if (offset + length > CYG_NAND_BYTES_PER_PAGE(dev)) return -EFBIG;

    EG(valid_page_addr(prt, page));

#ifdef CYGSEM_IO_NAND_USE_BBT
    cyg_nand_block_addr blk = CYG_NAND_PAGE2BLOCKADDR(dev,page);
    if (cyg_nand_bbti_query(dev, blk) != CYG_NAND_BBT_OK) {
        NAND_CHATTER(1,dev,"Asked to read page %u in bad block %u\n", page, blk);
        EG(-EINVAL);
    }
#endif

    if (length==0) return 0;

    LOCK_DEV(dev);
    locked = 1;

    if (dev->fns->read_part_page && !check_ecc) {
        EG(dev->fns->read_part_page(dev, dest, page, offset, length));
    } else {
        pagebuffer = nandi_grab_pagebuf();
        got_pagebuf = 1;
        EG(nandi_read_whole_page_raw(dev, page, pagebuffer, 0, 0, check_ecc));
        if (dest)
            memcpy(dest, &pagebuffer[offset], length);
    }

err_exit:
    if (got_pagebuf) nandi_release_pagebuf();
    if (locked) UNLOCK_DEV(dev);
    return rv;
}


/* Internal, mostly-unchecked interface to read a page. 
 * Takes a DEVICE address.
 * Caller must hold the devlock! */
int nandi_read_whole_page_raw(cyg_nand_device *dev, cyg_nand_page_addr page,
            CYG_BYTE * dest, CYG_BYTE * spare, size_t spare_size,
            int check_ecc)
{
    CYG_BYTE ecc_read[CYG_NAND_ECCPERPAGE(dev)],
             ecc_calc[CYG_NAND_ECCPERPAGE(dev)];
    CYG_BYTE *ecc_calc_p, *ecc_read_p;

    CYG_BYTE oob_buf[dev->spare_per_page];
    int rv=0,tries=0;
    size_t remain;
    const int do_hw_ecc = (dev->ecc->flags & NAND_ECC_FLAG_IS_HARDWARE) && check_ecc;

    // Stride for reading from device:
    const unsigned read_data_stride = do_hw_ecc ? dev->ecc->data_size : CYG_NAND_BYTES_PER_PAGE(dev);
    // Stride for calculating/checking/repairing ECC:
    const unsigned ecc_data_stride = dev->ecc->data_size;
    // Stride within the ECC data
    const unsigned ecc_stride = dev->ecc->ecc_size;

    if (dest)  CYG_CHECK_DATA_PTRC(dest);
    if (spare) CYG_CHECK_DATA_PTRC(spare);

    CYG_ASSERTC(CYG_NAND_BYTES_PER_PAGE(dev) % ecc_data_stride == 0);

    do {
        CYG_BYTE *data_dest = dest;
        CYG_BYTE *ecc_dest = ecc_calc;

        ++tries;
        remain = CYG_NAND_BYTES_PER_PAGE(dev);

        TIMETAG();
        EG(dev->fns->read_begin(dev, page));
        TIMETAG();

        if (dest) {
            int step;
            while (remain) {
                step = read_data_stride;
                CYG_ASSERTC(remain >= read_data_stride);

                TIMETAG();
                if (do_hw_ecc && dev->ecc->init) dev->ecc->init(dev);
                EG(dev->fns->read_stride(dev, data_dest, read_data_stride));
                TIMETAG();

                if (do_hw_ecc) {
                    dev->ecc->calc_rd(dev, 0, ecc_dest);
                    ecc_dest += ecc_stride;
                    TIMETAG();
                }

                data_dest += read_data_stride;
                remain -= read_data_stride;
            }

            memset(oob_buf, 0xff, CYG_NAND_SPARE_PER_PAGE(dev));
            TIMETAG();
            EG(dev->fns->read_finish(dev, oob_buf, dev->spare_per_page));
            TIMETAG();
            nand_oob_unpack(dev, spare, spare_size, ecc_read, oob_buf);

            if (check_ecc) {
                TIMETAG();
                if (!do_hw_ecc) {
                    // Calculate software ECC in one go to try and take
                    // advantage of the cache.
                    data_dest = dest;
                    remain = CYG_NAND_BYTES_PER_PAGE(dev);
                    ecc_calc_p = ecc_calc;

                    while (remain) {
                        if (dev->ecc->init) dev->ecc->init(dev);
                        dev->ecc->calc_rd(dev, data_dest, ecc_calc_p);

                        remain -= ecc_data_stride;
                        data_dest += ecc_data_stride;
                        ecc_calc_p += ecc_stride;
                    }
                }

                // Now repair ...
                rv = 0;
                int step_rv;
                remain = CYG_NAND_BYTES_PER_PAGE(dev);
                data_dest = dest;

                ecc_calc_p = ecc_calc;
                ecc_read_p = ecc_read;

                while (remain) {
                    CYG_ASSERTC(remain >= ecc_data_stride);

                    step_rv = dev->ecc->repair(dev,data_dest,ecc_data_stride,ecc_read_p,ecc_calc_p);
                    if (step_rv == -1) {
                        rv = -1;
                        break;
                    }
                    rv |= step_rv;

                    data_dest += ecc_data_stride;
                    ecc_read_p += ecc_stride;
                    ecc_calc_p += ecc_stride;
                    remain -= ecc_data_stride;
                }
                TIMETAG();
            }
        } else { // !dest: very simple case
            TIMETAG();
            EG(dev->fns->read_finish(dev, oob_buf, dev->spare_per_page));
            TIMETAG();
            rv = 0;
            nand_oob_unpack(dev, spare, spare_size, ecc_read, oob_buf);
        }

        if (rv==-1 && (tries < CYGNUM_NAND_MAX_READ_RETRIES) ) {
            NAND_CHATTER(4, dev, "ECC uncorrectable error on read, retrying\n");
        }
    } while (rv==-1 && (tries < CYGNUM_NAND_MAX_READ_RETRIES) );

    switch (rv) {
        case 0:
            NAND_CHATTER(8,dev,"Read page %u OK\n", page);
            break;
        case 1:
        case 2:
        case 3:
            NAND_CHATTER(2,dev, "Page %u ECC correction, type %d\n", page,rv);
            rv=0;
            break;
        case -1:
        default:
            NAND_ERROR(dev,"Page %u read gave ECC uncorrectable error\n", page);
            rv=-EIO;
            break;
    }
err_exit:
    return rv;
}

__externC
int cyg_nandp_write_page(cyg_nand_partition *prt, cyg_nand_page_addr ppage,
        const void * src, const void * spare, size_t spare_size)
{
    int rv, locked = 0;
    TIMETAG_INIT();
    PARTITION_CHECK(prt);
    cyg_nand_device *dev = prt->dev;
    DEV_INIT_CHECK(dev);
    cyg_nand_page_addr page = PAGE_P_TO_D(prt,ppage);

    EG(valid_page_addr(prt, page));

#ifdef CYGSEM_IO_NAND_USE_BBT
    cyg_nand_block_addr blk = CYG_NAND_PAGE2BLOCKADDR(dev,page);
    if (cyg_nand_bbti_query(dev, blk) != CYG_NAND_BBT_OK) {
        NAND_CHATTER(1,dev,"Asked to write page %u in bad block %u\n", page, blk);
        EG(-EINVAL);
    }
#endif
    LOCK_DEV(dev);
    locked = 1;
    EG(nandi_write_page_raw(dev, page, src, spare, spare_size));

err_exit:
    if (locked) UNLOCK_DEV(dev);
    TIMETAG();
    return rv;
}

/* Internal, mostly-unchecked interface to write a page. 
 * Takes a DEVICE address. */
__externC
int nandi_write_page_raw(cyg_nand_device *dev, cyg_nand_page_addr page,
        const CYG_BYTE * src, const CYG_BYTE * spare, size_t spare_size)
{
#ifdef CYGSEM_IO_NAND_READONLY
    return -EROFS;
#else
    int rv;
    CYG_BYTE oob_packed[dev->spare_per_page];
    CYG_BYTE ecc[CYG_NAND_ECCPERPAGE(dev)];

    const int do_hw_ecc = (dev->ecc->flags & NAND_ECC_FLAG_IS_HARDWARE);
    const unsigned write_data_stride = do_hw_ecc ? dev->ecc->data_size : CYG_NAND_BYTES_PER_PAGE(dev);
    const unsigned ecc_data_stride = dev->ecc->data_size;
    const unsigned ecc_stride = dev->ecc->ecc_size;
    size_t remain;
    CYG_BYTE *ecc_dest = ecc;

    memset(ecc, 0xff, CYG_NAND_ECCPERPAGE(dev));

    if (src)   CYG_CHECK_DATA_PTRC(src);
    if (spare) CYG_CHECK_DATA_PTRC(spare);
    CYG_ASSERTC(CYG_NAND_BYTES_PER_PAGE(dev) % ecc_data_stride == 0);

    TIMETAG();
    // If we're doing software ECC, do it all in one go now.
    if (src && !do_hw_ecc) {
        const CYG_BYTE *data_src = src;
        remain = CYG_NAND_BYTES_PER_PAGE(dev);

        while (remain) {
            CYG_ASSERTC(remain >= ecc_data_stride);
            if (dev->ecc->init) dev->ecc->init(dev);
            dev->ecc->calc_wr(dev, data_src, ecc_dest);

            remain -= ecc_data_stride;
            data_src += ecc_data_stride;
            ecc_dest += ecc_stride;
        }
    }
    TIMETAG();

    EG(dev->fns->write_begin(dev, page));
    TIMETAG();
    if (src) {
        remain = CYG_NAND_BYTES_PER_PAGE(dev);
        while (remain) {
            CYG_ASSERTC(remain >= write_data_stride);
            TIMETAG();
            if (do_hw_ecc && dev->ecc->init) dev->ecc->init(dev);
            EG(dev->fns->write_stride(dev, src, write_data_stride));
            TIMETAG();
            if (do_hw_ecc) {
                dev->ecc->calc_wr(dev, 0, ecc_dest);
                ecc_dest += ecc_stride;
            }
            src += write_data_stride;
            remain -= write_data_stride;
        }
    }
    TIMETAG();

    nand_oob_pack(dev, spare, spare_size, ecc, oob_packed);
    NAND_CHATTER(8,dev,"Write page %u\n", page);
    EG(dev->fns->write_finish(dev, oob_packed, dev->spare_per_page));
    TIMETAG();

    /* N.B. We don't read-back to verify; drivers may do so themselves if
     * they wish. Typically the spec sheet says that a read-back test
     * is unnecessary if the device reports a successful program, and 
     * if ECC is being used. */

err_exit:
    return rv;
#endif
}

__externC
int cyg_nandp_erase_block(cyg_nand_partition *prt, cyg_nand_block_addr pblk)
{
#ifdef CYGSEM_IO_NAND_READONLY
    return -EROFS;
#else
    PARTITION_CHECK(prt);
    cyg_nand_device *dev = prt->dev;
    int rv;
    DEV_INIT_CHECK(dev);
    cyg_nand_block_addr blk = BLOCK_P_TO_D(prt, pblk);

    TIMETAG();
    LOCK_DEV(dev);
    TIMETAG();

    EG(valid_block_addr(prt, blk));
#ifdef CYGSEM_IO_NAND_USE_BBT
    if (cyg_nand_bbti_query(dev, blk) != CYG_NAND_BBT_OK) {
        NAND_CHATTER(1,dev,"Asked to erase bad block %u\n", blk);
        EG(-EINVAL);
    }
#endif
    NAND_CHATTER(8,dev,"Erasing block %u\n", blk);
    rv = dev->fns->erase_block(dev, blk);
    if (rv==-EIO) {
#ifdef CYGSEM_IO_NAND_USE_BBT
        cyg_nand_bbti_markbad(dev, blk); // deliberate ignore
#endif
        EG(rv);
    }
err_exit:
    TIMETAG();
    UNLOCK_DEV(dev);
    TIMETAG();
    return rv;
#endif
}

__externC
int cyg_nandp_bbt_query(cyg_nand_partition *prt, cyg_nand_block_addr pblk)
{
#ifdef CYGSEM_IO_NAND_USE_BBT
    int rv;
    PARTITION_CHECK(prt);
    cyg_nand_device *dev = prt->dev;
    DEV_INIT_CHECK(dev);
    cyg_nand_block_addr blk = BLOCK_P_TO_D(prt, pblk);
    LOCK_DEV(dev);
    EG(valid_block_addr(prt, blk));
    rv = cyg_nand_bbti_query(dev, blk);
err_exit:
    UNLOCK_DEV(dev);
    return rv;
#else
    return -ENOSYS;
#endif
}

__externC
int cyg_nandp_bbt_markbad(cyg_nand_partition *prt, cyg_nand_block_addr pblk)
{
#ifdef CYGSEM_IO_NAND_USE_BBT
    int rv;
    PARTITION_CHECK(prt);
    cyg_nand_device *dev = prt->dev;
    DEV_INIT_CHECK(dev);
    cyg_nand_block_addr blk = BLOCK_P_TO_D(prt, pblk);
    LOCK_DEV(dev);
    EG(valid_block_addr(prt, blk));
    EG(cyg_nand_bbti_markbad(dev, blk));
err_exit:
    UNLOCK_DEV(dev);
    return rv;
#else
    return -ENOSYS;
#endif
}

__externC
int cyg_nand_bbt_markbad_pageaddr(cyg_nand_partition *prt, cyg_nand_page_addr ppg)
{
#ifdef CYGSEM_IO_NAND_USE_BBT
    int rv;
    PARTITION_CHECK(prt);
    cyg_nand_device *dev = prt->dev;
    DEV_INIT_CHECK(dev);
    cyg_nand_block_addr pblk = CYG_NAND_PAGE2BLOCKADDR(dev, ppg);
    cyg_nand_block_addr blk = BLOCK_P_TO_D(prt, pblk);
    EG(valid_block_addr(prt, blk));
    LOCK_DEV(dev);
    EG(cyg_nand_bbti_markbad(dev, blk));
err_exit:
    UNLOCK_DEV(dev);
    return rv;
#else
    return -ENOSYS;
#endif
}

/* Computes the ECC for a whole device page.
 * This is intended for use with software ECC only! (The calc_wr
 * function will be used.)
 *
 * 'page' points to the data; a whole page will necessarily be read.
 * The computed ECC will be stored in 'ecc_o'; CYG_NAND_ECCPERPAGE(dev)
 * bytes will be written. */
void nand_ecci_calc_page(cyg_nand_device *dev, const CYG_BYTE *page, CYG_BYTE *ecc_o)
{
    int i;
    const int nblocks = (1<<dev->page_bits) / dev->ecc->data_size;

    CYG_CHECK_DATA_PTRC(page);
    CYG_CHECK_DATA_PTRC(ecc_o);

    for (i=0; i<nblocks; i++) {
        if (dev->ecc->init)
            dev->ecc->init(dev);
        dev->ecc->calc_wr(dev,page,ecc_o);
        page += dev->ecc->data_size;
        ecc_o += dev->ecc->ecc_size;
    }
}

/* Checks and (if necessary) repairs the ECC for (up to) a whole device page.
 * 'page' points to the data; an error at position after @nbytes@ will not
 * be corrected.
 * Broadly the same semantics as for cyg_nand_ecc_t.repair; 
 * both ECCs are of size CYG_NAND_ECCPERPAGE(dev), and ecc_read may
 * be corrected as well as the data.
 * Returns:
 *      0 for no errors
 *      1 if there was at least one corrected data error
 *      2 if there was at least one corrected ECC error
 *      3 if there was at least one corrected error in both data and ECC
 *     -1 if there was an uncorrectable error (>1 bit in a single ECC block)
 */
int nand_ecci_repair_page(cyg_nand_device *dev, CYG_BYTE *page, size_t remain,
        CYG_BYTE *ecc_read, const CYG_BYTE *ecc_calc)
{
    int i, page_rv=0;
    const int nblocks = (1<<dev->page_bits) / dev->ecc->data_size;
    CYG_CHECK_DATA_PTRC(page);
    CYG_CHECK_DATA_PTRC(ecc_read);
    CYG_CHECK_DATA_PTRC(ecc_calc);

    for (i=0; i<nblocks; i++) {
        int stride = dev->ecc->data_size;
        if (stride > remain) stride = remain;
        int chunk_rv = dev->ecc->repair(dev,page,stride,ecc_read,ecc_calc);
        if (chunk_rv < 0) return chunk_rv;
        page_rv |= chunk_rv;
        page += dev->ecc->data_size;
        remain -= dev->ecc->data_size;
        ecc_read += dev->ecc->ecc_size;
        ecc_calc += dev->ecc->ecc_size;
    }
    return page_rv;
}

