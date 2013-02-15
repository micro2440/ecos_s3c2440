//=============================================================================
//
//      nand_synth.c
//
//      Synthetic NAND flash driver
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
// Author(s):   wry, bartv
// Date:        2009-03-06
// Note:        This file borrows in places from the synthetic flash driver.
//
//####DESCRIPTIONEND####
//=============================================================================

#include <cyg/nand/nand_device.h>
#include <cyg/nand/nand.h>
#include CYGBLD_ISO_ERRNO_CODES_HEADER
#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_endian.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/cyg_ass.h>
#include <pkgconf/io_nand.h>
#include <pkgconf/devs_nand_synth.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

// ----------------------------------------------------------------------------
// Multiple devices can be supported in future by #include'ing this file
// from several different modules (possibly generated) e.g. nand_synth_dev0.c,
// nand_synth_dev1.c, etc. This makes it simpler to have separate logging
// for each device etc. Everything is static so there are no name clashes,
// but debugging the driver via gdb becomes tricky when multiple devices
// are configured.
//
// These #define's would be provided by nand_synth_dev0.c etc.

#define DEVID               0
#define IMAGE_FILENAME      CYGDAT_NAND_SYNTH_FILENAME
#define PAGESIZE            CYGNUM_NAND_SYNTH_PAGESIZE
#define SPARE_PER_PAGE      CYGNUM_NAND_SYNTH_SPARE_PER_PAGE
#define PAGES_PER_BLOCK     CYGNUM_NAND_SYNTH_PAGES_PER_BLOCK
#define BLOCK_COUNT         CYGNUM_NAND_SYNTH_BLOCK_COUNT

// ----------------------------------------------------------------------------
// Statics.

// A driver lock to control access to this device.
static cyg_drv_mutex_t  lock;

// A timestamp for when the test run started. This ends up in both the
// nand image and the logfile, so that images, logfiles and checkpoint
// files can be matched up.
static struct cyg_hal_sys_timeval       timeval;

// The current image file.
static int              image_fd    = -1;
static int*             image_header;
static int*             image_erase_counts;
static int*             image_write_counts;
static int*             image_factory_bads;
static unsigned char*   image_ok_blocks;
static unsigned char*   image_data;

// Various size fields.
#define HEADER_SIZE     64
#define DATA_SIZE       (BLOCK_COUNT * PAGES_PER_BLOCK * (PAGESIZE + SPARE_PER_PAGE))
#define IMAGE_SIZE      (HEADER_SIZE +                                      \
                         (BLOCK_COUNT * sizeof(int)) +                      \
                         (BLOCK_COUNT * PAGES_PER_BLOCK * sizeof(int)) +    \
                         (MAX_FACTORY_BAD * sizeof(int)) +                  \
                         (BLOCK_COUNT / 8) + DATA_SIZE)

// For interacting with the I/O auxiliary. synth_buf can be used for
// other things.
static int              synth_id    = -1;
#define SYNTH_BUFSIZE   1024
static unsigned char    synth_buf[SYNTH_BUFSIZE];

// The number of various calls made so far. This is used for both
// logging and for triggering new bad blocks.
static int      number_of_calls             = 0;
static int      number_of_read_calls        = 0;
static int      number_of_write_calls       = 0;
static int      number_of_erase_calls       = 0;
static int      number_of_factory_bad_calls = 0;
static int      number_of_errors_injected   = 0;

// The different types of event that can be logged.
static int      log_read                = 0;
static int      log_READ                = 0;
static int      log_write               = 0;
static int      log_WRITE               = 0;
static int      log_erase               = 0;
static int      log_error               = 0;
// The logical or of all the above.
static int      logging_enabled         = 0;

// Allow for a .checkpoint.0000 suffix.
#define MAX_LOGFILE_NAME (1024 - 32)
static char     logfile_name[MAX_LOGFILE_NAME];
static int      max_logfile_size        = 0;
static int      number_of_logfiles      = 0;
static int      generate_checkpoints    = 0;

// Another static, buffer to hold the current log entry. This needs to be
// (two bytes * PAGESIZE) to allow for READ and WRITE logging, plus some
// spare for the header.
static char     logfile_data[SYNTH_BUFSIZE + (2 * PAGESIZE)];

// Support for up to 32 factory-bad blocks. Changing the limit affects the
// image file format, so don't.
#define MAX_FACTORY_BAD 32
static int      factory_bad_blocks[MAX_FACTORY_BAD] = { -1 } ;

// Maximum number of bad block injections. The limit is
// imposed primarily for the sake of the GUI interface which allows
// for 8 erase and 8 program definitions. Because of the repeat option
// it is still possible to generate any number of bad blocks.
#define MAX_BAD_BLOCK_INJECTIONS  16

// Data structure used for run-time bad block injections
typedef struct bad_block_injection {
    int                         enabled;
    enum { ERASE, WRITE }       type;
    enum { CURRENT, NUMBER }    affects;        // The failure affects the current block/page in the operation, or for a specific block.
    int                         number;         // of block or page, ignored for type==CURRENT
    enum { RAND,SET }           after_type;     // The failure occurs after a random number of operations, or after a set number
    int                         after_count;    // The number of operations, or the max for RAND
    enum { ERASES,
           WRITES,
           CALLS,
           BLOCK_ERASES,
           PAGE_WRITES }        after_which;    // The failure occurs after n per-block/per-page operations, or after n calls into the driver,
                                            // or after n erase/write calls.
    int                         repeat;         // If CURRENT, fail another block after n operations

    int                         actual_count;   // after_count or rand() % after_count.
                                            // compared with number_of_calls/number_of_erase_calls/number_of_write_calls
                                            // or decrements to 0 for BLOCK_ERASES and PAGE_WRITES
    int                         pending;
} bad_block_injection;

static bad_block_injection  injections[MAX_BAD_BLOCK_INJECTIONS];

// Used as a heuristic for suppressing unwanted diagnostics.
static int                  last_block_gone_bad = -1;

// The bad block table needed by the generic NAND layer.
// 2 bits per block. This is separate from the bad block table
// held in the image, which is what gets used by the driver.
#ifdef CYGSEM_IO_NAND_USE_BBT
static unsigned char bbtable[BLOCK_COUNT / 4];
#endif

// ----------------------------------------------------------------------------
// Interaction with the I/O auxiliary and its nand.tcl script.
//
// First the protocol. This must be kept in step with nand.tcl
//
// Protocol version
#define NAND_PROTOCOL_VERSION           0x01
// Inform the host-side of the parameters.
#define NAND_SET_PARAMS                 0x01
// And of the partition table
#define NAND_SET_PARTITIONS             0x02
// Pop up a dialog, iff running in GUI mode
#define NAND_DIALOG                     0x03
// Retrieve the logfile settings
#define NAND_GET_LOGFILE_SETTINGS       0x04
// Retrieve the factory-bad blocks
#define NAND_GET_FACTORY_BADS           0x05
// And retrieve the bad block injection details
#define NAND_GET_BAD_BLOCK_INJECTIONS   0x06

