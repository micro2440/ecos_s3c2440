//==========================================================================
//
//      yaffs5.c
//
//      Test FAT filesystem
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2004, 2006, 2007 Free Software Foundation, Inc.
// Copyright (C) 2004, 2006, 2007 eCosCentric Limited                       
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
// Author(s):           nickg
// Contributors:        nickg
// Date:                2007-03-28
// Purpose:             Test FAT system
// Description:         This test gets some performance figures from the
//                      FAT filesystem.
//                      
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/system.h>

#ifdef CYGPKG_KERNEL

#include <pkgconf/hal.h>
#include <pkgconf/kernel.h>
#include <pkgconf/io_fileio.h>

#include CYGDAT_DEVS_DISK_CFG           // testing config defines

#include <cyg/kernel/ktypes.h>         // base kernel types
#include <cyg/infra/cyg_trac.h>        // tracing macros
#include <cyg/infra/cyg_ass.h>         // assertion macros

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

#include <cyg/fileio/fileio.h>

#include <cyg/infra/testcase.h>
#include <cyg/infra/diag.h>            // HAL polled output

//==========================================================================
// Config options

#define TEST_DEVICE     CYGDAT_DEVS_DISK_TEST_DEVICE

#define TEST_MOUNTPOINT CYGDAT_DEVS_DISK_TEST_MOUNTPOINT

#define TEST_DIRECTORY CYGDAT_DEVS_DISK_TEST_DIRECTORY

#define TEST_FSTYPE     "yaffs"

#ifdef CYGDAT_DEVS_DISK_TEST_MOUNTPOINT2

#define TEST_DEVICE2     CYGDAT_DEVS_DISK_TEST_DEVICE2

#define TEST_MOUNTPOINT2 CYGDAT_DEVS_DISK_TEST_MOUNTPOINT2

#define TEST_DIRECTORY2 CYGDAT_DEVS_DISK_TEST_DIRECTORY2

#define TEST_FSTYPE2    "yaffs"

#endif

//==========================================================================

