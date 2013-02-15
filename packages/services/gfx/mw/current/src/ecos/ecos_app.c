/* 
 * Written 1999-03-19 by Jonathan Larmour, Cygnus Solutions
 * Modifed for touchscreen testing by Richard Panton 13-09-00
 * This file is in the public domain and may be used for any purpose
 */

/* CONFIGURATION CHECKS */

#include <pkgconf/system.h>     /* which packages are enabled/disabled */
#ifdef CYGPKG_KERNEL
# include <pkgconf/kernel.h>
#endif
#ifdef CYGPKG_LIBC
# include <pkgconf/libc.h>
#endif
#ifdef CYGPKG_IO_SERIAL
# include <pkgconf/io_serial.h>
#endif

#ifndef CYGFUN_KERNEL_API_C
# error Kernel API must be enabled to build this application
#endif

#ifndef CYGPKG_LIBC_STDIO
# error C library standard I/O must be enabled to build this application
#endif

#ifndef CYGPKG_IO_SERIAL_HALDIAG
# error I/O HALDIAG pseudo-device driver must be enabled to build this application
#endif

/* INCLUDES */

#include <stdio.h>                      /* printf */
#include <stdlib.h>                      /* printf */
#include <string.h>                     /* strlen */
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <sys/time.h>
#include <ctype.h>
#include <network.h>
#include <pkgconf/microwindows.h>
#ifdef CYGPKG_FS_ROM
#define USE_ROMDISK
#endif
#ifdef CYGPKG_FS_JFFS2
# include <pkgconf/io_flash.h>
#define USE_JFFS2
#undef  USE_ROMDISK
#endif
#ifdef CYGPKG_IO_DISK
# include <pkgconf/io_disk.h>
#define USE_DISK
#endif

#define STACKSIZE ( 65536 )
#ifdef CYGBLD_MICROWINDOWS_NANOX_DEMOS_NTETRIS 
#define USE_NTETRIS
#endif
#ifdef CYGBLD_MICROWINDOWS_NANOX_DEMOS_WORLD
#define USE_WORLD
#endif
#ifdef CYGBLD_MICROWINDOWS_NANOX_DEMOS_DEMO
#define USE_DEMO
#endif
#ifdef CYGBLD_MICROWINDOWS_FLNX_HELLO
#define USE_FLNX
#endif
extern void ecos_nx_setup(CYG_ADDRWORD data);
extern void nanowm_thread(CYG_ADDRWORD data);
extern void nanox_thread(CYG_ADDRWORD data);
extern void nxkbd_thread(CYG_ADDRWORD data);
#ifdef USE_DEMO
extern void demo_thread(CYG_ADDRWORD data);
#endif
#ifdef USE_FLNX
extern void flnx_thread(CYG_ADDRWORD data);
#endif
#ifdef USE_NXSCRIBBLE
extern void nxscribble_thread(CYG_ADDRWORD data);
#endif
#ifdef USE_LANDMINE
extern void landmine_thread(CYG_ADDRWORD data);
#endif
#ifdef USE_NTETRIS
extern void ntetris_thread(CYG_ADDRWORD data);
#endif
#ifdef USE_FLNX
extern void flnx_thread(CYG_ADDRWORD data);
extern void flnxdemo_thread(CYG_ADDRWORD data);
#endif
#ifdef USE_WORLD
extern void world_thread(CYG_ADDRWORD data);
#endif
static void startup_thread(CYG_ADDRWORD data);

typedef void fun(CYG_ADDRWORD);
struct nx_thread {
    char         *name;
    fun          *entry;
    int          prio;
    cyg_handle_t t;
    cyg_thread   t_obj;
    char         stack[STACKSIZE];
};

struct nx_thread _threads[] = {
    { "System startup", startup_thread,    11 },
    { "Nano-X server",  nanox_thread,      12 },
    { "Nano-WM",        nanowm_thread,     14 },
    /*{ "Nano-KBD",       nxkbd_thread,      13 },*/
#ifdef USE_NXSCRIBBLE
    { "Scribble",       nxscribble_thread, 20 },
#endif
#ifdef USE_LANDMINE
    { "Landmine",       landmine_thread,   19 },
#endif
#ifdef USE_NTETRIS
    { "Nano-Tetris",    ntetris_thread,    18 },
#endif
#ifdef USE_FLNX
    { "flnx",           flnx_thread,    23 },
    { "flnxdemo",       flnxdemo_thread,    23 },
#endif
#ifdef USE_WORLD
    { "World Map",      world_thread,      21 },
#endif
#ifdef USE_DEMO
    { "demo",            demo_thread,      22 },
#endif
};
#define NUM(x) (sizeof(x)/sizeof(x[0]))