static void
nand_synth_init(void)
{
    int bufsize;
    diag_sprintf((char*)synth_buf, "%d", DEVID);
    synth_id    = synth_auxiliary_instantiate("devs/nand/synth",
                                              SYNTH_MAKESTRING(CYGPKG_DEVS_NAND_SYNTH),
                                              "nand",
                                              (const char*)synth_buf,
                                              NULL);
    if (-1 == synth_id) {
        diag_printf("NAND synth_devinit: warning, failed to instantiate host-side support.\n");
    } else {
        // Protocol first so that the host-side can detect mismatches immediately.
        // Filename last so no need to worry about special characters.
        bufsize = diag_snprintf((char*)synth_buf, SYNTH_BUFSIZE, "%d,%d,%d,%d,%d,%s",
                                NAND_PROTOCOL_VERSION,
                                PAGESIZE,
                                SPARE_PER_PAGE,
                                PAGES_PER_BLOCK,
                                BLOCK_COUNT,
                                IMAGE_FILENAME);
        if (bufsize >= SYNTH_BUFSIZE) {
            diag_printf("NAND synth_devinit: internal error, buffer overflow.\n");
            synth_id = -1;
        } else {
            synth_auxiliary_xchgmsg(synth_id, NAND_SET_PARAMS,
                                    0, 0,
                                    synth_buf, bufsize,
                                    NULL, NULL, NULL, 0);
        }
    }
#ifdef CYGPKG_DEVS_NAND_SYNTH_PARTITION_0
    if (-1 != synth_id) {
        // No need to worry about overflow here with at most four
        // partitions and a 1K buffer
        bufsize = diag_sprintf((char*)synth_buf, "%d,%d",
                               CYGNUM_DEVS_NAND_SYNTH_PARTITION_0_BASE,
                               CYGNUM_DEVS_NAND_SYNTH_PARTITION_0_SIZE);
# ifdef CYGPKG_DEVS_NAND_SYNTH_PARTITION_1
        bufsize += diag_sprintf((char*)synth_buf + bufsize, ",%d,%d",
                                CYGNUM_DEVS_NAND_SYNTH_PARTITION_1_BASE,
                                CYGNUM_DEVS_NAND_SYNTH_PARTITION_1_SIZE);
# endif
# ifdef CYGPKG_DEVS_NAND_SYNTH_PARTITION_2
        bufsize += diag_sprintf((char*)synth_buf + bufsize, ",%d,%d",
                                CYGNUM_DEVS_NAND_SYNTH_PARTITION_2_BASE,
                                CYGNUM_DEVS_NAND_SYNTH_PARTITION_2_SIZE);
# endif
# ifdef CYGPKG_DEVS_NAND_SYNTH_PARTITION_3
        bufsize += diag_sprintf((char*)synth_buf + bufsize, ",%d,%d",
                                CYGNUM_DEVS_NAND_SYNTH_PARTITION_3_BASE,
                                CYGNUM_DEVS_NAND_SYNTH_PARTITION_3_SIZE);
# endif
        synth_auxiliary_xchgmsg(synth_id, NAND_SET_PARTITIONS,
                                0, 0,
                                synth_buf, bufsize,
                                NULL, NULL, NULL, 0);
    }
#endif  // PARTITION_0

    if (-1 != synth_id) {
        // Now pop up the dialog box if -nanddebug was set, and
        // wait for completion.
        int response;
        synth_auxiliary_xchgmsg(synth_id, NAND_DIALOG,
                                0, 0, NULL, 0,
                                &response,
                                NULL, NULL, 0);
    }

    if (-1 != synth_id) {
        // Retrieve the log information.
        int response, len;
        len = SYNTH_BUFSIZE - 1;
        synth_auxiliary_xchgmsg(synth_id, NAND_GET_LOGFILE_SETTINGS,
                                SYNTH_BUFSIZE - 1, 0,
                                NULL, 0,
                                &response,
                                synth_buf, &len, SYNTH_BUFSIZE - 1);
        if (! response) {
            diag_printf("NAND synth_devinit: failed to retrieve logfile settings, logging disabled.\n");
        } else {
            unsigned char*  tmp;
            int             ok      = 1;
            synth_buf[len]          = '\0';
            log_read                = (synth_buf[0] == '1');
            log_READ                = (synth_buf[1] == '1');
            log_write               = (synth_buf[2] == '1');
            log_WRITE               = (synth_buf[3] == '1');
            log_erase               = (synth_buf[4] == '1');
            log_error               = (synth_buf[5] == '1');
            generate_checkpoints    = (synth_buf[6] == '1');
            tmp = &(synth_buf[7]);
            while (ok && (*tmp != '\0') && (*tmp != ',')) {
                if ((*tmp < '0') || (*tmp > '9')) {
                    diag_printf("NAND synth_devinit: invalid response to GET_LOGFILE_SETTINGS\n    Logging disabled.");
                    ok  = 0;
                    break;
                }
                max_logfile_size    = (10 * max_logfile_size) + (*tmp++ - '0');
            }
            if (ok && (*tmp != ',')) {
                diag_printf("NAND synth_devinit: invalid response to GET_LOGFILE_SETTINGS\n    Logging disabled.");
                ok = 0;
            } else {
                tmp++;
            }
            while (ok && (*tmp != '\0') && (*tmp != ',')) {
                if ((*tmp < '0') || (*tmp > '9')) {
                    diag_printf("NAND synth_devinit: invalid response to GET_LOGFILE_SETTINGS\n    Logging disabled.");
                    ok  = 0;
                    break;
                }
                number_of_logfiles  = (10 * number_of_logfiles) + (*tmp++ - '0');
            }
            if (ok && (*tmp != ',')) {
                diag_printf("NAND synth_devinit: invalid response to GET_LOGFILE_SETTINGS\n    Logging disabled.");
                ok = 0;
            } else {
                tmp++;
            }
            if (ok) {
                if (strlen((char*)tmp) >= MAX_LOGFILE_NAME) {
                    diag_printf("NAND synth_devinit: invalid response to GET_LOGFILE_SETTINGS\n    Logging disabled.");
                    ok = 0;
                } else {
                    strcpy(logfile_name, (char*)tmp);
                }
            }
            if (ok) {
                logging_enabled     = log_read || log_READ || log_write || log_WRITE || log_erase || log_error;
            }
#if 0
            diag_printf("logging enabled %d\n", logging_enabled);
            diag_printf("log_read %d, log_READ %d, log_write %d, log_WRITE %d\n", log_read, log_READ, log_write, log_WRITE);
            diag_printf("log_erase %d, log_error %d\n", log_erase, log_error);
            diag_printf("logfile %s\n", logfile_name);
            diag_printf("max logfile size %d\n", max_logfile_size);
            diag_printf("number of logfiles %d\n", number_of_logfiles);
            diag_printf("generate checkpoints %d\n", generate_checkpoints);
#endif            
        }
    }

    if (-1 != synth_id) {
        // Retrieve the bad block information. This will only be used when a new
        // nand image is created.
        int             i;
        unsigned char*  str;
        int             response, len;
        int             ok = 1;

        len = SYNTH_BUFSIZE - 1;
        synth_auxiliary_xchgmsg(synth_id, NAND_GET_FACTORY_BADS,
                                SYNTH_BUFSIZE - 1, 0,
                                NULL, 0,
                                &response,
                                synth_buf, &len, len);
        if (!response) {
            diag_printf("NAND synth_devinit: failed to retrieve factory bad block settings.\n"
                        "  If a new nand image is created it will not have any factory-bad blocks.\n");
        } else {
            str         = synth_buf;
            str[len]    = '\0';
            for (i = 0; i < MAX_FACTORY_BAD; i++) {
                factory_bad_blocks[i] = 0;
                while (('0' <= *str) && (*str <= '9')) {
                    factory_bad_blocks[i] = (10 * factory_bad_blocks[i]) + (*str++ - '0');
                }
                if ('\0' == *str) {
                    break;
                }
                if (',' != *str) {
                    diag_printf("NAND synth_devinit: invalid factory bad block response from I/O auxiliary\n"
                                "  \"s\"\n"
                                "  Unexpected character '%c'\n"
                                "  Ignoring rest of string.\n",
                                *str);
                    ok = 0;
                    break;
                }
                str++;
            }
            for ( i++ ; i < MAX_FACTORY_BAD; i++) {
                factory_bad_blocks[i] = -1;
            }
            if (ok && ('\0' != *str)) {
                diag_printf("NAND synth_devinit: invalid factory bad block response from I/O auxiliary.\n"
                            "  Too many bad blocks in response string.\n"
                            "  Using only the first %d entries.\n", MAX_FACTORY_BAD);
            }
#if 0
            if (factory_bad_blocks[0] == -1) {
                diag_printf("NAND synth_devinit: no factory bad blocks defined.\n");
            } else {
                diag_printf("NAND synth_devinit: factory bad blocks are ");
                for (i = 0; (i < MAX_FACTORY_BAD) && (factory_bad_blocks[i] != -1); i++) {
                    diag_printf("%d ", factory_bad_blocks[i]);
                }
                diag_printf("\n");
            }
#endif
        }
    }

    if (-1 != synth_id) {
        // Retrieve the bad block injection information.
        int             i;
        int             ok = 1;
        unsigned char*  str;
        int             response, len;

        len = SYNTH_BUFSIZE - 1;
        synth_auxiliary_xchgmsg(synth_id, NAND_GET_BAD_BLOCK_INJECTIONS,
                                SYNTH_BUFSIZE - 1, 0,
                                NULL, 0,
                                &response,
                                synth_buf, &len, len);
        if (!response) {
            diag_printf("NAND synth_devinit: failed to retrieve bad block injection settings.\n"
                        "  No run-time block failures will occur.\n");
        } else {
            synth_buf[len]  = '\0';
            str = synth_buf;
            for (i = 0; i < MAX_BAD_BLOCK_INJECTIONS; i++) {
                if (*str == '\0') {
                    break;
                }
                if (*str == 'E') {
                    injections[i].type = ERASE;
                } else if (*str == 'W') {
                    injections[i].type = WRITE;
                } else {
                    ok = 0;
                    break;
                }
                str++;
                if (*str == 'c') {
                    injections[i].affects   = CURRENT;
                } else if (*str == 'n') {
                    injections[i].affects   = NUMBER;
                } else {
                    ok = 0;
                    break;
                }
                str++;
                injections[i].number = 0;
                while (('0' <= *str) && (*str <= '9')) {
                    injections[i].number = (10 * injections[i].number) + (*str++ - '0');
                }
                if (*str == 'r') {
                    injections[i].after_type    = RAND;
                } else if (*str == 'e') {
                    injections[i].after_type    = SET;
                } else {
                    ok = 0;
                    break;
                }
                str++;
                injections[i].after_count    = 0;
                while (('0' <= *str) && (*str <= '9')) {
                    injections[i].after_count = (10 * injections[i].after_count) + (*str++ - '0');
                }
                if (*str == 'E') {
                    injections[i].after_which   = ERASES;
                } else if (*str == 'W') {
                    injections[i].after_which   = WRITES;
                } else if (*str == 'C') {
                    injections[i].after_which   = CALLS;
                } else if (*str == 'e') {
                    injections[i].after_which   = BLOCK_ERASES;
                } else if (*str == 'w') {
                    injections[i].after_which   = PAGE_WRITES;
                } else {
                    ok = 0;
                    break;
                }
                str++;
                if (*str == 'o') {
                    injections[i].repeat  = 0;
                } else if (*str == 'r') {
                    injections[i].repeat  = 1;
                } else {
                    ok = 0;
                    break;
                }
                str++;

                // This failure definition is ok.
                injections[i].enabled = 1;
                injections[i].pending = 0;
                if (RAND == injections[i].after_type) {
                    injections[i].actual_count = rand() % injections[i].after_count;
                } else {
                    injections[i].actual_count = injections[i].after_count;
                }
            }
            for ( ; i < MAX_BAD_BLOCK_INJECTIONS; i++) {
                injections[i].enabled = 0;
            }
            if (! ok) {
                diag_printf("NAND synth_devinit: invalid response from I/O auxiliary to GET_FAILURE_SETTINGS request.\n"
                            "  Reply string was \"%s\"\n"
                            "  Disabling run-time block failures.\n",
                            synth_buf);
                for (i = 0; i < MAX_BAD_BLOCK_INJECTIONS; i++) {
                    injections[i].enabled = 0;
                }
            }

#if 0
            if (!injections[0].enabled) {
                diag_printf("NAND synth_devinit: no bad block injections.\n");
            } else {
                diag_printf("NAND synth_devinit: bad block injections:\n");
                for (i = 0; i < MAX_BAD_BLOCK_INJECTIONS; i++) {
                    if (!injections[i].enabled) {
                        continue;
                    }
                    diag_printf("  %s %s ",
                                ((injections[i].type == ERASE) ? "Erase" : "Write"),
                                ((injections[i].affects == CURRENT) ? "current" :
                                 ((injections[i].type == ERASE) ? "block" : "page")));
                    if (injections[i].affects != CURRENT) {
                        diag_printf("%d ", injections[i].number);
                    }
                    diag_printf("after %s %d %s%s\n",
                                ((injections[i].after_type == RAND) ? "rand% " : ""),
                                injections[i].after_count,
                                ((injections[i].after_which == ERASES) ?         "erases" :
                                 (injections[i].after_which == WRITES) ?         "writes" :
                                 (injections[i].after_which == CALLS) ?          "calls"  :
                                 (injections[i].after_which == BLOCK_ERASES) ?   "block erases" : "page writes"),
                                (injections[i].repeat ? " repeat" : ""));
                }
            }
#endif
        }
    }
    
}

