//==========================================================================
//
//      yaffs2.c
//
//      Test fileio system
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
// Date:                2000-05-25
// Purpose:             Test fileio system
// Description:         This test uses the testfs to check out the initialization
//                      and basic operation of the fileio system
//                      
//                      
//                      
//                      
//                      
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/system.h>

#ifdef CYGPKG_KERNEL

#include <pkgconf/hal.h>
#include <pkgconf/kernel.h>
#include <pkgconf/io_fileio.h>
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

#include <cyg/fileio/fileio.h>

#include <cyg/infra/testcase.h>
#include <cyg/infra/diag.h>            // HAL polled output

#include <cyg/kernel/instrmnt.h>

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

#if 0
# define X(_args_)                              \
{                                               \
    cyg_drv_isr_lock();                         \
    diag_printf("%08x: ",cyg_thread_self());    \
    diag_printf _args_;                         \
    cyg_drv_isr_unlock();                       \
                                                \
}
#else
# define X(_args_) CYG_EMPTY_STATEMENT
#endif

#ifdef JNGPKG_USB_STACK
extern int usb_stack_init(void);
#endif

//==========================================================================

#define SHOW_RESULT( _fn, _res ) \
diag_printf("<FAIL>: " #_fn "() returned %ld %s\n", (long)_res, _res<0?strerror(errno):"");

//==========================================================================

#define IOSIZE  100

#define NTHREAD 6


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


static cyg_mutex_t db_lock;

#define db_printf( args... )                    \
{                                               \
    cyg_mutex_lock( &db_lock );                 \
    diag_printf("%d: ",id);                     \
    diag_printf( args );                        \
    cyg_mutex_unlock( &db_lock );               \
}

//==========================================================================

static void listdir( char *name, int statp, int numexpected, int *numgot, int id )
{
    int err;
    DIR *dirp;
    int num=0;
    
    db_printf("<INFO>: reading directory %s\n",name);

    dirp = opendir( name );
    if( dirp == NULL ) SHOW_RESULT( opendir, -1 );

    for(;;)
    {
        struct dirent entry_data;
        struct dirent *entry;
        
        err = readdir_r( dirp, &entry_data, &entry );
        
        if( err < 0 || entry == NULL )
            break;
        num++;
        
        cyg_mutex_lock( &db_lock );
        diag_printf("%d: <INFO>: entry %14s",id, entry->d_name);
        
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
                else
                {
                    SHOW_RESULT( stat, err );
                    cyg_scheduler_lock();
                    CYG_INSTRUMENT_USER(0xff,err,errno);
                    for(;;);
                }
            }
            else
            {
                diag_printf(" [mode %08x ino %08x nlink %d size %ld]",
                            sbuf.st_mode,sbuf.st_ino,sbuf.st_nlink,sbuf.st_size);
            }
        }

        diag_printf("\n");
        cyg_mutex_unlock( &db_lock );
    }

    err = closedir( dirp );
    if( err < 0 ) SHOW_RESULT( stat, err );
    if (numexpected >= 0 && num != numexpected)
        CYG_TEST_FAIL("Wrong number of dir entries\n");
    if ( numgot != NULL )
        *numgot = num;
}

//==========================================================================

static void createfile( char *name, size_t size, int id  )
{
    char buf[IOSIZE];
    int fd;
    ssize_t wrote;
    int i;
    int err;

    db_printf("<INFO>: create file %s size %ld\n",name, (long)size);

    err = access( name, F_OK );
    if( err < 0 && errno == EEXIST )
    {
        err = unlink( name );
        if( err < 0 ) SHOW_RESULT( unlink, err );
    }
    else
        if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );
    
    for( i = 0; i < IOSIZE; i++ ) buf[i] = i%256;
 
    fd = open( name, O_WRONLY|O_CREAT );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    while( size > 0 )
    {
        ssize_t len = size;
        if ( len > IOSIZE ) len = IOSIZE;
        
        wrote = write( fd, buf, len );
        if( wrote != len ) SHOW_RESULT( write, wrote );        

        size -= wrote;

        cyg_thread_yield();
    }

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

//==========================================================================

#if 0
static void maxfile( char *name, int id  )
{
    char buf[IOSIZE];
    int fd;
    ssize_t wrote;
    int i;
    int err;
    size_t size = 0;
    size_t prevsize = 0;
    
    db_printf("<INFO>: create maximal file %s\n",name);
    db_printf("<INFO>: This may take a few minutes\n");

    err = access( name, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );
    
    for( i = 0; i < IOSIZE; i++ ) buf[i] = i%256;
 
    fd = open( name, O_WRONLY|O_CREAT );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    do
    {
        wrote = write( fd, buf, IOSIZE );
        //if( wrote < 0 ) SHOW_RESULT( write, wrote );        

        if( wrote >= 0 )
            size += wrote;

        if( (size-prevsize) > 100000 )
        {
            db_printf("<INFO>: size = %ld \n", size);
            prevsize = size;
        }
        
    } while( wrote == IOSIZE );

    db_printf("<INFO>: file size == %ld\n",size);

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}
#endif