int
strcasecmp(const char *s1, const char *s2)
{
    char c1, c2;

    while ((c1 = tolower(*s1++)) == (c2 = tolower(*s2++)))
        if (c1 == 0)
            return (0);
    return ((unsigned char)c1 - (unsigned char)c2);
}

static void 
startup_thread(CYG_ADDRESS data)
{
    cyg_ucount32 nanox_data_index;
    int i;
    struct nx_thread *nx;

    printf("SYSTEM INITIALIZATION in progress\n");
    printf("NETWORK:\n");
    init_all_network_interfaces();

#ifdef USE_ROMDISK
    {
        char ROM_fs[32];
        int res;

        printf("Mount ROM file system\n");
#if (defined CYGPKG_HAL_ARM_SA11X0_IPAQ) && (!defined CYGBLD_MICROWINDOWS_VNC_DRIVERS)
        // Work around hardware anomaly which causes major screen flicker
        {
            char *hold_rom_fs;
            if ((hold_rom_fs = malloc(0x80080)) != 0) {
                // Note: ROM fs requires 32 byte alignment
                hold_rom_fs = (char *)(((unsigned long)hold_rom_fs + 31) & ~31);
                memcpy(hold_rom_fs, 0x50F00000, 0x80000);
                sprintf(ROM_fs, "0x%08x", hold_rom_fs);
            } else {
                printf("Can't allocate memory to hold ROM fs!\n");
            }
        }
#else
        sprintf(ROM_fs, "0x%08x", 0x50F00000);
        sprintf(ROM_fs, "0x%08x", 0x61F00000);
#endif
        printf("ROM fs at %s\n", ROM_fs);
        if ((res = mount(ROM_fs, "/", "romfs")) < 0) {
            printf("... failed\n");
        }
        chdir("/");
    }
#endif



#ifdef USE_JFFS2
    {
        int res;
        printf("... Mounting JFFS2 on \"/flash\"\n");
#ifdef CYGPKG_DEVS_FLASH_SYNTH_V2
        res = mount( "/dev/flash/0/", "/flash", "jffs2" );
#else
        res = mount( CYGDAT_IO_FLASH_BLOCK_DEVICE_NAME_1, "/", "jffs2" );
#endif
        if (res < 0) {
            printf("Mount \"/flash\" failed - res: %d\n", res);
        }
        chdir("/");
    }
#endif


#ifdef USE_DISK
    {
        int res;
        printf("... Mounting DISK on \"/\"\n");
#ifdef CYGPKG_DEVS_DISK_ECOSYNTH
        res = mount( "/dev/synthdisk0/", "/", "fatfs" );
#endif
        if (res < 0) {
            printf("Mount \"/\" failed - res: %d\n", res);
        }
        chdir("/");
    }   
#endif

    // Allocate a free thread data slot
    // Note: all MicroWindows/NanoX threads use this slot for NanoX-private
    // data.  That's why there is only one call here.
    nanox_data_index = cyg_thread_new_data_index();
    printf("data index = %d\n", nanox_data_index);

    printf("Creating system threads\n");
    nx = &_threads[1];
    for (i = 1;  i < NUM(_threads);  i++, nx++) {
        cyg_thread_create(nx->prio,
                          nx->entry,
                          (cyg_addrword_t) nanox_data_index,
                          nx->name,
                          (void *)nx->stack, STACKSIZE,
                          &nx->t,
                          &nx->t_obj);
    }

    printf("Starting threads\n");
    nx = &_threads[1];
    for (i = 1;  i < NUM(_threads);  i++, nx++) {
        printf("Starting %s\n", nx->name);
        cyg_thread_resume(nx->t);
        // Special case - run additional code, specific to this environment
        // only after the server has had a chance to startup
        if (i == 2) {
            ecos_nx_init(nanox_data_index);
        }
    }

    printf("SYSTEM THREADS STARTED!\n");
}

void cyg_user_start(void)
{
    struct nx_thread *nx;

    nx = &_threads[0];
    cyg_thread_create(nx->prio,
                      nx->entry,
                      (cyg_addrword_t) 0,
                      nx->name,
                      (void *)nx->stack, STACKSIZE,
                      &nx->t,
                      &nx->t_obj);
    cyg_thread_resume(nx->t);
}