#define SHOW_RESULT( _fn, _res )                                                                \
{                                                                                               \
    diag_printf("<FAIL>: " #_fn "() returned %ld %s\n", (long)_res, _res<0?strerror(errno):"");        \
}

//==========================================================================

typedef struct
{
    cyg_int64           ticks;
    cyg_int32           halticks;
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
static cyg_int32        ticks_per_second;

//==========================================================================

static char date_string[] = __DATE__;
static char time_string[] = __TIME__;

cyg_uint32 timehash;

//==========================================================================

static void wait_for_tick( void )
{
    cyg_tick_count_t now = cyg_current_time();

    while( cyg_current_time() == now )
        continue;
}

static void get_timestamp( timestamp *ts )
{
    ts->ticks = cyg_current_time();
    HAL_CLOCK_READ( (cyg_uint32 *)&ts->halticks );
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

    ticks_per_second = (1000000000ULL*rtc_resolution[1])/rtc_resolution[0];
    us_per_haltick = 1000000/(rtc_period * rtc_resolution[1]);
    halticks_per_us = (rtc_period * rtc_resolution[1])/1000000; 
    
    wait_for_tick();

    get_timestamp( &t.start );
    get_timestamp( &t.end );

    calculate_interval( &t );

    ticks_overhead = t.interval;

    diag_printf("Timing overhead %lld ticks\n", ticks_overhead );
    
}

//==========================================================================

#ifndef CYGPKG_LIBC_STRING

char *strcat( char *s1, const char *s2 )
{
    char *s = s1;
    while( *s1 ) s1++;
    while( (*s1++ = *s2++) != 0);
    return s;
}

#endif

__externC int
rename( const char * /* oldpath */, const char * /* newpath */ ) __THROW;

//==========================================================================

static void listdir( char *name, int statp, int numexpected, int *numgot )
{
    int err;
    DIR *dirp;
    int num=0;
    
    diag_printf("<INFO>: reading directory %s\n",name);
    
//    diag_printf("<INFO>: opendir(%s)\n",name);
    dirp = opendir( name );
    if( dirp == NULL ) SHOW_RESULT( opendir, -1 );

    for(;;)
    {
        struct dirent *entry = readdir( dirp );
        
        if( entry == NULL )
            break;
        num++;
        diag_printf("<INFO>: entry %14s",entry->d_name);
        if( statp )
        {
            char fullname[PATH_MAX];
            struct stat sbuf;

            if( name[0] )
            {
                strcpy(fullname, name );
                if( !(name[0] == '/' && name[1] == 0 ) )
                    strcat(fullname, "/" );
            }
            else fullname[0] = 0;
            
            strcat(fullname, entry->d_name );

            err = stat( fullname, &sbuf );
            if( err < 0 )
            {
                if( errno == ENOSYS )
                    diag_printf(" <no status available>");
                else SHOW_RESULT( stat, err );
            }
            else
            {
                diag_printf(" [mode %08x ino %08x nlink %d size %ld]",
                            sbuf.st_mode,sbuf.st_ino,sbuf.st_nlink,sbuf.st_size);
            }
        }

        diag_printf("\n");
    }

//    diag_printf("<INFO>: closedir(%s)\n",name);    
    err = closedir( dirp );
    if( err < 0 ) SHOW_RESULT( stat, err );
    if (numexpected >= 0 && num != numexpected)
        CYG_TEST_FAIL("Wrong number of dir entries\n");
    if ( numgot != NULL )
        *numgot = num;
}

//==========================================================================

static void createfile( char *name, size_t asize, size_t block_size )
{
    size_t size = asize;
    cyg_uint8 *buf;
    cyg_uint32 *b;
    int fd;
    ssize_t wrote;
    int i;
    int err;
    timing opent, writet;
        
    diag_printf("\n--------------------------------------------------------------------\n");
    diag_printf("<INFO>: create file %s size %ld block_size %ld\n",name, (long)size, (long)block_size);

    buf = malloc( block_size );
    if( buf == NULL )
    {
        SHOW_RESULT( malloc, -1 );
        return;
    }
    b = (cyg_uint32 *)buf;
    
    err = access( name, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );

    i = 0;
    while( i < block_size/sizeof(cyg_uint32) )
    {
        b[i] = i;
        i++;
        b[i++] = 0;
        b[i++] = timehash;
        b[i++] = 0x12345678;
    }

    
    get_timestamp( &opent.start );        
    fd = open( name, O_WRONLY|O_CREAT );
    get_timestamp( &opent.end );        
    if( fd < 0 ) SHOW_RESULT( open, fd );

    get_timestamp( &writet.start );    
    while( size > 0 )
    {
        ssize_t len = size;
        if ( len > block_size ) len = block_size;

        b[1] = asize-size;
        b[block_size/sizeof(cyg_uint32)-3] = asize-size;
        wrote = write( fd, buf, len );
        if( wrote != len ) SHOW_RESULT( write, wrote );        

        size -= wrote;
    }
    get_timestamp( &writet.end );        

    free( buf );
    
    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );

    calculate_interval( &opent );
    calculate_interval( &writet );

    diag_printf("CreateFile Results: %s size %ld\n", name, (long)asize);
    diag_printf("         Open        %8d.%03d ms\n", (int)(opent.us_interval/1000), (int)(opent.us_interval%1000) );
    diag_printf("        Write        %8d.%03d ms\n", (int)(writet.us_interval/1000), (int)(writet.us_interval%1000) );
    diag_printf("    Data Rate          %10lld KiB/s\n", (1000000LL*asize)/1024/writet.us_interval);
}

//==========================================================================

static void checkfile( char *name, size_t block_size )
{
    cyg_uint8 *buf;
    cyg_uint32 *b;
    int fd;
    ssize_t done;
    int err;
    off_t pos = 0;
    timing opent, readt;
    
    diag_printf("\n--------------------------------------------------------------------\n");
    diag_printf("<INFO>: check file %s\n",name);
    
    buf = malloc( block_size );
    if( buf == NULL )
    {
        SHOW_RESULT( malloc, -1 );
        return;
    }
    b = (cyg_uint32 *)buf;
    
    err = access( name, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );

    get_timestamp( &opent.start );
    fd = open( name, O_RDONLY );
    get_timestamp( &opent.end );            
    if( fd < 0 ) SHOW_RESULT( open, fd );

    get_timestamp( &readt.start );            
    for(;;)
    {
        done = read( fd, buf, block_size );
        if( done < 0 ) SHOW_RESULT( read, done );

        if( done == 0 ) break;

        // Check that this is the block we wrote
        if( b[0] != 0 )
            diag_printf("b[%d] != %08x\n", 0, 0 );

        if( b[1] != pos )
            diag_printf("b[%d] != %08x\n", 1, (unsigned)pos );

        if( b[2] != timehash )
            diag_printf("b[%d] != %08x\n", 2, timehash );

        if( b[3] != 0x12345678 )
            diag_printf("b[%d] != %08x\n", 3, 0x12345678 );

        if( b[(block_size/sizeof(cyg_uint32))-4] != (block_size/sizeof(cyg_uint32)-4) )
            diag_printf("b[%lu](%08x) != %08x\n", (long)block_size/sizeof(cyg_uint32)-4,
                                                  b[(block_size/sizeof(cyg_uint32))-4],
                                                  (unsigned)(block_size/sizeof(cyg_uint32)-4) );

        if( b[block_size/sizeof(cyg_uint32)-3] != pos )
            diag_printf("b[%lu] != %08x\n", (long)block_size/sizeof(cyg_uint32)-3, (unsigned)pos );

        if( b[block_size/sizeof(cyg_uint32)-2] != timehash )
            diag_printf("b[%lu] != %08x\n", (long)block_size/sizeof(cyg_uint32)-2, timehash );

        if( b[block_size/sizeof(cyg_uint32)-1] != 0x12345678 )
            diag_printf("b[%lu] != %08x\n", (long)block_size/sizeof(cyg_uint32)-1, 0x12345678 );
        
        pos += done;
    }
    get_timestamp( &readt.end );            

    free( buf );
    
    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );

    calculate_interval( &opent );
    calculate_interval( &readt );

    diag_printf("CheckFile Results: %s size %ld\n", name, pos);
    diag_printf("         Open        %8d.%03d ms\n", (int)(opent.us_interval/1000), (int)(opent.us_interval%1000) );
    diag_printf("         Read        %8d.%03d ms\n", (int)(readt.us_interval/1000), (int)(readt.us_interval%1000) );
    diag_printf("    Data Rate          %10lld KiB/s\n", (1000000LL*pos)/1024/readt.us_interval);
}