// ----------------------------------------------------------------------------
// Logging support. Some of this is also used for errors.

static int  logfile_fd              = -1;
static int  logfile_size            = 0;
static int  oldest_logfile_suffix   = -1;
static int  next_logfile_suffix     = 0;

static void
create_checkpoint(void)
{
    int fd;
    int written;
    if ( !logging_enabled || !generate_checkpoints) {
        return;
    }

    diag_sprintf((char*)synth_buf, "%s.checkpoint", logfile_name);
    fd = cyg_hal_sys_open((char*)synth_buf, CYG_HAL_SYS_O_WRONLY | CYG_HAL_SYS_O_CREAT | CYG_HAL_SYS_O_TRUNC,
                          CYG_HAL_SYS_S_IRUSR | CYG_HAL_SYS_S_IWUSR | CYG_HAL_SYS_S_IRGRP | CYG_HAL_SYS_S_IWGRP);
    if (fd < 0) {
        diag_printf("NAND: failed to create checkpoint file %s, errno %d\n    Disabling checkpoints.\n",
                    synth_buf, -fd);
        generate_checkpoints = 0;
        return;
    }
    written = cyg_hal_sys_write(fd, image_header, IMAGE_SIZE);
    cyg_hal_sys_close(fd);
    if (written < 0) {
        diag_printf("NAND: error writing current image to checkpoint file %s, errno %d\n    Disabling checkpoints.\n",
                    synth_buf, -written);
        generate_checkpoints = 0;
    } else if (written != IMAGE_SIZE) {
        diag_printf("NAND: error writing current image to checkpoint file %s\n    Only write %d out of %d bytes\n    Disabling checkpoints.\n",
                    synth_buf, written, IMAGE_SIZE);
        generate_checkpoints = 0;
    }
}

static void
logfile_overflow(void)
{
    int result;
    
    cyg_hal_sys_close(logfile_fd);
    
    if (number_of_logfiles > 1) {
        if (-1 != oldest_logfile_suffix) {
            // We have some number of existing files .0, .1 ... (next_logfile_suffix - 1)
            // plus the current logfile. ((next + 1) - oldest) -> number of logfiles,
            // including the current one.
            if (((next_logfile_suffix + 1) - oldest_logfile_suffix) >= number_of_logfiles) {
                // So time to delete the oldest.
                diag_sprintf((char*)synth_buf, "%s.%d", logfile_name, oldest_logfile_suffix);
                result = cyg_hal_sys_unlink((const char*)synth_buf);
                if (result < 0) {
                    diag_printf("NAND logfile overflow: warning, failed to delete old logfile %s, errno %d\n",
                                (char*)synth_buf, -result);
                }
                if (generate_checkpoints) {
                    diag_sprintf((char*)synth_buf, "%s.checkpoint.%d", logfile_name, oldest_logfile_suffix);
                    result = cyg_hal_sys_unlink((const char*)synth_buf);
                    if (result < 0) {
                        diag_printf("NAND logfile overflow: warning, failed to delete old checkpoint file %s, errno %d\n",
                                    (char*)synth_buf, -result);
                    }
                }
                oldest_logfile_suffix += 1;
            }
        }
        // We now know we won't exceed the number_of_logfiles limit, so rename the current
        // logfile and checkpoint file.
        diag_sprintf((char*)synth_buf, "%s.%d", logfile_name, next_logfile_suffix);
        result = cyg_hal_sys_rename((const char*)logfile_name, (const char*)synth_buf);
        if (result < 0) {
            diag_printf("NAND logfile overflow: warning, failed to rename current logfile %s\n"
                        "    to archive logfile %s, errno %d\n"
                        "    Discarding current logfile contents.\n",
                        logfile_name, synth_buf, -result);
        }
        if (generate_checkpoints) {
            // We need a second buffer, Any logdata has already been written so
            // logfile_data can be reused.
            diag_sprintf((char*)logfile_data, "%s.checkpoint", logfile_name);
            diag_sprintf((char*)synth_buf, "%s.checkpoint.%d", logfile_name, next_logfile_suffix);
            result = cyg_hal_sys_rename((const char*)logfile_data, (const char*)synth_buf);
            if (result < 0) {
                diag_printf("NAND logfile overflow: warning, failed to rename current checkpoint file %s\n"
                            "    to archive checkpoint %s, errno %d\n"
                            "    Discarding current checkpoint data.\n",
                            logfile_data, synth_buf, -result);
            }
        }
        if (-1 == oldest_logfile_suffix) {
            oldest_logfile_suffix = next_logfile_suffix;
        }
        next_logfile_suffix += 1;
    }

    logfile_fd = cyg_hal_sys_open(logfile_name, CYG_HAL_SYS_O_WRONLY | CYG_HAL_SYS_O_CREAT | CYG_HAL_SYS_O_TRUNC,
                                  CYG_HAL_SYS_S_IRUSR | CYG_HAL_SYS_S_IWUSR | CYG_HAL_SYS_S_IRGRP | CYG_HAL_SYS_S_IWGRP);
    if (logfile_fd < 0) {
        diag_printf("NAND logfile overflow: failed to reopen logfile %s\n    Logging has been disabled.\n", logfile_name);
        logging_enabled = 0;
        return;
    }
    logfile_size    = 0;
    create_checkpoint();
}

