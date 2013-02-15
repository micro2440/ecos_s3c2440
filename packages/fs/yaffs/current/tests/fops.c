//=============================================================================
//
//      ops.c
//
//      Filesystem, file and directory operation tests for YAFFS on eCos
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
// Date:        2009-05-11
// Description: A good (read/write) shakedown of the filesystem ops.
//              On the SYNTH device, includes a long test which fills up
//              the filesystem.
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/infra/testcase.h>
#include <pkgconf/system.h>

#if !defined(CYGPKG_LIBC_TIME) || !defined(CYGPKG_ERROR) || !defined(CYGPKG_LIBC_STDIO)

void cyg_user_start(void)
{
    CYG_TEST_NA("Needs CYGPKG_LIBC_TIME, CYGPKG_ERROR and CYGPKG_LIBC_STDIO");
}

#else


#include <cyg/infra/diag.h>
#include <cyg/fileio/fileio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MUST(what) do { CYG_TEST_CHECK((what), #what); } while(0)

#define MUSTOS(what) do {       \
    if (0 != what) {            \
        perror(#what);          \
        CYG_TEST_FAIL(#what);   \
        cyg_test_exit();        \
    }                           \
} while(0)

#define EXPECT(what,val) do { CYG_TEST_CHECK((what) == (val), #what "==" #val); } while(0)

#ifdef CYGPKG_DEVS_NAND_SYNTH
#define DEVICE "synth/0"
#else
#define DEVICE "onboard/0"
#endif
#define MOUNTPOINT "/nand"
#define FSTYPE "yaffs"

MTAB_ENTRY(my_auto, MOUNTPOINT, FSTYPE, DEVICE, 0/*priv set up by mount*/);

void list_dir_contents(const char *dir) 
{
    DIR *d = opendir(dir);
    if (!d) {
        diag_printf("opendir %s: %s (%d)\n", dir, strerror(errno), errno);
        return;
    }
    diag_printf("Directory %s contains:\n", dir);

    struct dirent *de;
    int n = 0;
    while ((de=readdir(d))) {
        diag_printf("\t%s\n", de->d_name);
        ++n;
    }
    diag_printf("(ends)\n");

    // Rewind and try again
    int m = 0;
    rewinddir(d);
    while ((de=readdir(d)))
        ++m;
    if (n != m)
        CYG_TEST_FAIL_EXIT("dir reread check failed - rewinddir broken?");

    closedir(d);
}

#if 0
void MKDIR(const char *dir)
{
    MUSTOS(mkdir(dir,0777));
}
#else
#define MKDIR(d) MUSTOS(mkdir(d,0777))
#endif

#if 0
void RMDIR(const char *dir)
{
    MUSTOS(rmdir(dir));
}
#else
#define RMDIR(d) MUSTOS(rmdir(d))
#endif

#define RENAME(d,d2) MUSTOS(rename(d,d2))
#define UNLINK(p) MUSTOS(unlink(p))
#define LINK(p1,p2) MUSTOS(link(p1,p2))

void CHDIR(const char *dir)
{
    MUSTOS(chdir(dir));
}

void WANTDIR(const char *d)
{
    struct stat st;
    MUSTOS(stat(d,&st));
    EXPECT(S_ISDIR(st.st_mode),1);
}

void DONOTWANT(const char *f)
{
    struct stat st;
    if (stat(f,&st) != -1) {
        diag_printf("Do not want %s, but it is there\n",f);
        CYG_TEST_FAIL_EXIT("Unexpected entity present");
    }
    EXPECT(errno,ENOENT);
}

void RM_RF_work(const char *f, int recurse)
{
    //diag_printf("chdir %s <r=%d>\n",f,recurse);
    MUSTOS(chdir(f));
    DIR *d = opendir(".");
    if (!d) {
        diag_printf("opendir %s: %s\n", f, strerror(errno));
        return;
    }
    //diag_printf("opened dir %s\n",f);
    struct dirent de_stack;
    struct dirent *de_res;
    while (0==readdir_r(d, &de_stack, &de_res) && de_res) {
        struct stat st;
        if (0==strcmp(de_res->d_name,".")) continue;
        if (0==strcmp(de_res->d_name,"..")) continue;
        MUSTOS(stat(de_res->d_name,&st));
        if (S_ISDIR(st.st_mode)) {
            RM_RF_work(de_res->d_name, recurse+1);
            //diag_printf("rmdir %s\n", de_res->d_name);
            if (rmdir(de_res->d_name))
                perror("rmdir");
        } else {
            // diag_printf("unlink %s\n", de_res->d_name);
            MUSTOS(unlink(de_res->d_name));
        }
        // N.B. We have just unlinked a file or deleted a subdir
        // during a live opendir/readdir cycle. What we get from readdir
        // isn't (strictly) well-defined, but we expect to not crash.
        // In fact what we get from YAFFS is probably the best
        // possible behaviour: there's no explosion, and when we
        // next say readdir() we get the next entry!
    }
    //diag_printf("rm -rf closes %s\n",f);
    closedir(d);
    MUSTOS(chdir(".."));
    //diag_printf("chdir ..\n");
}

void RM_RF(const char *p)
{
    //diag_printf("chdir /\n");
    //MUSTOS(chdir(MOUNTPOINT));
    diag_printf("rm -rf %s\n",p);
    RM_RF_work(p,0);
    //diag_printf("rmdir %s\n",p);
    RMDIR(p);
}

void checklen(const char *f, off_t size)
{
    struct stat st;
    MUSTOS(stat(f, &st));
    if (st.st_size != size) {
        diag_printf("!! file %s size by stat is %lu, expect %lu\n", f, st.st_size, size);
        CYG_TEST_FAIL_EXIT("file size readback mismatch");
    }
}

// --------------------------------------------------------------

// Function inspiration borrowed from fileio1.c and just about 
// every other filesystem in eCos... Only this time, we're 
// sufficiently paranoid to want to use arc4 to generate our file data.

#define SHOW_RESULT(fn,res) do {                    \
    diag_printf("%s returned %d, errno %d (%s)\n", #fn, (int)res, errno, strerror(errno));   \
    CYG_TEST_FAIL("Unexpected result from "#fn);    \
    cyg_test_exit();                                \
} while(0)

#define IOSIZE 100

#include "ar4prng.inl"

#define SEEDLEN (sizeof(time_t))
unsigned char seed[SEEDLEN];

ar4ctx prng;

static void reinit_prng(void)
{
    static int seed_inited = 0;

    if (!seed_inited) {
        // This is a bit cruddy, but will introduce sufficient variation to make it interesting.
        *(time_t*)seed = time(0);
        seed_inited=1;
    }

    ar4prng_init(&prng, seed, SEEDLEN);
}

unsigned char buf[IOSIZE], testbuf[IOSIZE];

static void createfile( char *name, size_t size )
{
    int fd;
    ssize_t wrote;
    int err;
    size_t savesize = size;

    diag_printf("<INFO>: create file %s size %zd\n",name,size);

    err = access( name, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );
    
    reinit_prng();

 
    fd = open( name, O_WRONLY|O_CREAT );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    while( size > 0 )
    {
        ssize_t len = size;
        if ( len > IOSIZE ) len = IOSIZE;
        
        ar4prng_many(&prng, buf, len);
        wrote = write( fd, buf, len );
        if( wrote != len ) SHOW_RESULT( write, wrote );        

        size -= wrote;
    }

    err = fsync(fd);
    if (err < 0) SHOW_RESULT(fsync, err);

    struct stat st;
    err = fstat(fd, &st);
    if (err < 0) SHOW_RESULT(fstat, err);
    if (st.st_size != savesize)
        CYG_TEST_FAIL_EXIT("fstat size mismatched");

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

static void checkfile( char *name )
{
    int fd;
    ssize_t done, pos=0, lastread=-1;
    int i;
    int err;

    diag_printf("<INFO>: check file %s\n",name);
    
    reinit_prng();

    err = access( name, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );

    fd = open( name, O_RDONLY );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    for(;;)
    {
        done = read( fd, buf, IOSIZE );
        pos += done;
        if( done < 0 ) SHOW_RESULT( read, done );

        if( done == 0 ) break;
        lastread = done;

        ar4prng_many(&prng, testbuf, done);

        for( i = 0; i < done; i++ )
            if( buf[i] != testbuf[i] )
            {
                diag_printf("buf[%d](%02x) != %02x\n",i,buf[i],testbuf[i]);
                CYG_TEST_FAIL_EXIT("Data read not equal to data written");
            }
    }

    // While we're here, let's have a play with lseek.
#define LSEEK(f, o, w, expect) do {             \
    err = lseek(f, o, w);                       \
    if (err != expect) SHOW_RESULT(fseek, err); \
} while(0)

    LSEEK(fd, -lastread, SEEK_END, pos-lastread);
    done = read(fd, buf, IOSIZE);
    CYG_ASSERT(done == lastread, "reread last chunk size mismatch - lseek broken?");
    for (i=0; i<IOSIZE; i++) {
        if (buf[i] != testbuf[i]) {
            diag_printf("buf[%d](%02x) != %02x\n",i,buf[i],testbuf[i]);
            CYG_TEST_FAIL_EXIT("Data mismatch - lseek broken?");
        }
    }

    reinit_prng();
    ar4prng_many(&prng, testbuf, IOSIZE);

    LSEEK(fd, 10, SEEK_SET, 10);
    done = read(fd, &buf[10], IOSIZE-10);
    CYG_ASSERT(done == IOSIZE-10, "reading after fseek");
    LSEEK(fd, -IOSIZE, SEEK_CUR, 0); // i.e. to beginning of file
    done = read(fd, buf, 10);
    CYG_ASSERT(done == 10, "reading after 2nd fseek");
    for (i=0; i<IOSIZE; i++) {
        if (buf[i] != testbuf[i]) {
            diag_printf("buf[%d](%02x) != %02x\n",i,buf[i],testbuf[i]);
            CYG_TEST_FAIL_EXIT("Data mismatch - lseek broken?");
        }
    }

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

static void copyfile( char *name2, char *name1 )
{
    int err;
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

static void comparefiles( char *name2, char *name1 )
{
    int err;
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
        done1 = read( fd1, buf, IOSIZE );
        if( done1 < 0 ) SHOW_RESULT( read, done1 );

        done2 = read( fd2, testbuf, IOSIZE );
        if( done2 < 0 ) SHOW_RESULT( read, done2 );

        if( done1 != done2 )
            diag_printf("Files different sizes\n");
        
        if( done1 == 0 ) break;

        for( i = 0; i < done1; i++ )
            if( buf[i] != testbuf[i] )
            {
                diag_printf("buf[%d](%02x) != buf[%d](%02x)\n",i,buf[i],i,testbuf[i]);
                CYG_TEST_FAIL("Data in files not equal\n");
            }
    }

    err = close( fd1 );
    if( err < 0 ) SHOW_RESULT( close, err );

    err = close( fd2 );
    if( err < 0 ) SHOW_RESULT( close, err );
    
}

#ifdef CYGPKG_DEVS_NAND_SYNTH

#include <pkgconf/devs_nand_synth.h>

static void maxfile( char *name )
{
    int fd;
    ssize_t wrote;
    int err;
    size_t size = 0;
    
    diag_printf("<INFO>: create maximal file %s\n",name);

    err = access( name, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );

    reinit_prng();

    fd = open( name, O_WRONLY|O_CREAT );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    do
    {
        ar4prng_many(&prng, buf, IOSIZE);
        wrote = write( fd, buf, IOSIZE );
        if( wrote < 0 ) SHOW_RESULT( write, wrote );        

        CYG_ASSERT(size <= (CYGNUM_NAND_SYNTH_CHIPSIZE*1024), "filesystem overfull");
        size += wrote;
        
    } while( wrote == IOSIZE );

    diag_printf("<INFO>: file size == %zd\n",size);

    err = close( fd );
    cyg_fs_setinfo(MOUNTPOINT, FS_INFO_SYNC, NULL, 0); // sync();
    if( err < 0 ) SHOW_RESULT( close, err );
}
#endif // CYGPKG_DEVS_NAND_SYNTH

// --------------------------------------------------------------

void cyg_user_start(void)
{
    CYG_TEST_INIT();

    // 1. Mounting and unmounting. We should already be mounted
    // by the time we get here (MTAB_ENTRY).
    struct stat st;
    MUSTOS(stat(MOUNTPOINT, &st));
    diag_printf("got mountpoint ino %08x\n",st.st_ino);

    EXPECT(mount(DEVICE, MOUNTPOINT, FSTYPE), -1);
    MUSTOS(umount(MOUNTPOINT));
    EXPECT(umount(MOUNTPOINT),-1);
    MUSTOS(mount(DEVICE, MOUNTPOINT, FSTYPE));
    EXPECT(mount(DEVICE, MOUNTPOINT, FSTYPE), -1);
    CYG_TEST_INFO("mount, umount OK");

    // 2. Directory creation, deletion and rename
    // In fact we start by deleting if our test dir was already there...
#define DIR1 MOUNTPOINT"/testdir"
#define DIR2 MOUNTPOINT"/testdir2"
    if (0==stat(DIR1, &st)) {
        CYG_TEST_INFO("dir " DIR1 " already present - attempting to nuke");
        RM_RF(DIR1);
    }
    if (0==stat(DIR2, &st)) {
        CYG_TEST_INFO("dir " DIR2 " already present - attempting to nuke");
        RM_RF(DIR2);
    }
    EXPECT(rmdir(DIR1),-1);
    MKDIR(DIR1);
    RMDIR(DIR1);
    EXPECT(rmdir(DIR1),-1);
    MKDIR(DIR1);
    EXPECT(mkdir(DIR1,0777),-1);

    MKDIR(DIR1"/foo////");
    EXPECT(rmdir(DIR1),-1);
    EXPECT(rmdir(DIR1"/."),-1);
    EXPECT(rmdir(DIR1"/foo/.."),-1);
    EXPECT(rmdir(DIR1"/./foo/."),-1); // Linux doesn't allow this either.
    RMDIR(DIR1"/./foo/"); // Trailing slash on a dir op shouldn't catch us out

    RENAME(DIR1"/", MOUNTPOINT"/newdir/");
    WANTDIR(MOUNTPOINT"/newdir");
    WANTDIR(MOUNTPOINT"/newdir/////");
    DONOTWANT(DIR1);
    RMDIR(MOUNTPOINT"/newdir");

    EXPECT(rmdir(MOUNTPOINT"/"),-1);
    EXPECT(rmdir(MOUNTPOINT),-1);

    CYG_TEST_INFO("directory creation, deletion OK");

    // 3. chdir. We'll make a simple hierarchy:
    //      DIR1
    //      DIR1/1
    //      DIR1/1a
    //      DIR1/foo/1foo
    //      DIR2
    //      DIR2/2
    //      DIR2/bar/2bar
    //      DIR2/bar/2baz
    // .. then chdir into each in turn and confirm that the contents are as expected.
    MKDIR(DIR1);
    MKDIR(DIR1"/1");
    MKDIR(DIR1"/1a");
    MKDIR(DIR1"/foo");
    MKDIR(DIR1"/foo/1foo");
    MKDIR(DIR2);
    MKDIR(DIR2"/2");
    MKDIR(DIR2"/bar");
    MKDIR(DIR2"/bar/2bar");
    MKDIR(DIR2"/bar/2baz");

    CHDIR(DIR1);
    WANTDIR("1");
    WANTDIR("1a");
    WANTDIR("foo");
    DONOTWANT("2");
    DONOTWANT("1foo");
    DONOTWANT("bar");
    DONOTWANT("2bar");
    DONOTWANT("2baz");

    CHDIR("foo");
    WANTDIR("1foo");
    DONOTWANT("1");
    DONOTWANT("1a");
    DONOTWANT("foo");

    CHDIR("..");
    WANTDIR("1");
    WANTDIR("1a");
    WANTDIR("foo");
    DONOTWANT("bar");
    CHDIR(".");
    WANTDIR("1");
    WANTDIR("1a");
    WANTDIR("foo");
    DONOTWANT("bar");
    WANTDIR("foo/1foo");
    RMDIR("foo/1foo");
    DONOTWANT("foo/1foo");

    CYG_TEST_INFO("chdir, opendir, readdir OK");

    // 4. File creation, copying, renaming, deletion - throughout dirs

    CHDIR(DIR1);
    createfile("floop", 202);
    checklen("floop", 202);
    checkfile(DIR1"/floop");
    copyfile("floop", DIR2"/fee");
    CHDIR(DIR2);
    checkfile("fee");
    checklen("fee", 202);
    comparefiles("fee", DIR1"/floop");

    copyfile("fee", DIR1"/fum");
    checkfile(DIR1"/fum");
    chdir(DIR1);
    checkfile("fum");
    comparefiles("floop", "fum");
    comparefiles("fum", DIR2"/fee");

    // Rename ...
    RENAME("fum", DIR2"/fum2");
    DONOTWANT("fum");
    DONOTWANT("fum2");
    DONOTWANT(DIR1"/fum");
    checkfile(DIR2"/fum2");
    CHDIR(DIR2);
    checkfile("fum2");
    CHDIR(DIR1);
    RENAME(DIR2"/fum2", "fum3");
    checkfile("fum3");
    checkfile(DIR1"/fum3");
    DONOTWANT(DIR2"/fum3");
    comparefiles(DIR1"/fum3", DIR1"/floop");
    CHDIR(DIR2);
    comparefiles(DIR1"/fum3", DIR1"/floop");

    // A brief diversion to check rewinddir...
    list_dir_contents(DIR1);

    // Create-over, rename-over
    CHDIR(DIR1);
    checklen("floop", 202);
    checklen(DIR2"/fee", 202);
    createfile(DIR2"/fee", 4242);
    checklen(DIR2"/fee", 4242);
    checklen("floop", 202);
    RENAME(DIR2"/fee", "floop");
    checklen("floop", 4242);

    // Unlink
    CHDIR(DIR2);
    UNLINK(DIR1"/fum3");
    DONOTWANT(DIR1"/fum3");
    DONOTWANT("fum3");
    CHDIR(DIR1);
    DONOTWANT("fum3");

    CYG_TEST_INFO("file create/read/write, copy, delete OK");

    // 5. File hard links (and directory, which aren't supported)

    LINK("floop","floop2");
    checklen("floop2", 4242);
    LINK("floop",DIR2"/floop3");
    CHDIR(DIR2);
    LINK(DIR1"/floop", "floop4");
    LINK(DIR1"/floop", DIR1"/floop5");
    DONOTWANT("floop2");
    checkfile("floop3");
    checkfile("floop4");
    DONOTWANT("floop5");
    CHDIR(DIR1);
    checkfile("floop2");
    DONOTWANT("floop3");
    DONOTWANT("floop4");
    checkfile("floop5");

    checkfile(DIR1"/floop2");
    checkfile(DIR2"/floop3");
    checkfile(DIR2"/floop4");
    checkfile(DIR1"/floop5");

    UNLINK("floop2");
    UNLINK(DIR2"/floop3");
    CHDIR(DIR2);
    UNLINK("floop4");
    UNLINK(DIR1"/floop5");

    DONOTWANT(DIR1"/floop2");
    DONOTWANT(DIR2"/floop3");
    DONOTWANT(DIR2"/floop4");
    DONOTWANT(DIR1"/floop5");

    CHDIR(DIR1);
    checkfile("floop");
    // we'll leave floop lying around for fpathconf() test

    EXPECT(link(DIR1, DIR2"/dir-hardlink-should-fail"), -1);
    EXPECT(errno, EPERM);

    CYG_TEST_INFO("hard links OK");

#ifdef CYGPKG_DEVS_NAND_SYNTH
    // 6. EOF-on-write (by filling the filesystem!)
    CYG_TEST_INFO("bigfile test, this might take a few seconds");

    // To speed this test up tune your synth nand device to not be so big.
    maxfile(DIR1"/bigfile");
    checkfile(DIR1"/bigfile");
    {
        struct stat st;
        stat(DIR1"/bigfile", &st);
    }
    UNLINK(DIR1"/bigfile");
    CYG_TEST_INFO("EOF on write OK");
#else
    CYG_TEST_INFO("SKIPPING EOF-on-write test (only applied to synth nand)");
#endif

    // 7. Ancillary checks - getinfo, setinfo (no ioctls supported as yet)

#ifdef CYGSEM_FILEIO_INFO_DISK_USAGE
    struct cyg_fs_disk_usage usage;
    MUSTOS(cyg_fs_getinfo(MOUNTPOINT, FS_INFO_DISK_USAGE, &usage, sizeof(usage)));
    diag_printf("getinfo/disk_usage says:\n\ttotal blocks %llu\n\tfree blocks  %llu\n\tblock size   %u\n", usage.total_blocks, usage.free_blocks, usage.block_size);
#endif

    diag_printf("pathconf says:\n\tNAME_MAX %ld\n\tPATH_MAX %ld\n\tSYNC_IO %ld\n", 
            pathconf(DIR1, _PC_NAME_MAX),
            pathconf(DIR1, _PC_PATH_MAX),
            pathconf(DIR1, _PC_SYNC_IO));

    MUSTOS(cyg_fs_setinfo(MOUNTPOINT, FS_INFO_SYNC, 0, 0));

    int fd = open(DIR1"/floop", O_RDONLY);
    CYG_ASSERT(fd >= 0, "open");
    CYG_ASSERT(fpathconf(fd, _PC_NAME_MAX) != -1, "fpathconf PC_NAME_MAX");
    close(fd);

    CYG_TEST_INFO("getinfo, setinfo OK");

    // Clean up.

    CHDIR(MOUNTPOINT);
    RM_RF(DIR1);
    RM_RF(DIR2);
    DONOTWANT(DIR1);
    DONOTWANT(DIR2);
    CYG_TEST_INFO("recursive deletion OK");

    MUSTOS(umount(MOUNTPOINT));
#ifndef CYGDBG_USE_ASSERTS
    CYG_TEST_INFO("WARNING: asserts disabled, test INCOMPLETE, try switching on CYGPKG_INFRA_DEBUG");
#else
    CYG_TEST_PASS_FINISH("All OK");
#endif
}

#endif