//==========================================================================

#define TIME_DATA_MAX 20

static void checkfile1( char *name, size_t block_size )
{
    cyg_uint8 *buf;
    cyg_uint32 *b;
    int fd;
    ssize_t done;
    int err;
    off_t pos = 0;
    timing totalt;
    
    struct
    {
        int     valid;
        timing  mountt;
        timing  opent;
        timing  seekt;
        timing  readt;
        timing  closet;
        timing  umountt;
    } time_data[TIME_DATA_MAX];
    int td;

    for( td = 0; td < TIME_DATA_MAX; td++ )
        time_data[td].valid = 0;
    td = 0;
    
    diag_printf("\n--------------------------------------------------------------------\n");
    diag_printf("<INFO>: check file1 %s\n",name);
    
    buf = malloc( block_size );
    if( buf == NULL )
    {
        SHOW_RESULT( malloc, -1 );
        return;
    }
    b = (cyg_uint32 *)buf;

    get_timestamp( &totalt.start );
    for(;;)
    {
        get_timestamp( &time_data[td].mountt.start );
        err = mount( TEST_DEVICE, TEST_MOUNTPOINT, TEST_FSTYPE );
        get_timestamp( &time_data[td].mountt.end );
        if( err < 0 ) SHOW_RESULT( mount, err );
        
        get_timestamp( &time_data[td].opent.start );
        fd = open( name, O_RDONLY );        
        get_timestamp( &time_data[td].opent.end );
        if( fd < 0 ) SHOW_RESULT( open, fd );
    
        get_timestamp( &time_data[td].seekt.start );
        err = lseek( fd, pos, SEEK_SET );
        get_timestamp( &time_data[td].seekt.end );
        if( err < 0 ) SHOW_RESULT( lseek, err );
        
        get_timestamp( &time_data[td].readt.start );        
        done = read( fd, buf, block_size );
        get_timestamp( &time_data[td].readt.end );                
        if( done < 0 ) SHOW_RESULT( read, done );

        get_timestamp( &time_data[td].closet.start );        
        err = close( fd );
        get_timestamp( &time_data[td].closet.end );        
        if( err < 0 ) SHOW_RESULT( close, err );
        
        get_timestamp( &time_data[td].umountt.start );
        err = umount( TEST_MOUNTPOINT );
        get_timestamp( &time_data[td].umountt.end );
        if( err < 0 ) SHOW_RESULT( umount, err );
        
        if( done == 0 ) break;

        // Check that this is the block we wrote
        if( b[0] != 0 )
            diag_printf("b[%d] != %08x\n", 0, 0 );

        if( b[1] != pos )
            diag_printf("b[%d] != %08x\n", 1, (unsigned)pos );

//        if( b[2] != timehash )
//            diag_printf("b[%d] != %08x\n", 2, timehash );

        if( b[3] != 0x12345678 )
            diag_printf("b[%d] != %08x\n", 3, 0x12345678 );

        if( b[(block_size/sizeof(cyg_uint32))-4] != (block_size/sizeof(cyg_uint32)-4) )
            diag_printf("b[%lu](%08x) != %08x\n", (long)block_size/sizeof(cyg_uint32)-4,
                                                  b[(block_size/sizeof(cyg_uint32))-4],
                                                  (unsigned)(block_size/sizeof(cyg_uint32)-4) );

        if( b[block_size/sizeof(cyg_uint32)-3] != pos )
            diag_printf("b[%lu] != %08x\n", (long)block_size/sizeof(cyg_uint32)-3, (unsigned)pos );

//        if( b[block_size/sizeof(cyg_uint32)-2] != timehash )
//            diag_printf("b[%d] != %08x\n", block_size/sizeof(cyg_uint32)-2, timehash );

        if( b[block_size/sizeof(cyg_uint32)-1] != 0x12345678 )
            diag_printf("b[%lu] != %08x\n", (long)block_size/sizeof(cyg_uint32)-1, 0x12345678 );
        
        pos += done;

        time_data[td].valid = 1;
        if( td < (TIME_DATA_MAX-1) )
            td++;
    }
    get_timestamp( &totalt.end );

    free( buf );

    diag_printf("Block      Mount        Open         Seek         Read         Close        Umount\n");

    for( td = 0; td < TIME_DATA_MAX; td++ )
    {
        if( !time_data[td].valid )
            continue;
        
        calculate_interval( &time_data[td].mountt );
        calculate_interval( &time_data[td].opent );
        calculate_interval( &time_data[td].seekt );
        calculate_interval( &time_data[td].readt );
        calculate_interval( &time_data[td].closet );
        calculate_interval( &time_data[td].umountt );

        diag_printf("%2d:", td);
        diag_printf(" %8d.%03d", (int)(time_data[td].mountt.us_interval/1000), (int)(time_data[td].mountt.us_interval%1000) );
        diag_printf(" %8d.%03d", (int)(time_data[td].opent.us_interval/1000), (int)(time_data[td].opent.us_interval%1000) );
        diag_printf(" %8d.%03d", (int)(time_data[td].seekt.us_interval/1000), (int)(time_data[td].seekt.us_interval%1000) );
        diag_printf(" %8d.%03d", (int)(time_data[td].readt.us_interval/1000), (int)(time_data[td].readt.us_interval%1000) );
        diag_printf(" %8d.%03d", (int)(time_data[td].closet.us_interval/1000), (int)(time_data[td].closet.us_interval%1000) );
        diag_printf(" %8d.%03d", (int)(time_data[td].umountt.us_interval/1000), (int)(time_data[td].umountt.us_interval%1000) );
        diag_printf("\n");
    }

    calculate_interval( &totalt );
    diag_printf("CheckFile1 Results: %s size %ld\n", name, pos);
    diag_printf("        Total        %8d.%03d ms\n", (int)(totalt.us_interval/1000), (int)(totalt.us_interval%1000) );
    diag_printf("    Data Rate          %10lld KiB/s\n", (1000000LL*pos)/1024/totalt.us_interval);
    
}