//==========================================================================

static void checkfile( char *name, int id  )
{
    char buf[IOSIZE];
    int fd;
    ssize_t done;
    int i;
    int err;
    off_t pos = 0;

    db_printf("<INFO>: check file %s\n",name);
    
    err = access( name, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );

    fd = open( name, O_RDONLY );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    for(;;)
    {
        done = read( fd, buf, IOSIZE );
        if( done < 0 ) SHOW_RESULT( read, done );

        if( done == 0 ) break;

        for( i = 0; i < done; i++ )
            if( buf[i] != i%256 )
            {
//                db_printf("buf[%d+%d](%02x) != %02x\n",pos,i,buf[i],i%256);
//                CYG_TEST_FAIL("Data read not equal to data written\n");
                X(("buf[%ld+%d](%02x) != %02x\n",pos,i,buf[i],i%256));
                cyg_scheduler_lock();
                CYG_INSTRUMENT_USER(0xf0,err,errno);
                for(;;);
            }
        
        pos += done;

        cyg_thread_yield();
    }

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

//==========================================================================

static void copyfile( char *name2, char *name1, int id  )
{

    int err;
    char buf[IOSIZE];
    int fd1, fd2;
    ssize_t done, wrote;

    db_printf("<INFO>: copy file %s -> %s\n",name2,name1);

    err = access( name1, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );

    err = access( name2, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );
    
    fd1 = open( name1, O_WRONLY|O_CREAT );
    if( fd1 < 0 ) SHOW_RESULT( open, fd1 );

    fd2 = open( name2, O_RDONLY );
    if( fd2 < 0 ) SHOW_RESULT( open, fd2 );
    
    for(;;)
    {
        done = read( fd2, buf, IOSIZE );
        if( done < 0 ) SHOW_RESULT( read, done );

        if( done == 0 ) break;

        wrote = write( fd1, buf, done );
        if( wrote != done ) SHOW_RESULT( write, wrote );

        if( wrote != done ) break;

        cyg_thread_yield();
    }

    err = close( fd1 );
    if( err < 0 ) SHOW_RESULT( close, err );

    err = close( fd2 );
    if( err < 0 ) SHOW_RESULT( close, err );
    
}

//==========================================================================

static void comparefiles( char *name2, char *name1, int id  )
{
    int err;
    char buf1[IOSIZE];
    char buf2[IOSIZE];
    int fd1, fd2;
    ssize_t done1, done2;
    int i;

    db_printf("<INFO>: compare files %s == %s\n",name2,name1);

    err = access( name1, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );

    err = access( name1, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );
    
    fd1 = open( name1, O_RDONLY );
    if( fd1 < 0 ) SHOW_RESULT( open, fd1 );

    fd2 = open( name2, O_RDONLY );
    if( fd2 < 0 ) SHOW_RESULT( open, fd2 );
    
    for(;;)
    {
        done1 = read( fd1, buf1, IOSIZE );
        if( done1 < 0 ) SHOW_RESULT( read, done1 );

        done2 = read( fd2, buf2, IOSIZE );
        if( done2 < 0 ) SHOW_RESULT( read, done2 );

        if( done1 != done2 )
            db_printf("Files different sizes\n");
        
        if( done1 == 0 ) break;

        for( i = 0; i < done1; i++ )
            if( buf1[i] != buf2[i] )
            {
//                db_printf("buf1[%d](%02x) != buf1[%d](%02x)\n",i,buf1[i],i,buf2[i]);
//                CYG_TEST_FAIL("Data in files not equal\n");
                X(("%s[%d](%02x) != %s[%d](%02x)\n",name1,i,buf1[i],name2,i,buf2[i]));
                cyg_scheduler_lock();
                CYG_INSTRUMENT_USER(0xf1,err,errno);
                for(;;);
            }

        cyg_thread_yield();
    }

    err = close( fd1 );
    if( err < 0 ) SHOW_RESULT( close, err );

    err = close( fd2 );
    if( err < 0 ) SHOW_RESULT( close, err );
    
}

//==========================================================================

void checkcwd( const char *cwd, int id  )
{
    static char cwdbuf[PATH_MAX];
    char *ret;

    ret = getcwd( cwdbuf, sizeof(cwdbuf));
    if( ret == NULL ) SHOW_RESULT( getcwd, ret );    

    if( strcmp( cwdbuf, cwd ) != 0 )
    {
        db_printf( "cwdbuf %s cwd %s\n",cwdbuf, cwd );
        CYG_TEST_FAIL( "Current directory mismatch");
    }
}

//==========================================================================
// main

static cyg_sem_t sem;

void fileio2_main( CYG_ADDRESS id )
{
    int err;
    int existingdirents=-1;

    char dirname[64];
    char filename1[64];
    char filename2[64];

#define NEW_FILENAME( fn, f )                           \
{                                                       \
    fn[0]=0;                                            \
    strcat(fn, dirname);                                \
    strcat(fn, "/");                                    \
    strcat(fn,f);                                       \
}

#define NEW_FILENAME1( f ) NEW_FILENAME( filename1, f )
#define NEW_FILENAME2( f ) NEW_FILENAME( filename2, f )
    
    // --------------------------------------------------------------

    if( id == 0 )
    {
        int i;

#ifdef JNGPKG_USB_STACK
        err = usb_stack_init();
        cyg_thread_delay( 1000 );
#endif
    
        CYG_TEST_INIT();
        
        err = mount( TEST_DEVICE, TEST_MOUNTPOINT, TEST_FSTYPE );
        if( err < 0 ) {
            SHOW_RESULT( mount, err );
            CYG_TEST_FAIL_FINISH("yaffs2");
        }

        err = access( TEST_MOUNTPOINT TEST_DIRECTORY, F_OK );
        if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );

        if( err < 0 && errno == EACCES )
        {
            db_printf("<INFO>: create %s\n",
                      TEST_MOUNTPOINT TEST_DIRECTORY);
            err = mkdir( TEST_MOUNTPOINT TEST_DIRECTORY, 0 );
            if( err < 0 ) SHOW_RESULT( mkdir, err );
        }
        
        err = chdir( TEST_MOUNTPOINT TEST_DIRECTORY );
        if( err < 0 ) SHOW_RESULT( chdir, err );

        checkcwd( TEST_MOUNTPOINT TEST_DIRECTORY, id );
    
        for( i = 0; i < NTHREAD-1; i++ )
            cyg_semaphore_post( &sem );
    }
    else
    {
        cyg_semaphore_wait( &sem );
    }

    // --------------------------------------------------------------

    dirname[0] = 0;
    strcat(dirname, TEST_MOUNTPOINT );
    strcat(dirname, TEST_DIRECTORY );
    strcat(dirname, "/dirX");
    dirname[sizeof(TEST_MOUNTPOINT) +
            sizeof(TEST_DIRECTORY) - 2 +
            4] = '0' + id;
//    dirname[5] = 0;

    
    db_printf("<INFO>: create %s\n",dirname);        
    err = mkdir( dirname, 0 );
    if( err < 0 && errno != EEXIST ) SHOW_RESULT( mkdir, err );

    listdir( dirname, true, -1, &existingdirents, id );

    db_printf("<INFO>: existingdirents %d\n",existingdirents);
    
    // --------------------------------------------------------------

    NEW_FILENAME1( "foo" );
    createfile( filename1, 20257, id );
    checkfile( filename1, id );

    NEW_FILENAME1( "foo" );
    NEW_FILENAME2( "fee" );
    
    copyfile( filename1, filename2, id);
    checkfile( filename2, id );
    comparefiles( filename1, filename2, id );

    NEW_FILENAME1( "bar" );
    db_printf("<INFO>: mkdir bar\n");
    err = mkdir( filename1, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );

    listdir( dirname , true, existingdirents+3, NULL, id );

    NEW_FILENAME1( "bar/fum" );
    copyfile( filename2, filename1, id );
    checkfile( filename1, id );
    comparefiles( filename2, filename1, id );

    NEW_FILENAME1( "foo" );    
    NEW_FILENAME2( "bar/bundy" );    
    db_printf("<INFO>: rename %s -> %s\n",filename1,filename2);    
    err = rename( filename1, filename2 );
    if( err < 0 ) SHOW_RESULT( rename, err );

    NEW_FILENAME1( "bar" );
    
    listdir( dirname, true, existingdirents+2, NULL, id );
    listdir( filename1 , true, 4, NULL, id );

    NEW_FILENAME1( "fee" );        
    checkfile( filename2, id );
    comparefiles( filename1, filename2, id );

//    db_printf("spinning\n"); for(;;) cyg_thread_delay(10);
    
    
    // --------------------------------------------------------------

    db_printf("<INFO>: unlink %s\n",filename1);    
    err = unlink( filename1 );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    NEW_FILENAME1( "bar/fum" );    
    db_printf("<INFO>: unlink %s\n",filename1);        
    err = unlink( filename1 );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    NEW_FILENAME1( "bar/bundy" );        
    db_printf("<INFO>: unlink %s\n",filename1);        
    err = unlink( filename1 );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    NEW_FILENAME1( "bar" );            
    db_printf("<INFO>: rmdir %s\n",filename1);        
    err = rmdir( filename1 );
    if( err < 0 ) SHOW_RESULT( rmdir, err );
    
//    listdir( dirname, false, existingdirents, NULL, id );

        listdir( dirname, true, existingdirents, NULL, id );

    // --------------------------------------------------------------

    
    listdir( dirname , true, -1, &existingdirents, id);

    NEW_FILENAME1( "tinky" );            
    NEW_FILENAME2( "laalaa" );            
    createfile( filename1, 4567, id );
    copyfile( filename1, filename2, id );
    checkfile( filename1, id);
    checkfile( filename2, id);
    comparefiles( filename1, filename2, id );

    NEW_FILENAME1( "noonoo" );                
    db_printf("<INFO>: mkdir %s\n",filename1);    
    err = mkdir( filename1, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );

    listdir( dirname , true, existingdirents+3, NULL, id);

    NEW_FILENAME1( "noonoo/tinky" );            
    createfile( filename1, 6789, id );
    checkfile( filename1, id );

    NEW_FILENAME1( "noonoo/dipsy" );            
    NEW_FILENAME2( "noonoo/po" );            
    createfile( filename1, 34567, id );
    checkfile( filename1, id );
    copyfile( filename1, filename2, id );
    checkfile( filename2, id );
    comparefiles( filename1, filename2, id );

    NEW_FILENAME1( "noonoo" );                    
    listdir( filename1, true, 5, NULL, id );
    listdir( dirname, true, existingdirents+3, NULL, id );

    // --------------------------------------------------------------

    NEW_FILENAME1( "noonoo/tinky" );                
    db_printf("<INFO>: unlink %s\n",filename1);    
    err = unlink( filename1 );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    NEW_FILENAME1( "noonoo/dipsy" );                
    db_printf("<INFO>: unlink %s\n",filename1);    
    err = unlink( filename1 );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    NEW_FILENAME1( "noonoo/po" );
    db_printf("<INFO>: unlink %s\n",filename1);    
    err = unlink( filename1 );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    NEW_FILENAME1( "noonoo" );                        
    db_printf("<INFO>: rmdir %s\n",filename1); 
    err = rmdir( filename1 );
    if( err < 0 ) SHOW_RESULT( rmdir, err );

    listdir( dirname, true, existingdirents+2, NULL, id );
    
    // --------------------------------------------------------------

    
    NEW_FILENAME1( "tinky" );
    db_printf("<INFO>: unlink %s\n",filename1);    
    err = unlink( filename1 );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    NEW_FILENAME1( "laalaa" );
    db_printf("<INFO>: unlink %s\n",filename1);    
    err = unlink( filename1 );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    listdir( dirname, true, -1, NULL, id );

#if 0
    NEW_FILENAME1( "file.max" );
    maxfile( filename1, id );

    listdir( dirname, true, -1, NULL, id );    
        
    db_printf("<INFO>: unlink %s\n",filename1);    
    err = unlink( filename1 );
    if( err < 0 ) SHOW_RESULT( unlink, err );    
#endif

    db_printf("<INFO>: rmdir %s\n",dirname);    
    err = rmdir( dirname );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    db_printf("<INFO>: syncing\n");
    err = cyg_fs_setinfo(TEST_MOUNTPOINT, FS_INFO_SYNC, 0, 0);
    if (err != 0) SHOW_RESULT(sync, err);
    
    if( id == 0 )
    {
        int i;
        db_printf("<INFO>: waiting for other threads to finish\n");        
        for( i = 0; i < NTHREAD-1; i++ )
            cyg_semaphore_wait( &sem );

        CYG_TEST_PASS_FINISH("yaffs2");
    }
    else
    {
        db_printf("<INFO>: thread finished\n");
        cyg_semaphore_post( &sem );
    }
    
    cyg_thread_exit();
}

// -------------------------------------------------------------------------

#include <cyg/kernel/kapi.h>

#define STACK_SIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL*2)

static char stack[NTHREAD][STACK_SIZE];
static cyg_handle_t thread_handle[NTHREAD];
static cyg_thread thread[NTHREAD];

externC void
cyg_start( void )
{
    int i;

    CYG_INSTRUMENT_USER(0x01,0,0);
                    
    cyg_mutex_init( &db_lock );
    
    cyg_semaphore_init( &sem, 0 );
    
    for( i = 0; i < NTHREAD; i++ )
    {
        cyg_thread_create(3,                    // Priority - just a number
                          fileio2_main,         // entry
                          i,                    // index
                          0,                    // no name
                          &stack[i][0],         // Stack
                          STACK_SIZE,           // Size
                          &thread_handle[i],    // Handle
                          &thread[i]            // Thread data structure
            );
        cyg_thread_resume(thread_handle[i]);
    }

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
// EOF yaffs2.c
