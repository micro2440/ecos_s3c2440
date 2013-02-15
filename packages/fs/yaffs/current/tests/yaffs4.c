//==========================================================================
//
//      yaffs4.c
//
//      Test fileio system
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.
// Copyright (C) 2004, 2005, 2006, 2007 eCosCentric Limited                 
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
// Date:                2005-07-25
// Purpose:             Test fileio system
// Description:         This test uses the testfs to check out the initialization
//                      and basic operation of the fileio system.
//                      This is essentially a copy of yaffs1 but using long
//                      file names.
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
    diag_printf("<FAIL>: " #_fn "() returned %ld %s\n", (long)_res, _res<0?strerror(errno):""); \
}

//==========================================================================

#define IOSIZE  100

#define CLEANUP

#define LONGNAME1       "long_file_name_that_should_take_up_more_than_one_directory_entry_1"
#define LONGNAME2       "long_file_name_that_should_take_up_more_than_one_directory_entry_2"

//                       123456789012345678901234567890
#define FOO             "foo_longer_file_x.filename"
//#define FEE             "fee_.filename"
//#define BAR             "bar_long_directory_name.dirname"
#define FEE             "a234567890123"
#define BAR             "a2345678901234"
#define BUNDY           "bundy_long_file_name_ccccccccccccccccccccccccccccccc.filename"
#define FUM             "bundy_long_file_name_fum.filename"
#define DOO             "doo_aaaaaaaaaabbbbbbbbbbbccccccccccccc"
#define FIE             "fie.rrrrrrrrrrrrllllllllllllvvvvvvvvvvvvv"
#define FOE             "foe_xxxxxxxxxxllllll.xxxxxxeeeeeeeeee"

#define TINKY           "tinky_long_file_name.filename"
#define LAALAA          "laalaa_long_file_name.filename"
#define NOONOO          "noonoo_long_directory_name.dirname"
#define DIPSY           "dipsy_long_file_name.filename"
#define PO              "po_long_file_name.filename"

#define XXXX            "xxxx_long_directory_name.dirname"
#define YYYY            "yyyy_long_directory_name.dirname"
#define ZZZZ            "zzzz_long_directory_name.dirname"
#define WWWW            "wwww_long_directory_name.dirname"


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
        diag_printf("<INFO>: entry");
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
                diag_printf(" [mode %08x ino %08x nlink %d size %6ld]",
                            sbuf.st_mode,sbuf.st_ino,sbuf.st_nlink,sbuf.st_size);
            }
        }

        diag_printf(" %s\n",entry->d_name);        
    }

//    diag_printf("<INFO>: closedir(%s)\n",name);    
    err = closedir( dirp );
    if( err < 0 ) SHOW_RESULT( stat, err );
    if (numexpected >= 0 && num != numexpected)
        CYG_TEST_FAIL("Wrong number of dir entries");
    if ( numgot != NULL )
        *numgot = num;
}

//==========================================================================

static void createfile( char *name, size_t size )
{
    char buf[IOSIZE];
    int fd;
    ssize_t wrote;
    int i;
    int err;

    diag_printf("<INFO>: create file %s size %ld\n",name, (long)size);

    err = access( name, F_OK );
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
    }

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

//==========================================================================

#if 0
static void maxfile( char *name )
{
    char buf[IOSIZE];
    int fd;
    ssize_t wrote;
    int i;
    int err;
    size_t size = 0;
    size_t prevsize = 0;
    
    diag_printf("<INFO>: create maximal file %s\n",name);
    diag_printf("<INFO>: This may take a few minutes\n");

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
            diag_printf("<INFO>: size = %ld \n", size);
            prevsize = size;
        }
        
    } while( wrote == IOSIZE );

    diag_printf("<INFO>: file size == %ld\n",size);

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}
#endif

//==========================================================================

static void checkfile( char *name )
{
    char buf[IOSIZE];
    int fd;
    ssize_t done;
    int i;
    int err;
    off_t pos = 0;

    diag_printf("<INFO>: check file %s\n",name);
    
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
                diag_printf("buf[%ld+%d](%02x) != %02x\n",pos,i,buf[i],i%256);
                CYG_TEST_FAIL("Data read not equal to data written\n");
                for(;;);
            }
        
        pos += done;
    }

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

//==========================================================================

