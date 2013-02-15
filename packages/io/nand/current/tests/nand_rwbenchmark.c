//=============================================================================
//
//      mounttime.c
//
//      Mount timings exerciser, use on a filesystem filled by `makefiles'
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2009 Free Software Foundation, Inc.
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
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   wry
// Date:        2009-07-02
// Description: Read, write and erase timing benchmark.
//              Some timing code borrowed from mmfs tests.
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/infra/testcase.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/cyg_ass.h>
#include <pkgconf/system.h>
#include <pkgconf/hal.h>
#include <cyg/hal/hal_intr.h>

#ifndef HAL_CLOCK_READ
# error "HAL_CLOCK_READ not defined!"
#endif

#if !defined(CYGPKG_LIBC_TIME) || !defined(CYGPKG_ERROR) || !defined(CYGPKG_LIBC_STDIO) || !defined(CYGPKG_KERNEL)

void cyg_user_start(void)
{
    CYG_TEST_NA("Needs CYGPKG_KERNEL, CYGPKG_LIBC_TIME, CYGPKG_ERROR and CYGPKG_LIBC_STDIO");
}

#else

#include <cyg/nand/nand.h>
#include <cyg/nand/nand_device.h>
#include <cyg/nand/util.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

#include <cyg/kernel/kapi.h>

#include "ar4prng.inl"
ar4ctx rnd;
/* we'll use our text segment as seed to the prng; not crypto-secure but
 * pretty good for running tests */
extern unsigned char _stext[], _etext[];


#ifdef CYGPKG_DEVS_NAND_SYNTH
#define DEVICE "synth"
#else
#define DEVICE "onboard"
#endif

#define PARTITION 0