static void
logfile_write(int len)
{
    int written;
    if ( ! logging_enabled) {
        return;
    }

    CYG_ASSERTC(logfile_fd >= 0);
    written = cyg_hal_sys_write(logfile_fd, logfile_data, len);
    if (written < 0) {
        diag_printf("NAND write_log: error writing to logfile (errno %d).\n    Disabling logging.\n", 0 - written);
        cyg_hal_sys_close(logfile_fd);
        logging_enabled = 0;
        return;
    }
    if (written != len) {
        diag_printf("NAND write_log: error writing to logfile, only wrote %d out of %d bytes.\n    Disabling logging.\n", written, len);
        cyg_hal_sys_close(logfile_fd);
        logging_enabled = 0;
        return;
    }

    logfile_size += len;
    if (logfile_size >= max_logfile_size) {
        logfile_overflow();
    }
}

static void
logfile_open(void)
{
    int len;
    if ( ! logging_enabled) {
        return;
    }

    logfile_fd = cyg_hal_sys_open(logfile_name, CYG_HAL_SYS_O_WRONLY | CYG_HAL_SYS_O_CREAT | CYG_HAL_SYS_O_TRUNC,
                                  CYG_HAL_SYS_S_IRUSR | CYG_HAL_SYS_S_IWUSR | CYG_HAL_SYS_S_IRGRP | CYG_HAL_SYS_S_IWGRP);
    if (logfile_fd < 0) {
        diag_printf("NAND synth_devinit: failed to open logfile %s\n    Logging has been disabled.\n", logfile_name);
        logging_enabled = 0;
        return;
    }

    len = diag_sprintf(logfile_data, "I 0 0 %ld %ld %s %d %d %d %d\n", timeval.hal_tv_sec, timeval.hal_tv_usec,
                       IMAGE_FILENAME, PAGESIZE, SPARE_PER_PAGE, PAGES_PER_BLOCK, BLOCK_COUNT);
    logfile_write(len);
}

static int
addhex(char* dest, unsigned char* data, int len)
{
    static const char hexdigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    int i;

    for (i = 0; i < len; i++) {
        *dest++ = hexdigits[(*data >> 4) & 0x0F];
        *dest++ = hexdigits[ *data++     & 0x0F];
    }
    return len + len;
}

// ----------------------------------------------------------------------------
// Bad block handling.
static void
mark_block_bad(int block)
{
    CYG_ASSERTC((block >= 0) && (block < BLOCK_COUNT));
    image_ok_blocks[block / 8] &= ~(0x01 << (block % 8));
    last_block_gone_bad = block;
}

static int
is_block_ok(int block)
{
    CYG_ASSERTC((block >= 0) && (block < BLOCK_COUNT));
    if (image_ok_blocks[block / 8] & (0x01 << (block % 8))) {
        return 1;
    } else {
        return 0;
    }
}

static int
is_block_bad(int block)
{
    return !is_block_ok(block);
}

static void
mark_page_bad(int page)
{
    CYG_ASSERTC((page >= 0) && (page < (PAGES_PER_BLOCK * BLOCK_COUNT)));
    mark_block_bad(page / PAGES_PER_BLOCK);
}

static int
is_page_ok(int page)
{
    CYG_ASSERTC((page >= 0) && (page < (PAGES_PER_BLOCK * BLOCK_COUNT)));
    return is_block_ok(page / PAGES_PER_BLOCK);
}

static int
is_page_bad(int page)
{
    return !is_page_ok(page);
}

static int
write_check_injections(cyg_nand_page_addr page)
{
    int i;
    int make_bad            = 0;
    int containing_block    = page / PAGES_PER_BLOCK;
    
    for (i = 0; i < MAX_BAD_BLOCK_INJECTIONS; i++) {
        if ( ! injections[i].enabled) {
            continue;
        }

        // First, figure out which injections should become pending.
        if (! injections[i].pending) {
            switch(injections[i].after_which) {
              case ERASES:
                if (number_of_erase_calls >= injections[i].actual_count) {
                    injections[i].pending   = 1;
                }
                break;
              case WRITES:
                if (number_of_write_calls >= injections[i].actual_count) {
                    injections[i].pending   = 1;
                }
                break;
              case CALLS:
                if (number_of_calls >= injections[i].actual_count) {
                    injections[i].pending   = 1;
                }
                break;
              case BLOCK_ERASES:
                // A BLOCK_ERASE counter cannot decrement because of a page write
                break;
              case PAGE_WRITES:
                if ((injections[i].type == ERASE) && (containing_block == injections[i].number)) {
                    injections[i].actual_count -= 1;
                    if (injections[i].actual_count <= 0) {
                        injections[i].pending   = 1;
                    }
                }
                if ((injections[i].type == WRITE) && (page == injections[i].number)) {
                    injections[i].actual_count -= 1;
                    if (injections[i].actual_count <= 0) {
                        injections[i].pending   = 1;
                    }
                }
                break;
            }
        }
        if ((injections[i].type == WRITE) && (injections[i].pending)) {
            if ((injections[i].affects == CURRENT) || (injections[i].number == page)) {
                // This will cause the block to be made bad later on. The flag may
                // be set several times because multiple injection definitions trigger.
                make_bad    = 1;
                // Now, if this injection is not repeatable then we can disable it and
                // it will be ignored from here on.
                if (! injections[i].repeat) {
                    injections[i].enabled   = 0;
                } else {
                    // OK, we need to reset actual_count. Note that this is done relative to the
                    // current number_of_erase_calls etc., not relative to actual_count. Otherwise
                    // the injection is too likely to trigger again during the next call, which will
                    // probably be for the primary bad block table. Using number_of_erase_calls
                    // keeps the faults spread over the device.
                    switch (injections[i].after_which) {
                      case ERASES:
                        if (injections[i].after_type == RAND) {
                            injections[i].actual_count = number_of_erase_calls + rand() % injections[i].after_count;
                        } else {
                            injections[i].actual_count = number_of_erase_calls + injections[i].after_count;
                        }
                        break;
                        
                      case WRITES:
                        if (injections[i].after_type == RAND) {
                            injections[i].actual_count = number_of_write_calls + rand() % injections[i].after_count;
                        } else {
                            injections[i].actual_count = number_of_write_calls + injections[i].after_count;
                        }
                        break;
                        
                      case CALLS:
                        if (injections[i].after_type == RAND) {
                            injections[i].actual_count = number_of_calls + rand() % injections[i].after_count;
                        } else {
                            injections[i].actual_count = number_of_calls + injections[i].after_count;
                        }
                        break;

                      case BLOCK_ERASES:
                      case PAGE_WRITES:
                        // actual_count decrements with each relevant operation, so reset it
                        injections[i].actual_count = injections[i].after_count;
                        break;
                    }

                    // And clear the pending flag until the next time the actual_count triggers.
                    injections[i].pending = 0;
                }
            }
        }
    }
    
    if ( make_bad && !is_page_bad(page)) {
        number_of_errors_injected += 1;
        if (log_error) {
            int len = diag_sprintf(logfile_data, "Bp %d %d %d %d\n",
                                   number_of_errors_injected, number_of_calls, page, page / PAGES_PER_BLOCK);
            logfile_write(len);
        }
        mark_page_bad(page);
        // Actual corruption of the written data happens in the write call,
        // which has a better idea of what gets written where.
        return 1;
    }
    return 0;
}