static void copyfile( char *name2, char *name1 )
{

    int err;
    char buf[IOSIZE];
    int fd1, fd2;
    ssize_t done, wrote;

    diag_printf("<INFO>: copy file %s -> %s\n",name2,name1);

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
    }

    err = close( fd1 );
    if( err < 0 ) SHOW_RESULT( close, err );

    err = close( fd2 );
    if( err < 0 ) SHOW_RESULT( close, err );
    
}

//==========================================================================

static void comparefiles( char *name2, char *name1 )
{
    int err;
    char buf1[IOSIZE];
    char buf2[IOSIZE];
    int fd1, fd2;
    ssize_t done1, done2;
    int i;

    diag_printf("<INFO>: compare files %s == %s\n",name2,name1);

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
            diag_printf("Files different sizes\n");
        
        if( done1 == 0 ) break;

        for( i = 0; i < done1; i++ )
            if( buf1[i] != buf2[i] )
            {
                diag_printf("buf1[%d](%02x) != buf1[%d](%02x)\n",i,buf1[i],i,buf2[i]);
                CYG_TEST_FAIL("Data in files not equal\n");
            }
    }

    err = close( fd1 );
    if( err < 0 ) SHOW_RESULT( close, err );

    err = close( fd2 );
    if( err < 0 ) SHOW_RESULT( close, err );
    
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
// main