//==========================================================================

void checkcwd( const char *cwd )
{
    static char cwdbuf[PATH_MAX];
    char *ret;

    ret = getcwd( cwdbuf, sizeof(cwdbuf));
    if( ret == NULL ) SHOW_RESULT( getcwd, ret );    

    if( strcmp( cwdbuf, cwd ) != 0 )
    {
        diag_printf( "cwdbuf %s cwd %s\n",cwdbuf, cwd );
        CYG_TEST_FAIL( "Current directory mismatch");
    }
}

//==========================================================================

void fs_callback( cyg_int32 event, CYG_ADDRWORD data )
{
#if 0
    diag_printf("Filesystem %sSAFE\n",
                event==CYG_FS_CALLBACK_UNSAFE?"UN":"");
#endif
}


//==========================================================================
// main

void yaffs5_main( CYG_ADDRESS id )
{
    int err;
    int existingdirents=-1;
    int i;
    timing mountt, umountt;
    
#if defined(CYGSEM_FILEIO_BLOCK_USAGE)
    struct cyg_fs_block_usage usage;
#endif
//    struct cyg_fs_callback_info callback;
    
    CYG_TEST_INIT();

    init_timing();

    for( i = 0 ; date_string[i] != 0 ; i++ )
        timehash = (timehash<<1) ^ date_string[i];
    for( i = 0 ; time_string[i] != 0 ; i++ )
        timehash = (timehash<<1) ^ time_string[i];
    
    // --------------------------------------------------------------

#if 1
#define DO_UMOUNT
    get_timestamp( &mountt.start );
    err = mount( TEST_DEVICE, TEST_MOUNTPOINT, TEST_FSTYPE );    
//    err = mount( TEST_DEVICE, TEST_MOUNTPOINT, "yaffs:sync=write" );    
//    err = mount( TEST_DEVICE, TEST_MOUNTPOINT, "yaffs:sync=close" );    
//    err = mount( TEST_DEVICE, TEST_MOUNTPOINT, "yaffs:sync=data" );    
    get_timestamp( &mountt.end );
    if( err < 0 ) {
        SHOW_RESULT( mount, err );
        CYG_TEST_FAIL_FINISH("yaffs5");
    }

#if 0
    callback.callback = fs_callback;
    callback.data = 0;
    err = cyg_fs_setinfo(TEST_MOUNTPOINT, FS_INFO_CALLBACK,
                         &callback, sizeof(callback));
    if( err < 0 ) SHOW_RESULT( cyg_fs_setinfo, err );
#endif
    
    calculate_interval( &mountt );
    diag_printf("        Mount        %8d.%03d ms\n", (int)(mountt.us_interval/1000), (int)(mountt.us_interval%1000) );
    
    err = access( TEST_MOUNTPOINT TEST_DIRECTORY, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );

    if( err < 0 && errno == EACCES )
    {
        diag_printf("<INFO>: create %s\n",
                    TEST_MOUNTPOINT TEST_DIRECTORY);
        err = mkdir( TEST_MOUNTPOINT TEST_DIRECTORY, 0 );
        if( err < 0 ) SHOW_RESULT( mkdir, err );
    }

    err = chdir( TEST_MOUNTPOINT TEST_DIRECTORY );    
    if( err < 0 ) SHOW_RESULT( chdir, err );

    checkcwd( TEST_MOUNTPOINT TEST_DIRECTORY );
    
    listdir( TEST_MOUNTPOINT TEST_DIRECTORY, true, -1, &existingdirents );

    // --------------------------------------------------------------
#if defined(CYGSEM_FILEIO_BLOCK_USAGE)
    err = cyg_fs_getinfo(TEST_MOUNTPOINT, FS_INFO_BLOCK_USAGE,
                         &usage, sizeof(usage));
    if( err < 0 ) SHOW_RESULT( cyg_fs_getinfo, err );
    diag_printf("<INFO>: total size: %6lld blocks, %10lld bytes\n",
               usage.total_blocks, usage.total_blocks * usage.block_size);
    diag_printf("<INFO>: free size:  %6lld blocks, %10lld bytes\n",
               usage.free_blocks, usage.free_blocks * usage.block_size);
    diag_printf("<INFO>: block size: %6u bytes\n", usage.block_size);
#endif
    // --------------------------------------------------------------
    
#endif
    
#if 1
    createfile( "yaffs5.0", 1024*1024, 16*1024 );
    checkfile( "yaffs5.0",16*1024 );    
    err = unlink( "yaffs5.0" );
    err = cyg_fs_setinfo(TEST_MOUNTPOINT, FS_INFO_SYNC, 0, 0);
    if (err != 0) SHOW_RESULT(sync, err);
#endif

#if 1
    createfile( "yaffs5.0", 200*1024, 1024 );
    checkfile( "yaffs5.0", 1024 );    
    err = unlink( "yaffs5.0" );
#endif
    
#if 1
    createfile( "yaffs5.1", 1024*1024, 1024 );
    createfile( "yaffs5.2", 1024*1024, 4*1024 );
    err = cyg_fs_setinfo(TEST_MOUNTPOINT, FS_INFO_SYNC, 0, 0);
    if (err != 0) SHOW_RESULT(sync, err);
    createfile( "yaffs5.3", 8*1024*1024, 32*1024 );

    checkfile( "yaffs5.1", 1024 );
    checkfile( "yaffs5.2", 4*1024 );
    checkfile( "yaffs5.3", 32*1024 );

    diag_printf("\n--------------------------------------------------------------------\n");
    
    listdir( "." , true, existingdirents+3, NULL );

    // --------------------------------------------------------------

    diag_printf("<INFO>: unlink yaffs5.1\n");    
    err = unlink( "yaffs5.1" );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    diag_printf("<INFO>: unlink yaffs5.2\n");    
    err = unlink( "yaffs5.2" );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    diag_printf("<INFO>: unlink yaffs5.3\n");    
    err = unlink( "yaffs5.3" );
    if( err < 0 ) SHOW_RESULT( unlink, err );
#endif
    
    // --------------------------------------------------------------

#if 1 && defined(DO_UMOUNT)

    diag_printf("<INFO>: unlink yaffs5.4\n");    
    err = unlink( "yaffs5.4" );
    
    createfile( "yaffs5.4", 5*1024*1024, 32*1024 );
#endif
    
    // --------------------------------------------------------------

#ifdef DO_UMOUNT
    diag_printf("<INFO>: cd /\n");    
    err = chdir( "/" );
    if( err == 0 )
    {
        checkcwd( "/" );
    
        diag_printf("<INFO>: umount %s\n",TEST_MOUNTPOINT);
        get_timestamp( &umountt.start );        
        err = umount( TEST_MOUNTPOINT );
        get_timestamp( &umountt.end );        
        if( err < 0 ) SHOW_RESULT( umount, err );

        calculate_interval( &umountt );
        diag_printf("      Umount        %8d.%03d ms\n", (int)(umountt.us_interval/1000), (int)(umountt.us_interval%1000) );
        
    }
#endif
    
#if 1

    diag_printf("<INFO>: syncing\n");    
    err = cyg_fs_setinfo(TEST_MOUNTPOINT, FS_INFO_SYNC, 0, 0);
    if (err != 0) SHOW_RESULT(sync, err);
        
    checkfile1(TEST_MOUNTPOINT TEST_DIRECTORY "/yaffs5.4", 32*1024 );

    // Remount just to remove yaffs5.4
    err = mount( TEST_DEVICE, TEST_MOUNTPOINT, TEST_FSTYPE );    
    if( err < 0 ) {
        SHOW_RESULT( mount, err );
        CYG_TEST_FAIL_FINISH("yaffs5");
    }
    err = unlink( TEST_MOUNTPOINT TEST_DIRECTORY "/yaffs5.4" );
    if( err < 0 ) SHOW_RESULT( unlink, err );
    err = umount( TEST_MOUNTPOINT );
    if( err < 0 ) SHOW_RESULT( umount, err );

#else

    // don't sync if not mounted
    diag_printf("<INFO>: syncing\n");    
    err = cyg_fs_setinfo(TEST_MOUNTPOINT, FS_INFO_SYNC, 0, 0);
    if (err != 0) SHOW_RESULT(sync, err);

#endif
    
    CYG_TEST_PASS_FINISH("yaffs5");
}

// -------------------------------------------------------------------------

#include <cyg/kernel/kapi.h>

static cyg_handle_t thread_handle;
static cyg_thread thread;
static char stack[CYGNUM_HAL_STACK_SIZE_TYPICAL+8*1024];
//static char stack[64*1024];

externC void
cyg_start( void )
{
    cyg_thread_create(3,                // Priority - just a number
                      yaffs5_main,      // entry
                      0,                // index
                      0,                // no name
                      &stack[0],        // Stack
                      sizeof(stack),    // Size
                      &thread_handle,   // Handle
                      &thread           // Thread data structure
        );
    cyg_thread_resume(thread_handle);

    cyg_scheduler_start();
}

#else // CYGPKG_KERNEL

#include <cyg/infra/testcase.h>

externC void cyg_start(void)
{
    CYG_TEST_NA("Needs CYGPKG_KERNEL");
}

#endif

// -------------------------------------------------------------------------
// EOF yaffs5.c