static int
erase_check_injections(cyg_nand_block_addr block)
{
    int i;
    int make_bad            = 0;
    
    for (i = 0; i < MAX_BAD_BLOCK_INJECTIONS; i++) {
        if ( ! injections[i].enabled) {
            continue;
        }

        // First, figure out which injections should become pending.
        if (! injections[i].pending) {
            switch(injections[i].after_which) {
              case ERASES:
                if (number_of_erase_calls >= injections[i].actual_count) {
                    injections[i].pending   = 1;
                }
                break;
              case WRITES:
                if (number_of_write_calls >= injections[i].actual_count) {
                    injections[i].pending   = 1;
                }
                break;
              case CALLS:
                if (number_of_calls >= injections[i].actual_count) {
                    injections[i].pending   = 1;
                }
                break;
              case PAGE_WRITES:
                // A PAGE_WRITES counter cannot decrement because of an erase
                break;
              case BLOCK_ERASES:
                if ((injections[i].type == ERASE) && (block == injections[i].number)) {
                    injections[i].actual_count -= 1;
                    if (injections[i].actual_count <= 0) {
                        injections[i].pending   = 1;
                    }
                }
                if ((injections[i].type == WRITE) && (block == (injections[i].number / PAGES_PER_BLOCK))) {
                    injections[i].actual_count -= 1;
                    if (injections[i].actual_count <= 0) {
                        injections[i].pending   = 1;
                    }
                }
                break;
            }
        }
        if ((injections[i].type == ERASE) && (injections[i].pending)) {
            if ((injections[i].affects == CURRENT) || (injections[i].number == block)) {
                // This will cause the block to be made bad later on. The flag may
                // be set several times because multiple injection definitions trigger.
                make_bad    = 1;
                // Now, if this injection is not repeatable then we can disable it and
                // it will be ignored from here on.
                if (! injections[i].repeat) {
                    injections[i].enabled   = 0;
                } else {
                    // OK, we need to reset actual_count.
                    switch (injections[i].after_which) {
                      case ERASES:
                        if (injections[i].after_type == RAND) {
                            injections[i].actual_count = number_of_erase_calls + rand() % injections[i].after_count;
                        } else {
                            injections[i].actual_count = number_of_erase_calls + injections[i].after_count;
                        }
                        break;
                        
                      case WRITES:
                        if (injections[i].after_type == RAND) {
                            injections[i].actual_count = number_of_write_calls + rand() % injections[i].after_count;
                        } else {
                            injections[i].actual_count = number_of_write_calls + injections[i].after_count;
                        }
                        break;
                        
                      case CALLS:
                        if (injections[i].after_type == RAND) {
                            injections[i].actual_count = number_of_calls + rand() % injections[i].after_count;
                        } else {
                            injections[i].actual_count = number_of_calls + injections[i].after_count;
                        }
                        break;

                      case BLOCK_ERASES:
                      case PAGE_WRITES:
                        // actual_count decrements with each relevant operation, so reset it
                        injections[i].actual_count = injections[i].after_count;
                        break;
                    }

                    // And clear the pending flag until the next time the actual_count triggers.
                    injections[i].pending = 0;
                }
            }
        }
    }
    
    if ( make_bad && !is_block_bad(block)) {
        number_of_errors_injected += 1;
        if (log_error) {
            int len = diag_sprintf(logfile_data, "Bb %d %d %d\n", number_of_errors_injected, number_of_calls, block);
            logfile_write(len);
        }
        // Actual corruption of the data happens in the erases call,
        // which has a better idea of what gets written where.
        mark_block_bad(block);
        return 1;
    }
    return 0;
}

// ----------------------------------------------------------------------------
// Opening the nand image file, or creating a new one.
//
// The file format is as follows:
//   A 64-byte parameter block containing:
//     A magic number 0xEC05A11F
//     Page size
//     Spare (OOB) size
//     Pages per block
//     Number of blocks
//     timeval.tv_sec
//     timeval.tv_usec
//     Spare fields.
//   Erase counts
//     Number_of_blocks integers
//   Write counts
//     (Number_of_blocks * Pages_per_block) integers
//   Factory-bad blocks.
//     32 integers. -1 indicates no entry.
//   An array of good/bad blocks
//     1 bit per block. 1 == ok, 0 == bad
//   The data:
//     All eraseblocks concatenated without padding
//       All pages without padding
//         The data area followed by the spare area.

#define NAND_SYNTH_MAGIC        0xec05a11F