void fileio4_main( CYG_ADDRESS id )
{
    int err;
    int existingdirents=-1;
    cyg_bool mp2 = false;
    int fd;
    
    char dirname[256];
    char filename1[256];
    char filename2[256];

#define NEW_FILENAME( fn, f )                           \
{                                                       \
    fn[0]=0;                                            \
    strcat(fn, dirname);                                \
    strcat(fn, "/");                                    \
    strcat(fn,f);                                       \
}

#define NEW_FILENAME1( f ) NEW_FILENAME( filename1, f )
#define NEW_FILENAME2( f ) NEW_FILENAME( filename2, f )

    CYG_TEST_INIT();
    
    // --------------------------------------------------------------

    err = mount( TEST_DEVICE, TEST_MOUNTPOINT, TEST_FSTYPE );    
    if( err < 0 ) {
        SHOW_RESULT( mount, err );
        CYG_TEST_FAIL_FINISH("yaffs4");
    }

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
    
    dirname[0] = 0;
    strcat(dirname, TEST_MOUNTPOINT );
    strcat(dirname, TEST_DIRECTORY );
    
    // --------------------------------------------------------------

    createfile( FOO, 20257 );    
    checkfile( FOO );
    copyfile( FOO, FEE);
    checkfile( FEE );
    comparefiles( FOO, FEE );
    diag_printf("<INFO>: mkdir " BAR "\n");
    err = mkdir( BAR, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );

    listdir( dirname , true, existingdirents+3, NULL );
    
    copyfile( FEE, BAR "/" FUM );
    checkfile( BAR "/" FUM );
    comparefiles( FEE, BAR "/" FUM );

    diag_printf("<INFO>: cd " BAR "\n");
    err = chdir( BAR );
    if( err < 0 ) SHOW_RESULT( chdir, err );

    NEW_FILENAME1( BAR );
    checkcwd( filename1 );
    
    diag_printf("<INFO>: rename ../" FOO " " BUNDY "\n");    
    err = rename( "../" FOO, BUNDY );
    if( err < 0 ) SHOW_RESULT( rename, err );
    
    listdir( dirname, true, existingdirents+2, NULL );
    listdir( "." , true, 4, NULL );

    checkfile( "../" BAR "/" BUNDY );
    comparefiles("../" FEE, BUNDY );

    // --------------------------------------------------------------
    // Check for some common error cases

    // First make some directories
    diag_printf("<INFO>: mkdir " DOO "\n");    
    err = mkdir( DOO, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );    

    diag_printf("<INFO>: mkdir " FIE" \n");    
    err = mkdir( FIE, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );    

    diag_printf("<INFO>: mkdir " FIE "/" FOE "\n");    
    err = mkdir( FIE "/" FOE, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );    

    
    diag_printf("<INFO>: rename " FUM " " DOO " -- should fail\n");    
    err = rename( FUM, DOO );
    if( err < 0 && errno != EISDIR ) SHOW_RESULT( rename, err );

    diag_printf("<INFO>: rename " DOO " " FUM " -- should fail\n");    
    err = rename( DOO, FUM );
    if( err < 0 && errno != ENOTDIR ) SHOW_RESULT( rename, err );

    diag_printf("<INFO>: rename " DOO " " FIE " -- should fail\n");    
    err = rename( DOO, FIE );
    if( err < 0 && errno != EIO ) SHOW_RESULT( rename, err );
    
    NEW_FILENAME1( XXXX "/" YYYY )
    diag_printf("<INFO>: rename " FUM " %s -- should fail\n",filename1);    
    err = rename( FUM, filename1 );
    if( err < 0 && errno != ENOENT ) SHOW_RESULT( rename, err );
    
    diag_printf("<INFO>: rename " FUM " " FUM " -- should fail\n");    
    err = rename( FUM, FUM );
    if( err < 0 && errno != ENOENT ) SHOW_RESULT( rename, err );

    diag_printf("<INFO>: unlink " FEE " -- should fail\n");        
    err = unlink( FEE );
    if( err < 0 && errno != ENOENT ) SHOW_RESULT( unlink, err );

    NEW_FILENAME1( BAR );
    diag_printf("<INFO>: rmdir %s -- should fail\n",filename1);        
    err = rmdir( filename1 );
    if( err < 0 && errno != ENOTEMPTY ) SHOW_RESULT( rmdir, err );

    diag_printf("<INFO>: unlink %s -- should fail\n",filename1);        
    err = unlink( filename1 );
    if( err < 0 && errno != EISDIR ) SHOW_RESULT( unlink, err );

    NEW_FILENAME1( BAR "/" FUM );
    diag_printf("<INFO>: rmdir %s -- should fail\n",filename1);        
    err = rmdir( filename1 );
    if( err < 0 && errno != ENOTDIR ) SHOW_RESULT( rmdir, err );

    diag_printf("<INFO>: mkdir %s -- should fail\n",filename1);        
    err = mkdir( filename1, 0 );
    if( err < 0 && errno != EEXIST ) SHOW_RESULT( mkdir, err );
    
    diag_printf("<INFO>: open " BUNDY "  exclusive -- should fail\n");        
    fd = open( BUNDY, O_WRONLY|O_CREAT|O_EXCL );
    if( fd < 0 && errno != EEXIST ) SHOW_RESULT( open, fd );

    diag_printf("<INFO>: open %s -- should fail\n",dirname);        
    fd = open( dirname, O_WRONLY );
    if( fd < 0 && errno != EISDIR ) SHOW_RESULT( open, fd );
    
    
    listdir( dirname, true, existingdirents+2, NULL );
    listdir( "." , true, 6, NULL );
    
    // --------------------------------------------------------------    

#ifdef CLEANUP

    diag_printf("<INFO>: rename " FIE " " DOO "\n");
    err = rename( FIE, DOO );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    diag_printf("<INFO>: rmdir " DOO "/" FOE "\n");
    err = rmdir(DOO "/" FOE);
    if( err < 0 ) SHOW_RESULT( unlink, err );    

    diag_printf("<INFO>: rmdir " DOO "\n");
    err = rmdir(DOO);
    if( err < 0 ) SHOW_RESULT( unlink, err );    
        
    diag_printf("<INFO>: unlink ../" FEE "\n");    
    err = unlink( "../" FEE );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    diag_printf("<INFO>: unlink " FUM "\n");        
    err = unlink( FUM );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    diag_printf("<INFO>: unlink ../" BAR "/" BUNDY "\n");        
    err = unlink( "../" BAR "/" BUNDY );
    if( err < 0 ) SHOW_RESULT( unlink, err );
#endif
    
    diag_printf("<INFO>: cd ..\n");        
    err = chdir( ".." );
    if( err < 0 ) SHOW_RESULT( chdir, err );

    checkcwd( dirname );

#ifdef CLEANUP    
    diag_printf("<INFO>: rmdir " BAR "\n");        
    err = rmdir( BAR );
    if( err < 0 ) SHOW_RESULT( rmdir, err );
    
    listdir( dirname, false, existingdirents, NULL );
#endif
    
    // --------------------------------------------------------------

#ifdef TEST_MOUNTPOINT2
    diag_printf("<INFO>: mount %s\n", TEST_MOUNTPOINT2);    
    err = mount( TEST_DEVICE2, TEST_MOUNTPOINT2, TEST_FSTYPE2 );

    if( err == 0 )
    {
        mp2 = true;
        diag_printf("<INFO>: mount %s succeeded\n", TEST_MOUNTPOINT2);    
        err = access( TEST_MOUNTPOINT2 TEST_DIRECTORY2, F_OK );
        if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );

        if( err < 0 && errno == EACCES )
        {
            diag_printf("<INFO>: create %s\n",
                        TEST_MOUNTPOINT2 TEST_DIRECTORY2);
            err = mkdir( TEST_MOUNTPOINT2 TEST_DIRECTORY2, 0 );
            if( err < 0 ) SHOW_RESULT( mkdir, err );
        }

        err = chdir( TEST_MOUNTPOINT2 TEST_DIRECTORY2 );    
        if( err < 0 ) SHOW_RESULT( chdir, err );

        checkcwd( TEST_MOUNTPOINT2 TEST_DIRECTORY2 );

        dirname[0] = 0;
        strcat(dirname, TEST_MOUNTPOINT2 );
        strcat(dirname, TEST_DIRECTORY2 );

    }
    else
    {
        mp2 = false;
        diag_printf("<INFO>: mount %s failed, ", TEST_MOUNTPOINT2);
        diag_printf("using %s instead\n", dirname );
    }

