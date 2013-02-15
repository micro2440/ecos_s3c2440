//=============================================================================
//
//      nand_bbt.c
//
//      eCos NAND flash library - bad block table
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

/* This source is intended to be on-chip compatible with the Linux MTD layer.
 * TODO: Multi-chip devices are not yet implemented.
 * ASSUMPTION: 1<<blockcount_bits will fit in an int.
 */

#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/nand/nand.h>
#include <cyg/nand/util.h>
#include "nand_bbt.h"
#include CYGBLD_ISO_ERRNO_CODES_HEADER
#include <string.h>
#include <stdlib.h>

/* ============================================================ */
// We need a global page buffer so we can manipulate the
// Bad Block Table. This has to be big enough for the largest
// sized page we will handle, so each driver has to declare
// it with a _requires_ statement in its CDL.

#if CYGNUM_NAND_PAGEBUFFER == 0
#error CYGNUM_NAND_PAGEBUFFER not set
/* If you've tripped over this error, it means that either there are
 * no NAND drivers present in your config, or they don't correctly 
 * impose a requirement on CYGNUM_NAND_PAGEBUFFER and therefore
 * the logic that uses the global pagebuffer (in nand_bbt.c) could
 * not possibly work correctly.
 */
#endif

static unsigned char bbt_pagebuf[CYGNUM_NAND_PAGEBUFFER];


cyg_drv_mutex_t nand_bbt_pagebuf_lock;

__externC void cyg_nand_bbt_initx(void)
{
    cyg_drv_mutex_init(&nand_bbt_pagebuf_lock);
}

#define LOCK_PAGEBUF()   do {                       \
    cyg_drv_mutex_lock(&nand_bbt_pagebuf_lock);     \
    i_locked = 1;                                   \
} while(0)

#define UNLOCK_PAGEBUF() do {                       \
    cyg_drv_mutex_unlock(&nand_bbt_pagebuf_lock);   \
    i_locked = 0;                                   \
} while(0)

CYG_BYTE* nandi_grab_pagebuf(void)
{
    cyg_drv_mutex_lock(&nand_bbt_pagebuf_lock);
    return &bbt_pagebuf[0];
}

void nandi_release_pagebuf(void)
{
    cyg_drv_mutex_unlock(&nand_bbt_pagebuf_lock);
}

#ifdef CYGSEM_IO_NAND_USE_BBT

#ifndef CYGSEM_IO_NAND_READONLY
static int cyg_nand_bbti_write_tables(cyg_nand_device *dev, int is_initial);
#endif

/* Mapping between on-chip and in-ram statuses ===================== */
static inline cyg_nand_bbt_status_t chip_2_ram(nand_bbti_onchip_status_t in)
{
    switch(in) {
        case BBTI_FACTORY_BAD:  return CYG_NAND_BBT_FACTORY_BAD;
        case BBTI_WORNBAD_1:    return CYG_NAND_BBT_WORNBAD;
        case BBTI_WORNBAD_2:    return CYG_NAND_BBT_WORNBAD;
        case BBTI_OK:           return CYG_NAND_BBT_OK;
    }
    CYG_FAIL("Invalid input to chip_2_ram");
    return CYG_NAND_BBT_FACTORY_BAD;
}

static inline nand_bbti_onchip_status_t ram_2_chip(cyg_nand_bbt_status_t in)
{
    switch(in) {
        case CYG_NAND_BBT_FACTORY_BAD:  return BBTI_FACTORY_BAD;
        case CYG_NAND_BBT_WORNBAD:      return BBTI_WORNBAD_1;
        case CYG_NAND_BBT_RESERVED:     return BBTI_OK;
        case CYG_NAND_BBT_OK:           return BBTI_OK;
    }
    CYG_FAIL("Invalid input to ram_2_chip");
    return BBTI_FACTORY_BAD;
}

/* ============================================================= */