static int
open_image(void)
{
    int*    header;
    int*    factory_bads;
    int     written;
    int     i;
    int     to_write;
    int     file_size;
    int     ok;
    
    image_fd    = cyg_hal_sys_open(IMAGE_FILENAME, CYG_HAL_SYS_O_RDWR,
                                   CYG_HAL_SYS_S_IRUSR | CYG_HAL_SYS_S_IWUSR | CYG_HAL_SYS_S_IRGRP | CYG_HAL_SYS_S_IWGRP);
    if (-ENOENT == image_fd) {
        diag_printf("NAND synth_devinit: creating new image file %s\n", IMAGE_FILENAME);
        image_fd    =cyg_hal_sys_open(IMAGE_FILENAME, CYG_HAL_SYS_O_RDWR | CYG_HAL_SYS_O_CREAT,
                                      CYG_HAL_SYS_S_IRUSR | CYG_HAL_SYS_S_IWUSR | CYG_HAL_SYS_S_IRGRP | CYG_HAL_SYS_S_IWGRP);
        if (image_fd < 0) {
            diag_printf("NAND synth_devinit: error, failed to create image file %s\n", IMAGE_FILENAME);
            return -EIO;
        }

        // We can use synth_buf to populate the file. Start with the
        // header.
        memset(synth_buf, 0x00FF, SYNTH_BUFSIZE);
        header      = (int*)synth_buf;
        *header++   = CYG_CPU_TO_BE32(NAND_SYNTH_MAGIC);
        *header++   = CYG_CPU_TO_BE32(PAGESIZE);
        *header++   = CYG_CPU_TO_BE32(SPARE_PER_PAGE);
        *header++   = CYG_CPU_TO_BE32(PAGES_PER_BLOCK);
        *header++   = CYG_CPU_TO_BE32(BLOCK_COUNT);
        *header++   = CYG_CPU_TO_BE32((cyg_uint32)timeval.hal_tv_sec);
        *header++   = CYG_CPU_TO_BE32((cyg_uint32)timeval.hal_tv_usec);

        written     = cyg_hal_sys_write(image_fd, synth_buf, HEADER_SIZE);
        if (written != HEADER_SIZE) {
            cyg_hal_sys_close(image_fd);
            diag_printf("NAND synth_devinit: error, failed to write header to image file %s\n", IMAGE_FILENAME);
            return -EIO;
        }
        // And the erase count and page write count arrays
        memset(synth_buf, 0x00, SYNTH_BUFSIZE);
        to_write    = BLOCK_COUNT * (PAGES_PER_BLOCK + 1) * sizeof(int);
        while (to_write > 0) {
            int this_write = (to_write > SYNTH_BUFSIZE) ? SYNTH_BUFSIZE : to_write;
            written = cyg_hal_sys_write(image_fd, synth_buf, this_write);
            if (written != this_write) {
                cyg_hal_sys_close(image_fd);
                diag_printf("NAND synth_devinit: error, failed to write counters to image file %s\n", IMAGE_FILENAME);
                return -EIO;
            }
            to_write -= written;
        }

        // And the factory-bad blocks.
        memset(synth_buf, 0x00FF, SYNTH_BUFSIZE);
        factory_bads    = (int*)synth_buf;
        for (i = 0; (i < MAX_FACTORY_BAD) && (-1 != factory_bad_blocks[i]); i++) {
            if (factory_bad_blocks[i] >= BLOCK_COUNT) {
                diag_printf("NAND synth_devinit: warning, invalid factory bad block %d\n    There are only %d erase blocks.\n",
                            factory_bad_blocks[i], BLOCK_COUNT);
            } else {
                *factory_bads++ = CYG_CPU_TO_BE32((cyg_uint32)factory_bad_blocks[i]);
            }
        }
        written     = cyg_hal_sys_write(image_fd, synth_buf, MAX_FACTORY_BAD * 4);
        if (written != (MAX_FACTORY_BAD * 4)) {
            cyg_hal_sys_close(image_fd);
            diag_printf("NAND synth_devinit: error, failed to write factory bad block data to image file %s\n", IMAGE_FILENAME);
            return -EIO;
        }

        // Now for the ok/bad blocks array. The current maximum is 8K blocks so 1K
        // will do nicely.
#if (BLOCK_COUNT > 8192)
# error Driver init does not support block counts > 8192.
#endif
#if ((BLOCK_COUNT % 8) != 0)
# error Erase block count should be a multiple of 8.        
#endif        
#if ((BLOCK_COUNT + 7) / 8) > SYNTH_BUFSIZE
# error Buffer sizes need adjusting.        
#endif
        memset(synth_buf, 0x00FF, SYNTH_BUFSIZE);
        for (i = 0; (i < MAX_FACTORY_BAD) && (-1 != factory_bad_blocks[i]); i++) {
            int block = factory_bad_blocks[i];
            if (block >= BLOCK_COUNT) {
                continue;   // A warning has been issued already.
            }
            synth_buf[block / 8] &= ~(0x01 << (block % 8));
        }
        written = cyg_hal_sys_write(image_fd, synth_buf, BLOCK_COUNT / 8);
        if ((BLOCK_COUNT / 8) != written) {
            cyg_hal_sys_close(image_fd);
            diag_printf("NAND synth_devinit: error, failed to write bad block data to image file %s\n", IMAGE_FILENAME);
            return -EIO;
        }

        // And finally the data.
        memset(synth_buf, 0x00FF, SYNTH_BUFSIZE);
        to_write = DATA_SIZE;
        while (to_write > 0) {
            int this_write = (to_write > SYNTH_BUFSIZE) ? SYNTH_BUFSIZE : to_write;
            written = cyg_hal_sys_write(image_fd, synth_buf, this_write);
            if (written > this_write) {
                cyg_hal_sys_close(image_fd);
                diag_printf("NAND synth_devinit: error, failed to write initial empty data to image file %s\n", IMAGE_FILENAME);
                return -EIO;
            }
            to_write -= written;
        }

        // Move back to the start of the file
        if (cyg_hal_sys_lseek(image_fd, 0, CYG_HAL_SYS_SEEK_SET) < 0) {
            cyg_hal_sys_close(image_fd);
            diag_printf("NAND synth_devinit: error, failed to seek back to start of image file %s\n", IMAGE_FILENAME);
            return -EIO;
        }
    }

    // The image file exists and has been opened, and the current seek position is at the start.
    // Before doing anything else, check the header.
    if (HEADER_SIZE != cyg_hal_sys_read(image_fd, synth_buf, HEADER_SIZE)) {
            cyg_hal_sys_close(image_fd);
            diag_printf("NAND synth_devinit: error, failed to read header from image file %s\n", IMAGE_FILENAME);
            return -EIO;
    }
    header  = (int*)synth_buf;
    ok  = 1;
    if (*header++ != CYG_CPU_TO_BE32(NAND_SYNTH_MAGIC)) {
        diag_printf("NAND synth_devinit: error, invalid header magic in image file %s\n", IMAGE_FILENAME);
        ok  = 0;
    } else {
        if (*header != CYG_CPU_TO_BE32(PAGESIZE)) {
            diag_printf("NAND synth_devinit: page size mismatch in image file %s\n", IMAGE_FILENAME);
            diag_printf("    Expected page size is %d bytes, file uses %d bytes\n",
                        PAGESIZE, (int)CYG_BE32_TO_CPU(*header));
            ok = 0;
        }
        header++;
        if (*header != CYG_CPU_TO_BE32(SPARE_PER_PAGE)) {
            diag_printf("NAND synth_devinit: spare per page (OOB) mismatch in image file %s\n", IMAGE_FILENAME);
            diag_printf("    Expected size is %d bytes, file uses %d bytes\n",
                        SPARE_PER_PAGE, (int)CYG_BE32_TO_CPU(*header));
            ok = 0;
        }
        header++;
        if (*header != CYG_CPU_TO_BE32(PAGES_PER_BLOCK)) {
            diag_printf("NAND synth_devinit: pages per block mismatch in image file %s\n", IMAGE_FILENAME);
            diag_printf("    Expected size is %d pages, file uses %d pages\n",
                        SPARE_PER_PAGE, (int)CYG_BE32_TO_CPU(*header));
            ok = 0;
        }
        header++;
        if (*header != CYG_CPU_TO_BE32(BLOCK_COUNT)) {
            diag_printf("NAND synth_devinit: erase block count mismatch in image file %s\n", IMAGE_FILENAME);
            diag_printf("    Expected count is %d blocks, file uses %d blocks\n",
                        BLOCK_COUNT, (int)CYG_BE32_TO_CPU(*header));
            ok = 0;
        }
        if (!ok) {
            cyg_hal_sys_close(image_fd);
            return -EIO;
        }
    }

    // Also check the file size.
    file_size       = cyg_hal_sys_lseek(image_fd, 0, CYG_HAL_SYS_SEEK_END);
    if (file_size != IMAGE_SIZE) {
        diag_printf("NAND synth_devinit: file size mismatch for image file %s\n", IMAGE_FILENAME);
        diag_printf("  The file should be %d bytes long. It is %d bytes long.\n", IMAGE_SIZE, file_size);
        cyg_hal_sys_close(image_fd);
        return -EIO;
    }

    // The file appears to be ok. Try to mmap it.
    // NOTE: allowing both READ and WRITE access here leaves the NAND image
    // open to accidental memory corruption. An alternative approach would
    // be to only specify READ here, and add mprotect() calls in the
    // write and erase functions. That would slow those operations by two
    // system calls, but arguably those operations should be a lot slower
    // than reads anyway.
    i = cyg_hal_sys_mmap(NULL,
                         IMAGE_SIZE,
                         CYG_HAL_SYS_PROT_READ | CYG_HAL_SYS_PROT_WRITE,
                         CYG_HAL_SYS_MAP_SHARED,
                         image_fd,
                         0);
    // The resulting address may be in the top half of the address map, so
    // testing for errors requires a bit of a kludge.
    if ((i < 0) && (i > -256)) {
        diag_printf("NAND synth_devinit: failed to mmap image file %s, error 0x%08x\n", IMAGE_FILENAME, i);
        cyg_hal_sys_close(image_fd);
        return -EIO;
    }
    image_header        = (int*) i;
    image_erase_counts  = (int*) (i + HEADER_SIZE);
    image_write_counts  = image_erase_counts + BLOCK_COUNT;
    image_factory_bads  = image_write_counts + (BLOCK_COUNT * PAGES_PER_BLOCK);
    image_ok_blocks     = (unsigned char*) (image_factory_bads + MAX_FACTORY_BAD);
    image_data          = image_ok_blocks + (BLOCK_COUNT / 8);

#if 0
    diag_printf("Image header    @ %p\n", image_header);
    diag_printf("Erase counts    @ %p\n", image_erase_counts);
    diag_printf("Write counts    @ %p\n", image_write_counts);
    diag_printf("Factory-bads    @ %p\n", image_factory_bads);
    diag_printf("OK block bitmap @ %p\n", image_ok_blocks);
    diag_printf("Image data      @ %p\n", image_data);   
#endif    
    // Final sanity check. Make sure that all the factory bad blocks are marked bad.
    for (i = 0; i < MAX_FACTORY_BAD; i++) {
        int bad = CYG_BE32_TO_CPU(image_factory_bads[i]);
        if ( (-1 != bad) && is_block_ok(bad)) {
            diag_printf("NAND synth_devinit: warning, block %d should be factory-bad but is marked as OK.\n", bad);
        }
    }

    // Update the timestamp in the header to match this test run.
    image_header[5] = CYG_CPU_TO_BE32((cyg_uint32)timeval.hal_tv_sec);
    image_header[6] = CYG_CPU_TO_BE32((cyg_uint32)timeval.hal_tv_usec);
    
    create_checkpoint();
    return 0;
}



/* -------------------------------------------------------------------- */

static size_t log2u(size_t v)
{
    size_t r = 0;
    while (v>>=1) ++r;
    return r;
}

static int synth_devinit(cyg_nand_device *dev)
{
    int rv = 0;

    cyg_drv_mutex_init(&lock);
    cyg_drv_mutex_lock(&lock);

#define xreturn(_r) do { rv = _r; goto err_exit; } while(0)

    // Remember the start of this run. This timestamp is incorporated into
    // both the image file header and the logfile, allowing the two to be
    // matched up.
    cyg_hal_sys_gettimeofday(&timeval, (struct cyg_hal_sys_timezone*)0);
    
    if (synth_auxiliary_running) {
        nand_synth_init();
    }

    rv = open_image();
    if (rv < 0) {
        image_fd    = -1;
        goto err_exit;
    }

    logfile_open();

    /* OK, synth file is good, so we set up the usual dev struct */
    dev->page_bits          = log2u(PAGESIZE);
    dev->spare_per_page     = SPARE_PER_PAGE;
    dev->block_page_bits    = log2u(PAGES_PER_BLOCK);
    dev->blockcount_bits    = log2u(BLOCK_COUNT);
    dev->chipsize_log       = dev->page_bits + dev->block_page_bits + dev->blockcount_bits;

#ifdef CYGSEM_IO_NAND_USE_BBT
    dev->bbt.datasize       = BLOCK_COUNT / 4;
    dev->bbt.data           = bbtable;
#endif
    if (SPARE_PER_PAGE >= 64) {
        dev->oob = &nand_mtd_oob_64;
    } else if (SPARE_PER_PAGE >= 16) {
        dev->oob = &nand_mtd_oob_16;
    }
    else {
#if 0 /* cannot use - see comment in nand_oob.c*/
        if (priv->params->sparesize >= 8)
                dev->oob = &nand_mtd_oob_8;
#endif
        diag_printf("NAND synth_devinit: error, OOB per page %u is not large enough (needs 16)\n", (unsigned)SPARE_PER_PAGE);
        rv = -ENOSYS;
        goto err_exit;
    }

    /* Partition table setup. So far only manual-cdl implemented. */
#ifdef CYGSEM_DEVS_NAND_SYNTH_PARTITION_MANUAL_CONFIG
# define LASTBLOCK ((1<<dev->blockcount_bits)-1)
# define PARTITION(i) do {                                      \
    cyg_nand_block_addr                                         \
        base = CYGNUM_DEVS_NAND_SYNTH_PARTITION_ ## i ## _BASE, \
        size = CYGNUM_DEVS_NAND_SYNTH_PARTITION_ ## i ## _SIZE, \
        last = size ? base + size - 1 : LASTBLOCK;              \
    dev->partition[i].dev = dev;                                \
    dev->partition[i].first = base;                             \
    dev->partition[i].last = last;                              \
    if ((base>LASTBLOCK)||(last>LASTBLOCK)) {                   \
        diag_printf("ERROR: Partition %d extends beyond file params (%d+%d=%d > %d), disabling\n", i, base, size, last, LASTBLOCK);     \
        dev->partition[i].dev = 0;                              \
    }                                                           \
} while(0)