#endif
    
    listdir( dirname , true, -1, &existingdirents);

    NEW_FILENAME1( TINKY );
    NEW_FILENAME2( LAALAA );
    
    createfile( filename1, 4567 );
    copyfile( filename1, filename2 );
    checkfile( filename1);
    checkfile( filename2);
    comparefiles( filename1, filename2 );

    diag_printf("<INFO>: cd %s\n",dirname);    
    err = chdir( dirname );
    if( err < 0 ) SHOW_RESULT( chdir, err );

    checkcwd( dirname );
        
    diag_printf("<INFO>: mkdir " NOONOO "\n");    
    err = mkdir( NOONOO, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );

    listdir( dirname , true, existingdirents+3, NULL);

    diag_printf("<INFO>: cd " NOONOO "\n");
    err = chdir( NOONOO );
    if( err < 0 ) SHOW_RESULT( chdir, err );

    NEW_FILENAME1( NOONOO );
    checkcwd( filename1 );
    
    createfile( TINKY, 6789 );
    checkfile( TINKY );

    createfile( DIPSY, 34567 );
    checkfile( DIPSY );
    copyfile( DIPSY, PO );
    checkfile( PO );
    comparefiles( DIPSY, PO );

    listdir( ".", true, 5, NULL );
    listdir( "..", true, existingdirents+3, NULL );

    // --------------------------------------------------------------

#ifdef CLEANUP    
    diag_printf("<INFO>: unlink " TINKY "\n");    
    err = unlink( TINKY );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    diag_printf("<INFO>: unlink " DIPSY "\n");    
    err = unlink( DIPSY );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    diag_printf("<INFO>: unlink " PO "\n");    
    err = unlink( PO );
    if( err < 0 ) SHOW_RESULT( unlink, err );
#endif
    
    diag_printf("<INFO>: cd ..\n"); 
    err = chdir( ".." );
    if( err < 0 ) SHOW_RESULT( chdir, err );
    checkcwd( dirname );

#ifdef CLEANUP    
    diag_printf("<INFO>: rmdir " NOONOO "\n"); 
    err = rmdir( NOONOO );
    if( err < 0 ) SHOW_RESULT( rmdir, err );