/* Update the in-RAM table, but do _not_ write it out. Unchecked! */
static void bbti_mark_raw(cyg_nand_device *dev, cyg_nand_block_addr blk, 
        cyg_nand_bbt_status_t st)
{
    unsigned offset = blk >> 2;
    unsigned byteshift = (blk & 3) * 2;
    dev->bbt.data[offset] =
        (dev->bbt.data[offset] &~ (3 << byteshift)) | (st << byteshift);
}

int cyg_nand_bbti_query(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    CYG_ASSERT(blk >= 0 && blk <= CYG_NAND_BLOCKCOUNT(dev), "valid block");
    unsigned offset = blk >> 2;
    unsigned byteshift = (blk & 3) * 2;
    return ( dev->bbt.data[offset] >> byteshift ) & 3;
}

int cyg_nand_bbti_markany(cyg_nand_device *dev, cyg_nand_block_addr blk,
                                  cyg_nand_bbt_status_t st)
{
    CYG_ASSERT(blk >= 0 && blk <= CYG_NAND_BLOCKCOUNT(dev), "valid block");
    bbti_mark_raw(dev, blk, st);
#ifdef CYGSEM_IO_NAND_READONLY
    NAND_CHATTER(2,dev,"ignoring mark request on read-only config\n");
    return -ENOSYS;
#else
    int rv = cyg_nand_bbti_write_tables(dev,0);
    if (rv) 
        NAND_ERROR(dev,"nand_bbti_mark %d as %d: %d (%s)\n", blk, st, -rv, strerror(-rv));
    return rv;
#endif
}

int cyg_nand_bbti_markbad(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    CYG_ASSERT(blk >= 0 && blk <= CYG_NAND_BLOCKCOUNT(dev), "valid block");
    return cyg_nand_bbti_markany(dev, blk, CYG_NAND_BBT_WORNBAD);
}


/* Search for, and read in, the BBTs.
 *
 * Algorithm: Start at the last block, work backwards (up to 4 blocks)
 * looking for the signature.
 * DEVICE MUST BE LOCKED.
 * Table descriptor patterns are 'Bbt0' primary / '1tbB' mirror.
 * Descriptors are located at offset 8 of the OOB area of the first
 * page of the block, length 4 bytes.
 * Table version tags are ON by default.
 * Version tag is a _single byte_ at offset 12 within the OOB area of
 * the first page of the block. A freshly created table has version 1;
 * it is incremented whenever the table is updated.
 * (FIXME: What happens after version 255 is not defined.)
 * If both tables are the same version, either may be used.
 * If their versions differ, the higher version is used.
 */

static CYG_BYTE nand_pattern_primary[] = { 'B','b','t','0' };
static CYG_BYTE nand_pattern_mirror [] = { '1','t','b','B' };
#define NAND_PATTERN_OFFSET 8
#define NAND_PATTERN_SIZE 4
#define NAND_VERSION_OFFSET 12
#define NAND_VERSION_SIZE 1

#define EG(x) do { rv = (x); if (rv != 0) goto err_exit; } while(0)

/* Of these two block states, which is worst?
 * "current" is a CYG_NAND_BBT_ (in-RAM) state.
 * "onchip" is a BBTI_ state (on-chip!)
 * Output is in-RAM format (CYG_NAND_BBT_).
 * */
static inline CYG_BYTE worst_of_the_two(CYG_BYTE current, CYG_BYTE onchip)
{
    CYG_ASSERT( (current == (current&3)) && (onchip == (onchip&3)), "invalid input to w_o_t_t");
    /* 
     * OnChip  |  OK     WornBad   Reserved   FactoryBad   < Current
     *         |  00       01       10             11
     *         +---------------------------------------
     *   FB 00 |  FB 11   FB 11     FB  11      FB 11     {
     *         |
     *   WB 01 |  WB 01   WB 01     WB  01      FB 11     { Output 
     *   or 10 |
     *         |
     *   OK 11 |  OK 00   WB 01     Res 10      FB 11     {
     *
     *  ('Reserved' never appears on chip.)
     */
    // Let the compiler take the strain, lookup tables are vv cheap
    switch ( (onchip << 2) | current) {
        // I would have used '0b0000' syntax, but gcc 3 doesn't support it
        case 0: case 1: case 2: case 3:
            return 3;

        case 4: case 8:
        case 5: case 9:
        case 6: case 10:
            return 1;
        case 7: case 11:
            return 3;

        case 12: return 0;
        case 13: return 1;
        case 14: return 2;
        case 15: return 3;
    }
    /* should never get here, indicates bad input and asserts disabled */
    return current; // least bad alternative if asserts disabled
}