#define MUST(what) do { \
        if (0 != what) { \
                perror(#what);          \
                CYG_TEST_FAIL(#what);   \
                cyg_test_exit();        \
        }                           \
} while(0)

#define min(a,b) ( (a) > (b) ? (b) : (a) )

// --------------------------------------------------------------
// Timing code cribbed from pvr5.c

typedef struct
{
    cyg_int64           ticks;
    cyg_uint32           halticks;
} timestamp;

typedef struct
{
    timestamp           start;
    timestamp           end;
    cyg_int64           interval;       // In HAL ticks
    cyg_int64           us_interval;    // Interval in microseconds
} timing;

static cyg_int64        ticks_overhead = 0;
static cyg_int32        us_per_haltick = 0;
static cyg_int32        halticks_per_us = 0;

static cyg_int64        rtc_resolution[] = CYGNUM_KERNEL_COUNTERS_RTC_RESOLUTION;
static cyg_int64        rtc_period = CYGNUM_KERNEL_COUNTERS_RTC_PERIOD;

static void wait_for_tick( void )
{
    cyg_tick_count_t now = cyg_current_time();

    while( cyg_current_time() == now )
        continue;
}

static void get_timestamp( timestamp *ts )
{
    ts->ticks = cyg_current_time();
    HAL_CLOCK_READ( &ts->halticks );
}

static cyg_int64 ticks_to_us( cyg_int64 ticks )
{
    cyg_int64 us;

    if( us_per_haltick != 0 )
        us = ticks * us_per_haltick;
    else
        us = ticks / halticks_per_us;

    return us;
}

static void calculate_interval( timing *t )
{
    t->interval = t->end.halticks - t->start.halticks;

    t->interval += (t->end.ticks - t->start.ticks) * rtc_period;

    t->interval -= ticks_overhead;

    t->us_interval = ticks_to_us( t->interval );
}

static void init_timing( void )
{
    timing t;

    cyg_thread_delay(2); // ensure clock running - asserts if not

    us_per_haltick = 1000000/(rtc_period * rtc_resolution[1]);
    halticks_per_us = (rtc_period * rtc_resolution[1])/1000000;

    wait_for_tick();

    get_timestamp( &t.start );
    get_timestamp( &t.end );

    calculate_interval( &t );

    ticks_overhead = t.interval;

    diag_printf("Timing overhead %lld ticks (%lluus), this will be factored out of all other measurements\n", ticks_overhead, t.us_interval );
}

#define disable_clock_latency_measurement()
#define enable_clock_latency_measurement()

// --------------------------------------------------------------

void
show_times_hdr(void)
{
    disable_clock_latency_measurement();
    diag_printf("\n");
#ifdef _TM_BASIC_HAL_CLOCK_READ_UNDEFINED
    diag_printf("HAL_CLOCK_READ() is not supported on this platform.\n");
    diag_printf("Timing results are meaningless.\n");
#endif
    diag_printf("All times are in microseconds (us) unless stated\n");
    diag_printf("\n");
    diag_printf("                                     Confidence\n");
    diag_printf("      Ave      Min      Max      Var  Ave  Min  Function\n");
    diag_printf("   ======   ======   ======   ====== ========== ========\n");
    enable_clock_latency_measurement();
}

// Display a time result
void
show_ns(cyg_uint64 ns)
{
    if (ns > 999995000) {
        ns += 5000000;  // round to nearest 0.01s
        diag_printf("%5u.%02us", (int)(ns/1000000000), (int)((ns%1000000000)/10000000));

    } else if (ns >= 99999995) {
        ns += 5000;  // round to 0.01ms
        diag_printf("%4u.%02ums", (int)(ns/1000000), (int)((ns%1000000)/10000));
    } else {
        ns += 5;  // round to .01us
        diag_printf("%6u.%02u", (int)(ns/1000), (int)((ns%1000)/10));
    }
}

void
show_times_detail(timing ft[], int nsamples, char *title, bool ignore_first)
{
    int i;
    int start_sample, total_samples;   
    cyg_uint64 delta, total, ave, min, max, ave_dev;
    /* we measure in ticks, convert to us, but store as ns for good precision */
    cyg_int32 con_ave, con_min;

    if (ignore_first) {
        start_sample = 1;
        total_samples = nsamples-1;
    } else {
        start_sample = 0;
        total_samples = nsamples;
    }

    total = 0;
    min = 0xFFFFffffFFFFffffULL;
    max = 0;
    for (i = start_sample;  i < nsamples;  i++) {
        calculate_interval(&ft[i]);
        delta = ft[i].us_interval * 1000;
        total += delta;
        if (delta < min) min = delta;
        if (delta > max) max = delta;
    }
    ave = total / total_samples;

    ave_dev = 0;
    for (i = start_sample;  i < nsamples;  i++) {
        delta = ft[i].us_interval * 1000;
        if (delta > ave)
            delta = delta - ave;
        else
            delta = ave - delta;
        ave_dev += delta;
    }
    ave_dev /= total_samples;

    con_ave = 0;
    con_min = 0;
    for (i = start_sample;  i < nsamples;  i++) {
        delta = ft[i].us_interval * 1000;
        if ((delta <= (ave+ave_dev)) && (delta >= (ave-ave_dev))) con_ave++;
        if ((delta <= (min+ave_dev)) && (delta >= (min-ave_dev))) con_min++;
    }
    con_ave = (con_ave * 100) / total_samples;
    con_min = (con_min * 100) / total_samples;

    disable_clock_latency_measurement();
    show_ns(ave);
    show_ns(min);
    show_ns(max);
    show_ns(ave_dev);
    diag_printf("  %3d%% %3d%%", con_ave, con_min);
    diag_printf(" %s\n", title);

    CYG_ASSERT( ave <= max, "ave < max" );
    
    enable_clock_latency_measurement();
}

void
show_times(timing ft[], int nsamples, char *title)
{
    show_times_detail(ft, nsamples, title, false);
#ifdef STATS_WITHOUT_FIRST_SAMPLE
    show_times_detail(ft, nsamples, "", true);
#endif
}

// --------------------------------------------------------------

#define WORKDIR MOUNTPOINT "/" "test"

#define NREADS 100
#define NBULKREADS 10
#define NBULKWRITES 10
#define NWRITES 30
#define NERASES 30

void show_test_parameters(cyg_nand_device *dev)
{
    disable_clock_latency_measurement();
    diag_printf("\nTesting parameters:\n");
    diag_printf("   NAND reads:            %5u\n", NREADS);
    if (NWRITES)
        diag_printf("   NAND writes:           %5u\n", NWRITES);
    if (NERASES)
        diag_printf("   NAND erases:           %5u\n", NERASES);
    if (NBULKREADS)
        diag_printf("   NAND bulk reads:       %5u\n", NBULKREADS);
    if (NBULKWRITES)
        diag_printf("   NAND bulk writes:      %5u\n", NBULKWRITES);
    diag_printf("   Device page size:      %5u bytes\n"
                "   Device block size:     %5u pages\n",
            CYG_NAND_BYTES_PER_PAGE(dev),
            CYG_NAND_PAGES_PER_BLOCK(dev) );

    enable_clock_latency_measurement();
}

cyg_nand_block_addr find_spare_block(cyg_nand_partition *part)
{
    const int oobz = CYG_NAND_APPSPARE_PER_PAGE(part->dev);
    unsigned char oob[oobz];
    int i,rv;
    cyg_nand_block_addr b;

    for (b=CYG_NAND_PARTITION_NBLOCKS(part)-1; b>=0; b--) {
        cyg_nand_page_addr pg = CYG_NAND_BLOCK2PAGEADDR(part->dev, b);
        rv = cyg_nandp_read_page(part, pg, 0, oob, oobz);
        if (rv != 0) continue; // bad block?

        for (i=0; i<oobz; i++)
            if (oob[i] != 0xff)
                goto next;

        // oob all FF: take it!
        return b;
next:
        ;
    }

    CYG_TEST_FAIL_EXIT("can't find an untagged block");
}

CYG_BYTE databuf[CYGNUM_NAND_PAGEBUFFER], testbuf[CYGNUM_NAND_PAGEBUFFER];
timing ft_read[NREADS];
timing ft_write[NWRITES];
timing ft_erase[NERASES];
timing ft_bulkread[NBULKREADS];
timing ft_bulkwrite[NBULKWRITES];

int fails = 0;

void test_reads(cyg_nand_partition *part, cyg_nand_block_addr b)
{
    cyg_nand_page_addr pgstart = CYG_NAND_BLOCK2PAGEADDR(part->dev, b),
                       pgend = CYG_NAND_BLOCK2PAGEADDR(part->dev, b+1)-1,
                       pg = pgstart;
    int i, rv;
    const int oobz = CYG_NAND_APPSPARE_PER_PAGE(part->dev);
    unsigned char oob[oobz];
#define ft ft_read

    /* First, set up the block the way we want it ... */
    cyg_nandp_erase_block(part, b);
    memcpy(oob, databuf, oobz);
    for (i=pgstart; i <= pgend; i++) {
        rv = cyg_nandp_write_page(part, i, databuf, oob, oobz);
        switch (rv) {
            case 0: break;
            case -EIO:
                    cyg_nandp_bbt_markbad(part, b);
                    CYG_TEST_FAIL_FINISH("Write failed; block now marked as bad. This run can't continue but should be OK to repeat.");

            default:
                    diag_printf("Unexpected write failure: %d\n", rv);
                    CYG_TEST_FAIL_FINISH("Pre-write failed");
        }
    }

#define CLEARDATA() memset(testbuf, 0, sizeof testbuf)
#define CHECKDATA() do { if (0 != memcmp(testbuf, databuf, sizeof testbuf)) { \
    CYG_TEST_FAIL("readback check failed");                                   \
    ++fails;                                                                  \
    break;                                                                    \
} } while(0)