#endif
    
    // --------------------------------------------------------------

    diag_printf("<INFO>: mkdir " XXXX "\n");    
    err = mkdir( XXXX, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );

    diag_printf("<INFO>: mkdir " XXXX "/" YYYY "\n");        
    err = mkdir( XXXX "/" YYYY, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );

    diag_printf("<INFO>: mkdir " XXXX "/" YYYY "/" ZZZZ "\n");        
    err = mkdir( XXXX "/" YYYY "/" ZZZZ, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );

    diag_printf("<INFO>: mkdir " XXXX "/" YYYY "/" ZZZZ "/" WWWW "\n");    
    err = mkdir( XXXX "/" YYYY "/" ZZZZ "/" WWWW, 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );

    NEW_FILENAME1( XXXX "/" YYYY "/" ZZZZ "/" WWWW );
    diag_printf("<INFO>: cd %s\n",filename1);
    err = chdir( filename1 );
    if( err < 0 ) SHOW_RESULT( chdir, err );
    checkcwd( filename1 );

    NEW_FILENAME1( XXXX "/" YYYY "/" ZZZZ );    
    diag_printf("<INFO>: cd ..\n");
    err = chdir( ".." );
    if( err < 0 ) SHOW_RESULT( chdir, err );
    checkcwd( filename1 );

    NEW_FILENAME1( XXXX "/" YYYY "/" ZZZZ );
    diag_printf("<INFO>: cd .\n");
    err = chdir( "." );
    if( err < 0 ) SHOW_RESULT( chdir, err );
    checkcwd( filename1 );

    NEW_FILENAME1( XXXX "/" YYYY );    
    diag_printf("<INFO>: cd ../../" YYYY "\n");
    err = chdir( "../../" YYYY );
    if( err < 0 ) SHOW_RESULT( chdir, err );
    checkcwd( filename1 );

    diag_printf("<INFO>: cd ../..\n");
    err = chdir( "../.." );
    if( err < 0 ) SHOW_RESULT( chdir, err );
    checkcwd( dirname );

    diag_printf("<INFO>: rmdir " XXXX "/" YYYY "/" ZZZZ "/" WWWW "\n"); 
    err = rmdir( XXXX "/" YYYY "/" ZZZZ "/" WWWW );
    if( err < 0 ) SHOW_RESULT( rmdir, err );

    diag_printf("<INFO>: rmdir "XXXX "/" YYYY "/" ZZZZ "\n"); 
    err = rmdir( XXXX "/" YYYY "/" ZZZZ );
    if( err < 0 ) SHOW_RESULT( rmdir, err );

    diag_printf("<INFO>: rmdir " XXXX "/" YYYY "\n"); 
    err = rmdir( XXXX "/" YYYY );
    if( err < 0 ) SHOW_RESULT( rmdir, err );

    diag_printf("<INFO>: rmdir "XXXX "\n"); 
    err = rmdir( XXXX );
    if( err < 0 ) SHOW_RESULT( rmdir, err );
    
    // --------------------------------------------------------------

    checkcwd( dirname );

#ifdef CLEANUP    
    diag_printf("<INFO>: unlink "TINKY "\n");    
    err = unlink( TINKY );
    if( err < 0 ) SHOW_RESULT( unlink, err );

    diag_printf("<INFO>: unlink " LAALAA" \n");    
    err = unlink( LAALAA );
    if( err < 0 ) SHOW_RESULT( unlink, err );
#endif
    
    // --------------------------------------------------------------
    
    diag_printf("<INFO>: cd %s\n", TEST_MOUNTPOINT TEST_DIRECTORY);
    err = chdir( TEST_MOUNTPOINT TEST_DIRECTORY);
    if( err < 0 ) SHOW_RESULT( chdir, err );
    checkcwd( TEST_MOUNTPOINT TEST_DIRECTORY );
    
    listdir( ".", true, -1, NULL );

#ifdef TEST_MOUNTPOINT2
    if( mp2 )
    {
        diag_printf("<INFO>: umount %s\n",TEST_MOUNTPOINT2);    
        err = umount( TEST_MOUNTPOINT2 );
        if( err < 0 ) SHOW_RESULT( umount, err );
    }
#endif

    // --------------------------------------------------------------

#if 0
    maxfile("file.max");

    listdir( "/", true, -1, NULL );    
        
    diag_printf("<INFO>: unlink file.max\n");    
    err = unlink( "file.max" );
    if( err < 0 ) SHOW_RESULT( unlink, err );    
#endif

    // --------------------------------------------------------------

    diag_printf("<INFO>: cd /\n");    
    err = chdir( "/" );
    if( err == 0 )
    {
        checkcwd( "/" );
    
        diag_printf("<INFO>: umount %s\n",TEST_MOUNTPOINT);    
        err = umount( TEST_MOUNTPOINT );
        if( err < 0 ) SHOW_RESULT( umount, err );
    }

    diag_printf("<INFO>: syncing\n");    
    err = cyg_fs_setinfo(TEST_MOUNTPOINT, FS_INFO_SYNC, 0, 0);
    if (err != 0) SHOW_RESULT(sync, err);
    
    CYG_TEST_PASS_FINISH("yaffs4");
}

// -------------------------------------------------------------------------

#include <cyg/kernel/kapi.h>

static cyg_handle_t thread_handle;
static cyg_thread thread;
static char stack[CYGNUM_HAL_STACK_SIZE_TYPICAL+1024*4];

externC void
cyg_start( void )
{
    cyg_thread_create(3,                // Priority - just a number
                      fileio4_main,     // entry
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
// EOF yaffs4.c