static int bbti_incorporate_one(cyg_nand_device *dev, cyg_nand_block_addr blk, CYG_BYTE version)
{
    int rv = 0, i_locked = 0;
    LOCK_PAGEBUF();

    cyg_nand_page_addr pg = CYG_NAND_BLOCK2PAGEADDR(dev,blk);
    unsigned char *optr = dev->bbt.data;

    if (version > dev->bbt.version) dev->bbt.version = version;
    
    int blks_to_read = 1 << dev->blockcount_bits;
    while (blks_to_read>0) {
        rv = nandi_read_whole_page_raw(dev, pg, bbt_pagebuf, 0, 0, 1);
        // read_page_raw does the ecc for us, and we don't care about the OOB here as we already know it's one of ours.
        if (rv<0) {
            // This is bad.
            int is_primary = blk == dev->bbt.primary;
            NAND_CHATTER(1,dev, "Reading BBT %s (page %u): uncorrectable error\n", is_primary ? "primary" : "mirror", pg);
            (void) is_primary;
            EG(rv);
        }
        if (rv>0) {
            NAND_CHATTER(2,dev,"Reading BBT page %u: ECC repaired, code %d\n", pg, rv);
            rv=0;
        }

        unsigned char *iptr = bbt_pagebuf;
        int data = blks_to_read / 4; /* one byte per 4 blocks on-chip */
        if (data > (1<<dev->page_bits))
            data = (1<<dev->page_bits);

        while (data) {
            unsigned char inp = *iptr;
            unsigned char cur = *optr;
            unsigned char out = 0;
            int j;

            // Each byte tells us about four blocks; so does each output byte
            // NB: Currently hard-coded to 2 bits both in RAM and on chip!
            for (j=0; j<8; j+=2) {
                unsigned char newchip = (inp >> j) & 3;
                //unsigned char new_ram = chip_2_ram(newchip);
                unsigned char curstat = (cur >> j) & 3;

                unsigned char new_ram = worst_of_the_two(curstat, newchip);

                // MTD does it in this order too.
                out |= new_ram << j;
            }
            *optr = out;
            ++iptr; ++optr; --data; blks_to_read -= 4;
        } // inner while(data)
        ++pg;
    } // outer while(blks_to_read>0)
err_exit:
    if (i_locked) UNLOCK_PAGEBUF();
    return rv;
}

/* ============================================================= */