# ifdef CYGPKG_DEVS_NAND_SYNTH_PARTITION_0
    PARTITION(0);
# endif
# ifdef CYGPKG_DEVS_NAND_SYNTH_PARTITION_1
    PARTITION(1);
# endif
# ifdef CYGPKG_DEVS_NAND_SYNTH_PARTITION_2
    PARTITION(2);
# endif
# ifdef CYGPKG_DEVS_NAND_SYNTH_PARTITION_3
    PARTITION(3);
# endif
#endif  // MANUAL_CONFIG

err_exit:
    cyg_drv_mutex_unlock(&lock);
    return rv;
}

#ifdef CYGSEM_NAND_SYNTH_RANDOMLY_LOSE
unsigned losscount=0;

externC unsigned
cyg_nand_synth_get_losscount(void)
{
    return losscount;
}
#endif

// Hacky state storage, protected by the dev lock:
static size_t _offset;
static size_t _stridden;
#ifdef CYGSEM_NAND_SYNTH_RANDOMLY_LOSE
static unsigned _lose;
static unsigned _lose_where;
static unsigned _lose_bit;
#endif
static int _page_going_bad;
static cyg_nand_page_addr _writepage;

static int
synth_readbegin(cyg_nand_device *dev, cyg_nand_page_addr page)
{
    CYG_ASSERTC(image_fd >= 0);
    CYG_ASSERTC(dev != NULL);
    CYG_ASSERTC((page >= 0)     && (page < (PAGES_PER_BLOCK * BLOCK_COUNT)));

    cyg_drv_mutex_lock(&lock);
    number_of_calls         += 1;
    number_of_read_calls    += 1;
    
    if (log_read) {
        int len = diag_sprintf(logfile_data, "rb %d %d %d\n",
                               number_of_read_calls, number_of_calls, page);
        logfile_write(len);
    }

    if (log_READ) {
        int len = diag_sprintf(logfile_data, "Rdb %d %d %d\n",
                number_of_read_calls, number_of_calls, page);
        logfile_write(len);
    }

    if (is_page_bad(page)) {
        // This is actually allowed. After a failed page write, higher-level code
        // may need to recover the data in the other blocks. We use a heuristic
        // to suppress unwanted warnings, although that may fail if two blocks
        // go bad in quick succession.
        if ((-1 == last_block_gone_bad) || ((page / PAGES_PER_BLOCK) != last_block_gone_bad)) {
            diag_printf("NAND synth_readpage: warning, attempt to read page %d which is in a block previously marked bad.\n"
                        "  This should only happen immediately after a previous write failure\n"
                        "  to recover the data in the remaining pages in the erase block.\n",
                        page);
        }
    }

    _offset  = page * (PAGESIZE + SPARE_PER_PAGE);
    _stridden = 0;

#ifdef CYGSEM_NAND_SYNTH_RANDOMLY_LOSE
    {
        if (rand()&(1<<13)) { // 50% chance of bitloss..
            _lose = ++losscount;
            // where (which byte) will we lose: in the page or in its ECC?
#define ECC_COUNT ((PAGESIZE/256)*3)
            int range = PAGESIZE + SPARE_PER_PAGE;
            _lose_where=rand()%range; // slightly biased selection but will do
            _lose_bit = rand()%8;

        } else
            _lose = 0;
    }
#endif

    return 0;
}

static int
synth_readstride(cyg_nand_device *dev, void * dest, size_t size)
{
    CYG_ASSERTC((dest == NULL)  || ((size >= 0) && (size <= PAGESIZE)));

    if (dest && (size > 0)) {
        memcpy(dest, &(image_data[_offset + _stridden]), size);
    }

#ifdef CYGSEM_NAND_SYNTH_RANDOMLY_LOSE
    if (_lose && dest) {
        if (_lose_where < PAGESIZE) {
            if ((_lose_where >= _stridden) && (_lose_where < _stridden+size)) {
                ((unsigned char*)dest)[_lose_where - _stridden] ^= (1<<_lose_bit);
                _lose = 0;
            }
        }
    }
#endif

    if (log_read) {
        int len = diag_sprintf(logfile_data, "rs %p %d\n", dest, (int)size);
        logfile_write(len);
    }

    if (log_READ) {
        if (dest) {
            int len = diag_sprintf(logfile_data, "Rdd %p %d ", dest, (int)size);
            len += addhex(&(logfile_data[len]), (unsigned char*)dest, (int) size);
            logfile_data[len++] = '\n';
            logfile_write(len);
        }
    }

    _stridden += size;
    return 0;
}

static int
synth_readfinish(cyg_nand_device *dev, void * spare, size_t spare_size)
{
    CYG_ASSERTC((spare == NULL) || ((spare_size >= 0) && (spare_size <= SPARE_PER_PAGE)));
    CYG_ASSERTC(_stridden || spare); // No data and no spare probably means an error

    if (spare && (spare_size > 0)) {
        memcpy(spare, &(image_data[_offset + PAGESIZE]), spare_size);
    }

#ifdef CYGSEM_NAND_SYNTH_RANDOMLY_LOSE
    if (_lose && spare) {
        if (_lose_where >= PAGESIZE) {
            unsigned spare_offset = _lose_where - PAGESIZE;
            if (spare_offset <= spare_size) {
                ((unsigned char*)spare)[spare_offset] ^= (1<<_lose_bit);
                _lose = 0;
            }
        }
    }
#endif

    if (log_read) {
        int len = diag_sprintf(logfile_data, "rf t=%d %p %d\n",
                _stridden, spare, (int)spare_size);
        logfile_write(len);
    }
    if (log_READ) {
        if (spare) {
            int len = diag_sprintf(logfile_data, "Rdf %p %d ",
                    spare, (int)spare_size);
            len += addhex(&(logfile_data[len]), (unsigned char*)spare, (int)spare_size);
            logfile_data[len++] = '\n';
            logfile_write(len);
        }
    }

    cyg_drv_mutex_unlock(&lock);
    return 0;
}

static int synth_readpart(cyg_nand_device *dev, void *dest,
                          cyg_nand_page_addr page,
                          size_t offset, size_t length)
{
    size_t page_offset  = page * (PAGESIZE + SPARE_PER_PAGE);

    cyg_drv_mutex_lock(&lock);
    number_of_calls         += 1;
    number_of_read_calls    += 1;

    memcpy(dest, &image_data[page_offset + offset], length);

#ifdef CYGSEM_NAND_SYNTH_RANDOMLY_LOSE
    {
        if (rand()&(1<<13)) { // 50% chance of bitloss..
            _lose = ++losscount;
            // where (which byte) will we lose: we might be lucky and
            // choose something outside of the read range.

            int range = PAGESIZE;
            int _lose_where=rand()%range; // slightly biased selection but will do

            if ( (_lose_where >= offset) && (_lose_where < offset+length) ) {
                CYG_BYTE *b = (CYG_BYTE*) dest;
                int _lose_bit = rand()%8;
                b[_lose_where - offset] ^= 1<< _lose_bit;
            }

        } else
            _lose = 0;
    }
#endif

    if (log_read) {
        int len = diag_sprintf(logfile_data, "rpp %d %d %d %d %d\n",
                               number_of_read_calls, number_of_calls,
                               page, offset, length);
        logfile_write(len);
    }

    if (log_READ) {
        int len = diag_sprintf(logfile_data, "Rpp %d %d %d %d %d ",
                number_of_read_calls, number_of_calls, page, offset, length);
        len += addhex(&(logfile_data[len]), (unsigned char*)dest, (int) length);
        logfile_data[len++] = '\n';
        logfile_write(len);
    }

    cyg_drv_mutex_unlock(&lock);
    return 0;
}

