//=============================================================================
//
//      mounttime.c
//
//      Mount timings exerciser, use on a filesystem filled by `makefiles'
//      THIS DOES NOT CURRENTLY WORK ON non-PRO eCos!
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
// Date:        2009-06-29
// Description: File system mount and unmount timing benchmark.
//              Intended to be used on a complicated filesystem, e.g.
//              one filled up by `makefiles'.
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

#include <cyg/fileio/fileio.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

#include <cyg/kernel/kapi.h>

#ifdef CYGPKG_DEVS_NAND_SYNTH
#define DEVICE "synth/0"
#else
#define DEVICE "onboard/0"
#endif

#define MOUNTPOINT "/n"
#define FSTYPE "yaffs"
#define _NOCACHE "skip-checkpoint-read"

#define FS_NOCACHE FSTYPE":"_NOCACHE

//#define MUST(what) do { CYG_TEST_CHECK((what), #what); } while(0)

#define MUST(what) do { \
        if (0 != what) { \
                perror(#what);          \
                CYG_TEST_FAIL(#what);   \
                cyg_test_exit();        \
        }                           \
} while(0)

#define min(a,b) ( (a) > (b) ? (b) : (a) )

// --------------------------------------------------------------

void RM_RF_work(const char *f, int recurse)
{
    MUST(chdir(f));
    DIR *d = opendir(".");
    if (!d) {
        diag_printf("opendir %s: %s\n", f, strerror(errno));
        return;
    }
    struct dirent de_stack;
    struct dirent *de_res;
    while (0==readdir_r(d, &de_stack, &de_res) && de_res) {
        struct stat st;
        if (0==strcmp(de_res->d_name,".")) continue;
        if (0==strcmp(de_res->d_name,"..")) continue;
        MUST(stat(de_res->d_name,&st));
        if (S_ISDIR(st.st_mode)) {
            RM_RF_work(de_res->d_name, recurse+1);
            if (rmdir(de_res->d_name))
                perror("rmdir");
        } else {
            MUST(unlink(de_res->d_name));
        }
    }
    closedir(d);
    MUST(chdir(".."));
}

void RM_RF(const char *p)
{
    diag_printf("rm -rf %s\n",p);
    RM_RF_work(p,0);
    MUST(rmdir(p));
}

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

static timing tm;

#define START() do {            \
    wait_for_tick();            \
    get_timestamp(&tm.start);   \
} while(0)

static void report(const char *what, timing tm)
{
    if (tm.us_interval > 1100000) 
        diag_printf("%30s: %2d.%03d s\n", what, (int)(tm.us_interval/1000000), (int)(tm.us_interval/1000)%1000);
    else
        diag_printf("%30s: %2d.%03d ms\n", what, (int)(tm.us_interval/1000), (int)(tm.us_interval%1000));

}

#define STOP(what) do {         \
    get_timestamp(&tm.end);     \
    calculate_interval(&tm);    \
    report(what, tm);           \
} while(0)

// --------------------------------------------------------------

void dumpmallinfo(void)
{
    struct mallinfo mi = mallinfo();
    int total_ord = mi.uordblks + mi.fordblks;
    double pct = (double)mi.uordblks / (double)total_ord;
    diag_printf("! Arena: size 0x%x, small blks %u/%u, ord blks used %u/%u (%u%%)\n", 
            mi.arena,
            mi.usmblks, mi.fsmblks,
            mi.uordblks, total_ord, (int)(100.0*pct));
}

// --------------------------------------------------------------

#define WORKDIR MOUNTPOINT "/" "test"

void mounttime_main(void)
{
#define NTIMES 3
    int i;
    for (i=0; i<NTIMES; i++) {
        START();
        MUST(mount(DEVICE, MOUNTPOINT, FSTYPE));
        STOP("remount");
        //if (i==0) dumpmallinfo();
        START();
        MUST(umount(MOUNTPOINT));
        STOP("unmount");

        START();
        MUST(mount(DEVICE, MOUNTPOINT, FS_NOCACHE));
        // plain eCos does not support mount options, this test won't work there.
        STOP("remount without checkpoint");
        //if (i==0) dumpmallinfo();
        START();
        MUST(umount(MOUNTPOINT));
        STOP("unmount");
        //if (i==0) dumpmallinfo();
    }
    //dumpmallinfo();

    // then consider repeats..

    CYG_TEST_EXIT("Run complete");
}

int main(void)
{
    CYG_TEST_INIT();
    init_timing();

    mounttime_main();
    return 0;
}

#endif