static int nandi_bbt_packing_test(cyg_nand_device *dev)
{
    // Startup sanity check: Can we pack the BBT marker and version into the app spare area, do they appear in the right place, and can we extract them?
    CYG_BYTE appspare[CYG_NAND_APPSPARE_PER_PAGE(dev)],
             packed[CYG_NAND_SPARE_PER_PAGE(dev)],
             testdata[NAND_PATTERN_SIZE > NAND_VERSION_SIZE ? NAND_PATTERN_SIZE : NAND_VERSION_SIZE],
             ecc[CYG_NAND_ECCPERPAGE(dev)];
    int i, rv, fail = 0;

#define MAGIC 42
    memset(ecc, 0xff, sizeof ecc);
    memset(testdata, 0, sizeof testdata);
    for (i=0; i<NAND_PATTERN_SIZE; i++)
        testdata[i] = i+MAGIC;

    i = nand_oob_packed_write(dev, NAND_PATTERN_OFFSET, NAND_PATTERN_SIZE, appspare, testdata);
    if (i == 0)
        i = nand_oob_packed_write(dev, NAND_VERSION_OFFSET, NAND_VERSION_SIZE, appspare, testdata);

    if (i != 0) {
        NAND_ERROR(dev,"BUG: BBT ident/version offset will not fit into this device's spare area\n");
        /* Read the warning in nand_oob.c by nand_mtd_oob_8.
         * To make that work, you have to teach this layer how to find
         * the BBT on such a device. */
        EG(-ENOSYS);
    }

    nand_oob_pack(dev, appspare, sizeof appspare, ecc, packed);
    for (i=0; i<NAND_PATTERN_SIZE; i++)
        if (packed[NAND_PATTERN_OFFSET + i] != i + MAGIC)
            ++fail;
    for (i=0; i<NAND_VERSION_SIZE; i++)
        if (packed[NAND_PATTERN_OFFSET + i] != i + MAGIC)
            ++fail;

    if (fail) {
        NAND_ERROR(dev, "BUG: NAND BBT tag pack did not work");
        EG(-ENOSYS);
    }

    memset(appspare, 0xff, sizeof appspare);
    nand_oob_unpack(dev, appspare, sizeof appspare, ecc, packed);

    memset(testdata, 0, sizeof testdata);
    i = nand_oob_packed_read(dev, NAND_PATTERN_OFFSET, NAND_PATTERN_SIZE, appspare, testdata);
    for (i=0; i<NAND_PATTERN_SIZE; i++)
        if (testdata[i] != i + MAGIC)
            ++fail;

    memset(testdata, 0, sizeof testdata);
    i = nand_oob_packed_read(dev, NAND_VERSION_OFFSET, NAND_VERSION_SIZE, appspare, testdata);
    for (i=0; i<NAND_VERSION_SIZE; i++)
        if (testdata[i] != i + MAGIC)
            ++fail;

    for (i=0; i < sizeof ecc; i++)
        if (ecc[i] != 0xff)
            ++fail;

    if (fail) {
        NAND_ERROR(dev, "BUG: NAND BBT tag unpack did not work");
        EG(-ENOSYS);
    }

    rv = 0;
err_exit:
    return rv;
}

/* ============================================================= */

#if (NAND_VERSION_SIZE != 1)
#error CT_ASSERT: BBT version handling needs recoded
#endif

/* Looks for the BBT copies, or for plausible places to put them.
 * If @find_only@: outputs are set to 0xFFFFFFFF where the respective
 *   ident patterns are not found.
 * Else: outputs are set to plausible-looking places to write BBTs;
 *   where the ident patterns are found, those block IDs are used.
 *   Only if there is nowhere plausible to go (blocks marked bad) then
 *   the relevant output(s) are set to 0xFFFFFFFF.
 * If live BBTs were found, their version identifiers will be stored
 * in *pri_ver and *mir_ver. If not, they will be untouched.
 */