#define CLEAROOB() memset(oob, 0, oobz)
#define CHECKOOB() do { if (0 != memcmp(oob, databuf, oobz)) {  \
    CYG_TEST_FAIL("readback OOB check failed");                 \
    ++fails;                                                    \
    break;                                                      \
} } while(0)


    for (i=0; i < NREADS; i++) {
        CLEARDATA();
        wait_for_tick();
        get_timestamp(&ft[i].start);
        cyg_nandp_read_page(part, pg, testbuf, 0, 0);
        get_timestamp(&ft[i].end);
        CHECKDATA();
        ++pg;
        if (pg > pgend) pg = pgstart;
    }
    show_times(ft, NREADS, "NAND page reads (page data only)");

    for (i=0; i < NREADS; i++) {
        CLEAROOB();
        wait_for_tick();
        get_timestamp(&ft[i].start);
        cyg_nandp_read_page(part, pg, 0, oob, oobz);
        get_timestamp(&ft[i].end);
        CHECKOOB();
        ++pg;
        if (pg > pgend) pg = pgstart;
    }
    show_times(ft, NREADS, "NAND page reads (OOB only)");

    for (i=0; i < NREADS; i++) {
        CLEARDATA();
        CLEAROOB();
        wait_for_tick();
        get_timestamp(&ft[i].start);
        cyg_nandp_read_page(part, pg, testbuf, oob, oobz);
        get_timestamp(&ft[i].end);
        CHECKDATA();
        CHECKOOB();
        ++pg;
        if (pg > pgend) pg = pgstart;
    }
    show_times(ft, NREADS, "NAND page reads (page + OOB)");

#undef ft
#define ft ft_bulkread
    for (i=0; i < NBULKREADS; i++) {
        CLEARDATA();
        wait_for_tick();
        get_timestamp(&ft[i].start);
        for (pg = pgstart; pg <= pgend; pg++) {
            cyg_nandp_read_page(part, pg, testbuf, 0, 0);
        }
        get_timestamp(&ft[i].end);
        CHECKDATA();
    }
    // report the results later.
#undef ft
    cyg_nandp_erase_block(part, b);
}