static int
synth_writebegin(cyg_nand_device *dev, cyg_nand_page_addr page)
{
    int     result  = 0;
    int     i;
    int     counter;
    CYG_ASSERTC(image_fd >= 0);
    CYG_ASSERTC(dev != NULL);
    CYG_ASSERTC((page >= 0)     && (page < (PAGES_PER_BLOCK * BLOCK_COUNT)));

    cyg_drv_mutex_lock(&lock);
    number_of_calls         += 1;
    number_of_write_calls   += 1;
    _offset = page * (PAGESIZE + SPARE_PER_PAGE);
    _stridden = 0;
    _writepage = page;

    if (log_write) {
        int len = diag_sprintf(logfile_data, "wb %d %d %d\n",
                               number_of_write_calls, number_of_calls, page);
        logfile_write(len);
    }
    if (log_WRITE) {
        int len = diag_sprintf(logfile_data, "Wb %d %d %d ",
                number_of_write_calls, number_of_calls, page);
        logfile_write(len);
    }

    if (is_page_bad(page)) {
        diag_printf("NAND synth_writepage: attempt to write page %d which is in a block previously marked bad.\n", page);
        result = -EIO;  // Disallow overwrites of a page in a bad block. Arguably this should be permitted.
    }

    // Do we make it bad this time?
    if (0 == result) _page_going_bad = write_check_injections(page);

#if CYGSEM_NAND_SYNTH_ALLOW_MULTIPLE_WRITES == 1
    (void) i;
#else
    // Do not allow multiple writes to a single page, for now.
    if (0 == result) {
        for (i = 0; i < PAGESIZE; i++) {
            if (0x00FF != image_data[_offset + i]) {
                diag_printf("NAND synth_writepage: attempt to write to page %d which is not erased.\n", page);
                diag_printf("  Value at offset 0x%08x is 0x%02x\n", (int)(_offset + i), image_data[_offset + i]);
                result  = -EIO;
                break;
            }
        }
    }
#endif

    if (0 == result) {
        counter                     = CYG_BE32_TO_CPU(image_write_counts[page]);
        counter                    += 1;
        image_write_counts[page]    = CYG_CPU_TO_BE32(counter);
    }
    
    if (0 != result) cyg_drv_mutex_unlock(&lock);
    return result;
}

static int
synth_writestride(cyg_nand_device *dev, const void * src, size_t size)
{
    int     idx,i;
    CYG_ASSERTC((src == NULL)   || ((size >= 0) && (size <= PAGESIZE)));

    if (log_write) {
        int len = diag_sprintf(logfile_data, "ws %p %d\n",
                               src, (int)size);
        logfile_write(len);
    }
    if (log_WRITE) {
        if (src) {
            int len = diag_sprintf(logfile_data, "Wds %p %d ", src, (int)size);
            len += addhex(&(logfile_data[len]), (unsigned char*)src, (int) size);
            logfile_data[len++] = '\n';
            logfile_write(len);
        }
    }

    if (src && (size > 0)) {
#if CYGSEM_NAND_SYNTH_ALLOW_MULTIPLE_WRITES == 1
        for (i=0; i<size; i++)
            image_data[_offset + _stridden + i] &= ((unsigned char*)src)[i];
#else
        memcpy(&(image_data[_offset + _stridden]), src, size);
#endif
        if (_page_going_bad) {
            // Fake it so that most of the write has succeeded, but one byte
            // is stuck at 0xFF. That means looking for a byte in src that
            // is not 0xFF. A minor improvement would be to affect just one
            // bit, not all bits.
            idx = rand() % size;
            for (i = 0; i < size; i++) {
                if (image_data[_offset + _stridden + idx] != 0x00FF) {
                    image_data[_offset + _stridden + idx] = 0xFF;
                    _page_going_bad = 0;    // Do not corrupt the OOB data as well.
                    break;
                }
                idx = (idx + 1) % size;
            }
        }
    }
    // There is still the possibility where page_gone_bad, but all the data
    // written was 0xFF. In that scenario all the data in the image is
    // actually correct but we are still reporting -EIO. Probably not
    // worth worrying about.

    _stridden += size;
    return 0;
}

static int
synth_writefinish(cyg_nand_device *dev, const void * spare, size_t spare_size)
{
    int     result  = 0;
    int     idx,i;
    
    CYG_ASSERTC((spare == NULL) || ((spare_size >= 0) && (spare_size <= SPARE_PER_PAGE)));
    CYG_ASSERTC(_stridden || spare);
    
    if (log_write) {
        int len = diag_sprintf(logfile_data, "wf %p %d\n",
                               spare, (int)spare_size);
        logfile_write(len);
    }
    if (log_WRITE) {
        if (spare) {
            int len = diag_sprintf(logfile_data, "Wfo %p %d ",
                                   spare, (int)spare_size);
            len += addhex(&(logfile_data[len]), (unsigned char*)spare, (int)spare_size);
            logfile_data[len++] = '\n';
            logfile_write(len);
        }
    }

    if (0 == result) {
        if (spare && (spare_size > 0)) {
#if CYGSEM_NAND_SYNTH_ALLOW_MULTIPLE_WRITES == 1
            for (i=0; i<spare_size; i++)
                image_data[_offset + PAGESIZE + i] &= ((unsigned char*)spare)[i];
#else
            memcpy(&(image_data[_offset + PAGESIZE]), spare, spare_size);
#endif
            if (_page_going_bad) {
                idx = rand() % spare_size;
                for (i = 0; i < spare_size; i++) {
                    if (image_data[_offset + PAGESIZE + idx] != 0x00FF) {
                        image_data[_offset + PAGESIZE + idx] = 0xFF;
                        break;
                    }
                    idx = (idx + 1) % spare_size;
                }
            }
        }
    }

    if (is_page_bad(_writepage)) {
        result = -EIO;
    }
    
    cyg_drv_mutex_unlock(&lock);
    return result;
}

static int
synth_eraseblock(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    int     result  = 0;
    size_t  offset;
    int     block_gone_bad  = 0;
    int     counter;

    CYG_ASSERTC(image_fd >= 0);
    CYG_ASSERTC(dev != NULL);
    CYG_ASSERTC((blk >= 0) && (blk < BLOCK_COUNT));

    cyg_drv_mutex_lock(&lock);
    number_of_calls         += 1;
    number_of_erase_calls   += 1;
    offset  = blk * PAGES_PER_BLOCK * (PAGESIZE + SPARE_PER_PAGE);

    if (log_erase) {
        int len = diag_sprintf(logfile_data ,"E %d %d %d\n",
                               number_of_erase_calls, number_of_calls, blk);
        logfile_write(len);
    }
    
    if (is_block_bad(blk)) {
        diag_printf("NAND synth_eraseblock: attempt to erase block %d which was previously marked bad.\n", blk);
        result = -EIO;
    }
    if (0 == result) {
        counter                  = CYG_BE32_TO_CPU(image_erase_counts[blk]);
        counter                 += 1;
        image_erase_counts[blk]  = CYG_CPU_TO_BE32(counter);
    }
    if (0 == result) {
        block_gone_bad = erase_check_injections(blk);
        memset(&(image_data[offset]), 0x00FF, PAGES_PER_BLOCK * (PAGESIZE + SPARE_PER_PAGE));
        if (block_gone_bad) {
            // Clear one bit in the block. It would be slightly better to find a bit
            // that was cleared prior to the erase, but not worth worrying about.
            int idx = rand() % (PAGES_PER_BLOCK * (PAGESIZE + SPARE_PER_PAGE));
            int bit = 0x01 << (rand() % 8);
            image_data[offset + idx] &= ~bit;
        }
    }
    if (is_block_bad(blk)) {
        result = -EIO;
    }

    cyg_drv_mutex_unlock(&lock);
    return result;
}

static int
synth_factorybad(cyg_nand_device *dev, cyg_nand_block_addr blk)
{
    int i;
    int is_bad  = 0;
    CYG_ASSERTC(image_fd >= 0);
    CYG_ASSERTC(dev != NULL);
    CYG_ASSERTC((blk >= 0) && (blk < BLOCK_COUNT));

    cyg_drv_mutex_lock(&lock);
    number_of_calls             += 1;
    number_of_factory_bad_calls += 1;

    // Check the actual factory bad blocks in the nand image file, not the
    // bad blocks from the user's .tdf or the dialog window. The latter
    // only get used when creating a new nand image.
    for (i = 0; i < MAX_FACTORY_BAD; i++) {
        if (blk == CYG_BE32_TO_CPU(image_factory_bads[i])) {
            is_bad = 1;
            break;
        }
    }
    
    if (logging_enabled) {
        int len = diag_sprintf(logfile_data, "F %d %d %d %d\n",
                               number_of_factory_bad_calls, number_of_calls, blk, is_bad);
        logfile_write(len);
    }
    cyg_drv_mutex_unlock(&lock);
    
    return is_bad;
}

static CYG_NAND_FUNS_V2(nand_synth_funs, synth_devinit,
        synth_readbegin, synth_readstride, synth_readfinish,
        synth_readpart,
        synth_writebegin, synth_writestride, synth_writefinish,
        synth_eraseblock, synth_factorybad);

CYG_NAND_DEVICE(nand_synth, "synth", &nand_synth_funs, NULL, &mtd_ecc256_fast, 0);