static int nandi_find_bbt_locations(cyg_nand_device *dev,
        cyg_nand_block_addr *pri_o, CYG_BYTE*pri_ver, 
        cyg_nand_block_addr *mir_o, CYG_BYTE*mir_ver, int find_only)
{
    cyg_nand_block_addr start = (1<<dev->blockcount_bits) - 1;
    CYG_BYTE oobbuf[CYG_NAND_APPSPARE_PER_PAGE(dev)];
    CYG_BYTE patternbuf[NAND_PATTERN_SIZE];
    CYG_BYTE versionbuf[NAND_VERSION_SIZE];
    int i, rv=0;
    cyg_nand_block_addr pri, mir, maybepri, maybemir;
    pri = mir = maybepri = maybemir = 0xFFFFFFFF;

    for (i=0; i<4; i++) {
        cyg_nand_block_addr blk = start-i;
        cyg_nand_page_addr pg = CYG_NAND_BLOCK2PAGEADDR(dev,blk);

#define BAD(b) ((cyg_nand_bbti_query(dev,b) != CYG_NAND_BBT_OK) && (cyg_nand_bbti_query(dev,b) != CYG_NAND_BBT_RESERVED))
        if (BAD(blk)) continue;

        // look for the pattern
        memset(oobbuf, 0xff, sizeof oobbuf);
        rv = nandi_read_whole_page_raw(dev, pg, 0, oobbuf, sizeof oobbuf, 1);
        if (rv<0) {
            NAND_CHATTER(1,dev, "find_bbt_locations: Error %d reading OOB of page %u (block %u)\n", -rv, pg, blk);
            EG(-EIO);
        }

        rv = nand_oob_packed_read(dev, NAND_PATTERN_OFFSET, NAND_PATTERN_SIZE, oobbuf, patternbuf);
        if (rv != 0) EG(-EIO); // should never fail given the sanity check above
        rv = nand_oob_packed_read(dev, NAND_VERSION_OFFSET, NAND_VERSION_SIZE, oobbuf, versionbuf);
        if (rv != 0) EG(-EIO); // should never fail given the sanity check above

        if (0==memcmp(patternbuf, nand_pattern_primary, NAND_PATTERN_SIZE)) {
            CYG_BYTE found_ver = *versionbuf;
            if (found_ver > *pri_ver) {
                *pri_ver = found_ver;
                pri = blk;
                continue;
            }
        }
        if (0==memcmp(patternbuf, nand_pattern_mirror,  NAND_PATTERN_SIZE)) {
            CYG_BYTE found_ver = *versionbuf;
            if (found_ver > *mir_ver) {
                *mir_ver = found_ver;
                mir = blk;
                continue;
            }
        }
        if (find_only) continue;

        // It's not an existing table, and it's not bad, so it might be
        // a plausible place to put one.

        if (maybepri == 0xFFFFFFFF) {
            maybepri = blk;
            continue;
        }

        if (maybemir == 0xFFFFFFFF) {
            maybemir = blk;
            continue;
        }
#undef BAD
    }
    *pri_o = (pri == 0xFFFFFFFF) ? maybepri : pri;
    *mir_o = (mir == 0xFFFFFFFF) ? maybemir : mir;

err_exit:
    return rv;
}


int cyg_nand_bbti_find_tables(cyg_nand_device *dev)
{
    int rv = 0, got = 0;
    CYG_BYTE pri_ver = 0, mir_ver = 0;

    NAND_CHATTER(3,dev, "Looking for bad-block table:\n");
    /* TODO: What happens if a BBT block goes bad? Can we put it beyond use?
     * - erasing it _should_ clear the descriptor pattern.
     * Obviously, we should look for further instances of the BBT patterns
     * with fresher version tag(s). */

    EG(nandi_bbt_packing_test(dev));
    EG(nandi_find_bbt_locations(dev, &dev->bbt.primary, &pri_ver, &dev->bbt.mirror, &mir_ver, 1));

    /* OK, we found one or both, so read them in.
     * We must read both, if available, and OR them together. */

    // TODO: Default pattern hard-coded for now.
#define all_ok_pattern ( CYG_NAND_BBT_OK       | (CYG_NAND_BBT_OK << 2) | \
                        (CYG_NAND_BBT_OK << 4) | (CYG_NAND_BBT_OK << 6)  )
    memset(dev->bbt.data, all_ok_pattern, dev->bbt.datasize);

    if (dev->bbt.primary != 0xFFFFFFFF) {
        ++got;
        NAND_CHATTER(3,dev, "Found primary BBT in block %u\n", dev->bbt.primary);
        rv = bbti_incorporate_one(dev, dev->bbt.primary, pri_ver);
        bbti_mark_raw(dev, dev->bbt.primary, CYG_NAND_BBT_RESERVED);
    }

    int rv2 = 0;
    if (dev->bbt.mirror != 0xFFFFFFFF) {
        ++got;
        NAND_CHATTER(3,dev, "Found mirror BBT in block %u\n", dev->bbt.mirror);
        rv2 = bbti_incorporate_one(dev, dev->bbt.mirror, mir_ver);
        bbti_mark_raw(dev, dev->bbt.mirror,  CYG_NAND_BBT_RESERVED);
    }

    if (!got) {
        NAND_CHATTER(3,dev,"Found no BBTs on chip\n");
        EG(-ENOENT);
    }

    if ( (rv < 0) && (rv2 < 0) ) {
        NAND_ERROR(dev, "No BBT usable - disabling device\n");
        EG(-EIO);
    }

    /* TODO: The choice of BBT location and behaviour (BBT versioning;
     * on-chip format) could in future be affected by CDL options or by
     * device-specific settings.
     * The Linux MTD layer has some examples of this. */