void test_writes(cyg_nand_partition *part, cyg_nand_block_addr b)
{
    cyg_nand_page_addr pgstart = CYG_NAND_BLOCK2PAGEADDR(part->dev, b),
                       pgend = CYG_NAND_BLOCK2PAGEADDR(part->dev, b+1)-1,
                       pg = pgstart;
    int i;
    const int oobz = CYG_NAND_APPSPARE_PER_PAGE(part->dev);
    unsigned char oob[oobz];
#define ft ft_write

    cyg_nandp_erase_block(part, b);
    for (i=0; i < NWRITES; i++) {
        wait_for_tick();
        get_timestamp(&ft[i].start);
        cyg_nandp_write_page(part, pg, databuf, databuf, oobz);
        get_timestamp(&ft[i].end);

        // Read it back to confirm
        CLEARDATA();
        CLEAROOB();
        cyg_nandp_read_page(part, pg, testbuf, oob, oobz);
        CHECKDATA();
        CHECKOOB();

        ++pg;
        if (pg > pgend) {
            cyg_nandp_erase_block(part, b);
            pg = pgstart;
        }
    }
    cyg_nandp_erase_block(part, b);
    show_times(ft, NWRITES, "NAND full-page writes");

#undef ft
#define ft ft_bulkwrite
    // N.B. Bulk writing times will likely improve later if we implement cache-programming mode as found on many a large-page device.
    for (i=0; i < NBULKWRITES; i++) {
        wait_for_tick();
        get_timestamp(&ft[i].start);
        for (pg = pgstart; pg <= pgend; pg++) {
            cyg_nandp_write_page(part, pg, testbuf, 0, 0);
        }
        get_timestamp(&ft[i].end);
        cyg_nandp_erase_block(part, b);
    }
    // report later.
#undef ft
}


void test_erases(cyg_nand_partition *part, cyg_nand_block_addr b)
{
    int i;
#define ft ft_erase

    for (i=0; i < NERASES; i++) {
        wait_for_tick();
        get_timestamp(&ft[i].start);
        cyg_nandp_erase_block(part, b);
        get_timestamp(&ft[i].end);

        if (i==0) {
            cyg_nand_page_addr pg = CYG_NAND_BLOCK2PAGEADDR(part->dev, b);
            const int oobz = CYG_NAND_APPSPARE_PER_PAGE(part->dev);
            unsigned char oob[oobz];
            int j;
            // TODO: It only makes sense to check the one, unless we want 
            // to try writing out more dummy data each time.
            CLEARDATA();
            CLEAROOB();
            cyg_nandp_read_page(part, pg, testbuf, oob, oobz);
            for (j=0; j < sizeof testbuf; j++) {
                if (testbuf[j] != 0xff) {
                    CYG_TEST_FAIL("readback check failed");
                    ++fails;
                    break;
                }
            }
            for (j=0; j < oobz; j++) {
                if (oob[j] != 0xff) {
                    CYG_TEST_FAIL("readback OOB check failed");
                    ++fails;
                    break;
                }
            }
        }
    }
    show_times(ft, NERASES, "NAND block erases");
#undef ft
}



void rwbenchmark_main(void)
{
    cyg_nand_device *dev;
    cyg_nand_partition *part;
    cyg_nand_block_addr block;

    cyg_nand_lookup(DEVICE, &dev);
    if (!dev)
        CYG_TEST_FAIL_EXIT("can't get device "DEVICE);
    part = cyg_nand_get_partition(dev, PARTITION); 
    if (!part)
        CYG_TEST_FAIL_EXIT("can't get partition");

    block = find_spare_block(part);
    diag_printf("Using block %d\n",block);

    show_test_parameters(dev);
    show_times_hdr();

    // TODO: For speed, refactor test_reads and test_writes into each other.
    test_reads(part, block);

    // Freshen our test data for the second phase
    ar4prng_many(&rnd, databuf, sizeof databuf);
    test_writes(part,block);

    test_erases(part,block);

    show_times(ft_bulkread,  NBULKREADS,  "Bulk reads (block data)");
    show_times(ft_bulkwrite, NBULKWRITES, "Bulk writes (block data)");
}

int main(void)
{
    CYG_TEST_INIT();
    init_timing();

    ar4prng_init(&rnd,_stext, _etext-_stext);
    ar4prng_many(&rnd, databuf, sizeof databuf);


#if defined(CYGVAR_KERNEL_COUNTERS_CLOCK_LATENCY) || defined(CYGVAR_KERNEL_COUNTERS_CLOCK_DSR_LATENCY)
    CYG_TEST_INFO("WARNING: Clock or DSR latency instrumentation can mess with the results, recommend you turn it off.");
#endif


    rwbenchmark_main();

    if (fails) {
        diag_printf("There were %d failures.\n",fails);
        CYG_TEST_FAIL_FINISH("something went wrong, THESE RESULTS ARE INVALID");
    }

    CYG_TEST_PASS_FINISH("Benchmarking complete");
}

#endif