err_exit:
    return rv;
}

/* Creates the initial BBTs by running a scan for factory bad blocks.
 * Returns: 0 if OK, else a -ve error code.
 */
int cyg_nand_bbti_build_tables(cyg_nand_device *dev)
{
    cyg_nand_block_addr blk, max = (1<<dev->blockcount_bits);

    NAND_CHATTER(1, dev, "Scanning for factory-bad blocks:\n");

    for (blk=0; blk < max; blk++) {
        int rv = dev->fns->is_factory_bad(dev, blk);
        if (rv<0) {
            NAND_ERROR(dev,"NAND factory-bad scan failed on block %u (%u: %s)\n", blk, -rv, strerror(-rv));
            return rv;
        }
        if (rv) {
            bbti_mark_raw(dev, blk, CYG_NAND_BBT_FACTORY_BAD);
            NAND_CHATTER(2, dev, "bad: %u\n", blk);
        }
    }
    NAND_CHATTER(2, dev, "... done\n");

#ifdef CYGSEM_IO_NAND_READONLY
    NAND_CHATTER(1,dev, "Read-only config: not writing out BBT\n");
    return 0;
#else
    /* This is a virgin device, so we have free choice. 
     * write_tables() takes care of choosing where.
     */

    dev->bbt.version = 0; /* write_tables() will bump to 1, which is correct */

    int rv = cyg_nand_bbti_write_tables(dev, 1);
    if (rv<0)
        NAND_ERROR(dev,"Error %d writing out initial BBT: %s\n", -rv, strerror(-rv));
    else
        NAND_CHATTER(3,dev, "Chosen BBT locations: primary %u, mirror %u\n", dev->bbt.primary, dev->bbt.mirror);

    if (dev->bbt.primary != 0xFFFFFFFF)
        bbti_mark_raw(dev, dev->bbt.primary, CYG_NAND_BBT_RESERVED);
    if (dev->bbt.mirror != 0xFFFFFFFF)
        bbti_mark_raw(dev, dev->bbt.mirror,  CYG_NAND_BBT_RESERVED);

    return rv;
#endif
}

#ifndef CYGSEM_IO_NAND_READONLY
/* Erases and writes out a single copy of the BBT.
 * If an error occurred, returns appropriately; if -EIO, there was
 * evidently a write error in writing out the BBT and we need to restart
 * the write loop... */
static int bbti_write_one_table(cyg_nand_device *dev,
                                cyg_nand_block_addr blk, CYG_BYTE *pattern)
{
    const unsigned pagesize = 1 << dev->page_bits;
    size_t bbt_disk_size = (1<<dev->blockcount_bits)/4;
    
    int st = cyg_nand_bbti_query(dev, blk);
    if (st == CYG_NAND_BBT_WORNBAD) {
        NAND_ERROR(dev, "BBT block %u is worn-bad, not updating\n", blk);
        return 0;
    }

    int rv = dev->fns->erase_block(dev, blk);
    if (rv<0) {
        /* We can't arbitrarily pick another block for the BBT,
         * because we might trample on application data.
         * All the Linux MTD layer does is warn and error out,
         * so that's good enough for us. */
        NAND_ERROR(dev,"Erase BBT block %u failed; cannot update table!\n", blk);
        return rv;
    }

    cyg_nand_page_addr pg = CYG_NAND_BLOCK2PAGEADDR(dev,blk);

    CYG_BYTE *iptr = dev->bbt.data;
    int to_write = bbt_disk_size; // on-chip size, number of bytes
    int i_locked = 0;

    LOCK_PAGEBUF();

    while (to_write) {
        CYG_BYTE *optr = bbt_pagebuf;
        int this_write = to_write;
        if (this_write >= pagesize)
            this_write = pagesize;
        else
            memset(bbt_pagebuf, 0xff, sizeof(bbt_pagebuf));

        /* Hard-coded 2-bit conversion for now. */
        while (this_write) {
            CYG_BYTE d = *iptr,
                     out = 0;
            int j;
            for (j=0; j<8; j+=2) {
                CYG_BYTE inp = (d >> j) & 3;
                out |= ram_2_chip(inp) << j;
            }
            *optr = out;
            --to_write; --this_write; ++iptr; ++optr;
        }

        /* Prep ECC & OOB, then send */
        CYG_BYTE appspare[CYG_NAND_APPSPARE_PER_PAGE(dev)];
        memset(appspare, 0xff, sizeof appspare);

        rv = nand_oob_packed_write(dev, NAND_PATTERN_OFFSET, NAND_PATTERN_SIZE, appspare, pattern);
        if (rv != 0) EG(-EIO); // Should never fail, we've passed the startup sanity check.
        rv = nand_oob_packed_write(dev, NAND_VERSION_OFFSET, NAND_VERSION_SIZE, appspare, &dev->bbt.version);
        if (rv != 0) EG(-EIO); // Should never fail, we've passed the startup sanity check.

        rv = nandi_write_page_raw(dev, pg, bbt_pagebuf, appspare, sizeof appspare);
        if (rv==-EIO) {
            /* Ouch. Our BBT block has failed. We cannot write another, 
             * as we might trample app data. We'll mark bad, and hope
             * that the other copy is still usable. */
            bbti_mark_raw(dev, blk, CYG_NAND_BBT_WORNBAD);
            NAND_ERROR(dev, "Write error on BBT block %u, marking bad\n", blk);
            goto err_exit;
        }
        if (rv<0) {
            /* As with an erase fail, we can't do very much here. */
            NAND_ERROR(dev,"Unexpected error writing to BBT block %u; cannot update table!\n", blk);
            goto err_exit;
        }
        ++pg;
    }

err_exit:
    UNLOCK_PAGEBUF();
    return rv;
}


/* Increments the BBT version, then writes out _both_ tables. 
 * Caller must have the devlock. */
static int cyg_nand_bbti_write_tables(cyg_nand_device *dev, int is_initial)
{
    int rv, retries=-1;

top:
    ++retries;

    if (dev->bbt.version == 255)
        NAND_ERROR(dev,"Warning! NAND BBT version would overflow, doing the best we can\n");
    else
        dev->bbt.version ++;

    CYG_BYTE pri_ver, mir_ver;
    rv = nandi_find_bbt_locations(dev, &dev->bbt.primary, &pri_ver,
            &dev->bbt.mirror, &mir_ver, is_initial ? 0 : 1);

#define RETRY_LIMIT 3

    if (dev->bbt.primary == 0xFFFFFFFF) {
        NAND_ERROR(dev,"Cannot find a location to write primary BBT!\n");
        return -EIO;
    } else {
        rv = bbti_write_one_table(dev, dev->bbt.primary, nand_pattern_primary);
        if (rv == -EIO && retries < RETRY_LIMIT) goto top;
    }
    if (dev->bbt.mirror == 0xFFFFFFFF) {
        NAND_ERROR(dev,"Cannot find a location to write mirror BBT!\n");
        return -EIO;
    } else {
        rv = bbti_write_one_table(dev, dev->bbt.mirror, nand_pattern_mirror);
        if (rv == -EIO && retries < RETRY_LIMIT) goto top;
    }

    if (retries >= RETRY_LIMIT) 
        NAND_ERROR(dev, "Warning! Retry limit on BBT writes reached, this shouldn't happen...\n");

    return 0;
}
#endif

#endif
