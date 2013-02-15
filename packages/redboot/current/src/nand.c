//==========================================================================
//
//      nand.c
//
//      RedBoot - NAND library support
//
//==========================================================================
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
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    wry
// Date:         2009-05-29
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <redboot.h>
#include <pkgconf/system.h>
#ifdef CYGPKG_IO_FILEIO
# include <cyg/fileio/fileio.h>
#endif
#include <cyg/nand/nand.h>
#include <cyg/nand/nand_device.h>
#include <cyg/nand/util.h>

CYG_HAL_TABLE_BEGIN( __NAND_cmds_TAB__, NAND_cmds);
CYG_HAL_TABLE_END( __NAND_cmds_TAB_END__, NAND_cmds);

extern struct cmd __NAND_cmds_TAB__[], __NAND_cmds_TAB_END__;

#ifdef CYGSEM_REDBOOT_NAND_BADBLOCKS_CMD
static unsigned count_nand_devices(void)
{
    unsigned n = 0;
    cyg_nand_device *nand;
    for (nand = &cyg_nanddevtab[0]; nand != &cyg_nanddevtab_end; nand++,n++);
    return n;
}
#endif

//==========================================================================

static void
nand_usage(char *why)
{
    diag_printf("*** invalid 'nand' command: %s\n", why);
    cmd_usage(__NAND_cmds_TAB__, &__NAND_cmds_TAB_END__, "nand ");
}

static void
do_nand(int argc, char *argv[])
{
    struct cmd *cmd;

    if (argc < 2) {
        nand_usage("too few arguments");
        return;
    }
    if ((cmd = cmd_search(__NAND_cmds_TAB__, &__NAND_cmds_TAB_END__, 
                          argv[1])) != (struct cmd *)0) {
        (cmd->fun)(argc-1, argv+1);
        return;
    }
    nand_usage("unrecognized command");
}

RedBoot_nested_cmd("nand", "Manage NAND arrays", "{cmds}", do_nand, 
        __NAND_cmds_TAB__, &__NAND_cmds_TAB_END__);

//==========================================================================
// nand list - enumerates the available devices

static void nand_list_guts(const char *banner, const char *eachdev,
        const char *donefound, const char *donenone)
{
    cyg_bool found=false;
    cyg_nand_device *nand;
    for (nand = &cyg_nanddevtab[0]; nand != &cyg_nanddevtab_end; nand++) {
        if (!found) {
            diag_printf(banner);
            found=true;
        }
        diag_printf(eachdev, nand->devname);
    }
    if (found) {
        if (donefound)
            diag_printf(donefound);
    } else {
        if (donenone)
            diag_printf(donenone);
    }
}

static void nand_list(int argc, char*argv[])
{
    nand_list_guts("NAND devices available:\n", "\t%s\n", 
            0, "No NAND devices available.\n");
}

// Startup banner is suspiciously similar..
void _nand_info(void)
{
    nand_list_guts("NAND:", " %s", "\n", 0);
    // We could do more here, maybe interrogate each device like nand_info,
    // but we'd have to be certain that calling each device's devinit
    // wouldn't risk an unrecoverable crash in case of trouble.
}

local_cmd_entry("list", "Lists available NAND devices", "", nand_list, NAND_cmds);

//==========================================================================
// nand info DEVICE - list chip info, partitions & sizes

static void pretty_print(unsigned log) {
    char si = ' ';
    if (log>30) {
        si='G'; log -= 30;
    } else if (log>20) {
        si='M'; log -= 20;
    } else if (log>10) {
        si='k'; log -= 10;
    }
    diag_printf("%u %cB", 1<<log, si);
}

static void do_info(cyg_nand_device *nand)
{
    // Dev must be initialised.
    diag_printf("NAND device `%s':\n"
                "  %u bytes/page, %u pages/block, "
                "capacity %u blocks x ",
                nand->devname,
                1 << nand->page_bits,
                1 << nand->block_page_bits,
                1 << nand->blockcount_bits);
    pretty_print(nand->page_bits + nand->block_page_bits);
    diag_printf(" = ");
    pretty_print(nand->chipsize_log);
    diag_printf("\n");

    cyg_bool found = false;
    int p;
    for (p=0; p < CYGNUM_NAND_MAX_PARTITIONS; p++) {
        if (nand->partition[p].dev) {
            if (!found) {
                found=true;
                diag_printf("  Partition Start Blocks\n");
            }
            diag_printf("  %6u    %4u  %5u\n",
                    p,
                    nand->partition[p].first,
                    1 + nand->partition[p].last - nand->partition[p].first);
        }
    }
    if (!found)
        diag_printf("  (No partitions defined.)\n");
}

static void nand_info(int argc, char*argv[])
{
    cyg_nand_device *nand;

    int arg = 1;
    while (arg < argc) {
        const char *dev = argv[arg];
        if (0==cyg_nand_lookup(dev, &nand))
            do_info(nand);
        else
            diag_printf("No such NAND device `%s'\n", dev);
        ++arg;
    }

    // However, if they've not provided an arg ...
    if (argc == 1) {
        for (nand = &cyg_nanddevtab[0]; nand != &cyg_nanddevtab_end; nand++) {
            cyg_nand_lookup(nand->devname,0);
            do_info(nand);
        }
    }
}

local_cmd_entry("info", "Displays information about one or more NAND devices", "[<device>] [<device>...]", nand_info, NAND_cmds);

//==========================================================================
#ifdef CYGSEM_REDBOOT_NAND_BADBLOCKS_CMD
// nand badblocks [-d <device>] SUBCOMMAND - bad block info and management
// Shared handling allows -d device to default to the single present device.

CYG_HAL_TABLE_BEGIN( __NAND_badblocks_cmds_TAB__, NAND_badblocks_cmds);
CYG_HAL_TABLE_END( __NAND_badblocks_cmds_TAB_END__, NAND_badblocks_cmds);

extern struct cmd __NAND_badblocks_cmds_TAB__[],
                  __NAND_badblocks_cmds_TAB_END__;

static cmd_fun do_nand_badblocks;
static cmd_fun nbb_subcommand;

#define nbb_cmd(_w, _h, _u) \
    static struct cmd _cmd_tab_nbb_subcommand_##_w CYG_HAL_TABLE_QUALIFIED_ENTRY(NAND_badblocks_cmds,nbb_subcommand) = { #_w, _h, "[-d <device>] " _u, nbb_subcommand, 0, 0 }

struct cmd _cmd_tag_do_nand_badblocks CYG_HAL_TABLE_QUALIFIED_ENTRY(NAND_cmds, do_nand_badblocks) = {
    "badblocks", "Interrogates and manages the device bad block table",
    "<subcommand> [-d <device>] [<subcommand args>...]",
    do_nand_badblocks,
    __NAND_badblocks_cmds_TAB__, &__NAND_badblocks_cmds_TAB_END__
};

static void nbb_usage(const char *why)
{
    diag_printf("*** invalid `nand badblocks' subcommand: %s\n", why);
    cmd_usage(__NAND_badblocks_cmds_TAB__,
            &__NAND_badblocks_cmds_TAB_END__, "nand badblocks ");
}

static void do_nand_badblocks(int argc, char**argv)
{
    struct cmd *cmd;
    if (argc<2) {
        // defaulting behaviour
        nbb_subcommand(argc,argv);
        return;
    }
    if ((cmd = cmd_search(__NAND_badblocks_cmds_TAB__,
                    &__NAND_badblocks_cmds_TAB_END__, argv[1]))
            != (struct cmd *)0) {
        (cmd->fun)(argc, argv);
        return;
    }
    nbb_usage("unrecognized command");
}

static void nbb_help(int argc, char**argv)
{
    cmd_usage(__NAND_badblocks_cmds_TAB__,
            &__NAND_badblocks_cmds_TAB_END__, "nand badblocks ");
}

local_cmd_entry("help", "Explains the available commands", "", nbb_help, NAND_badblocks_cmds);

static struct {
    cyg_nand_bbt_status_t state;
    const char *msg;
} states[5] = {
    { CYG_NAND_BBT_OK, "OK" },
    { CYG_NAND_BBT_WORNBAD, "worn bad" },
    { CYG_NAND_BBT_RESERVED, "reserved" },
    { CYG_NAND_BBT_FACTORY_BAD, "factory bad" },
    { 0,0 }
};

static void nbb_states(int argc, char**argv)
{
    int i = 0;
    diag_printf("The possible block states are:\n");
    while (states[i].msg) {
        diag_printf("\t%u : %s\n", states[i].state, states[i].msg);
        ++i;
    }
}

local_cmd_entry("states", "Lists the valid states in the BBT", "", nbb_states, NAND_badblocks_cmds);

// .........................

// nand badblocks SUMMARY and nand badblocks LIST:
// count the number of blocks of a given type and (in list mode) dump them out.

// Forbidden knowledge:
extern int cyg_nand_bbti_query(cyg_nand_device *dev, cyg_nand_block_addr blk);

static void do_summary(cyg_nand_device *nand)
{
    // count the bad blocks
    cyg_nand_block_addr i = 0;
    unsigned state[1+CYG_NAND_BBT_FACTORY_BAD];
    int rv;
    memset(state, 0, sizeof(state));
    for (i=0; i < CYG_NAND_BLOCKCOUNT(nand); i++) {
        rv = cyg_nand_bbti_query(nand,i);
        if (rv>=CYG_NAND_BBT_OK && rv <= CYG_NAND_BBT_FACTORY_BAD)
            ++state[rv];
    }

    diag_printf("Device `%s' has %u blocks", nand->devname, CYG_NAND_BLOCKCOUNT(nand));

    int counted=0;
#define REPORT(_s,_msg) do {                                        \
    if (state[_s] != 0) {                                           \
        if (counted)                                                \
            diag_printf(", %u %s", state[_s], _msg);                \
        else                                                        \
            diag_printf(", of which %u are %s", state[_s], _msg);   \
        counted += state[_s];                                       \
    }                                                               \
} while(0)

    // Report in sensible order, not enum order.
    REPORT(CYG_NAND_BBT_FACTORY_BAD, "factory bad");
    REPORT(CYG_NAND_BBT_WORNBAD, "worn bad");
    REPORT(CYG_NAND_BBT_RESERVED, "reserved");

    int pct = 100 * (CYG_NAND_BLOCKCOUNT(nand) - counted) / CYG_NAND_BLOCKCOUNT(nand);
    diag_printf(". %u blocks (%d%%) are OK.\n", CYG_NAND_BLOCKCOUNT(nand) - counted, pct);
}

nbb_cmd(summary, "Summarises the bad blocks table", "");

// .........................

static void do_sublist(cyg_nand_device *nand, cyg_nand_bbt_status_t st, const char *msg)
{
    cyg_nand_block_addr i = 0;
    int rv;
    unsigned found = 0;

    diag_printf("%s: ", msg);
    for (i=0; i < CYG_NAND_BLOCKCOUNT(nand); i++) {
        rv = cyg_nand_bbti_query(nand,i);
        if (rv == st)  {
            diag_printf("%s%d", (found ? ", " : ""), i);
            ++found;
        }
    }

    if (found)
        diag_printf(". (count: %d blocks)\n", found);
    else
        diag_printf("no blocks\n");
}

static void do_list(cyg_nand_device *nand) {
    do_sublist(nand, CYG_NAND_BBT_FACTORY_BAD, "factory bad");
    do_sublist(nand, CYG_NAND_BBT_WORNBAD, "worn bad");
    do_sublist(nand, CYG_NAND_BBT_RESERVED, "reserved");
}

nbb_cmd(list, "Lists the contents of the bad blocks table", "");

// .........................

// nand badblocks mark -d device -b block -s state
// Manually marks a block in the BBT. Use with caution!
#ifdef CYGSEM_REDBOOT_NAND_BADBLOCKS_MARK_CMD
// more forbidden knowledge:
extern int cyg_nand_bbti_markany(cyg_nand_device *dev, cyg_nand_block_addr blk, cyg_nand_bbt_status_t st);

static void nbb_mark_usage(void) {
    diag_printf("Usage: nand badblocks mark [-d device] -b block -s state\n  The valid states are shown by `nand badblocks states'.\n");
}

static void do_mark(cyg_nand_device *nand, int markblock, int markstate) 
{
    int bail=0;
    if (markblock < 0 || markblock > CYG_NAND_BLOCKCOUNT(nand)) {
        diag_printf("Invalid block number.\n");
        bail=1;
    }
    if (markstate < CYG_NAND_BBT_OK || markstate > CYG_NAND_BBT_FACTORY_BAD) {
        diag_printf("Invalid state.\n");
        bail=1;
    }
    if (markstate == CYG_NAND_BBT_RESERVED) {
        diag_printf("State `reserved' is for the BBT itself and cannot be written to flash.\n");
        bail=1;
    }
    if (bail) {
        nbb_mark_usage();
        return;
    }


    // We don't need to take the devlock as RedBoot is single-threaded.
    int rv = cyg_nand_bbti_markany(nand, markblock, markstate);
    if (rv != 0)
        diag_printf("mark operation failed with status %d\n", rv);
    else
        diag_printf("OK\n");
}

nbb_cmd(mark, "Manually marks a block in the BBT", "-b block -s state");

#endif

// .........................

static void nbb_subcommand(int argc, char*argv[])
{
    cyg_nand_device *nand = 0;
    const char *subcmd = argv[1];
    char *dev;
    cyg_bool got_dev = false;
#define MAXOPTS 4
    struct option_info opts[MAXOPTS];
    enum {
        unset=0,
        dodefault,
        summary,
        list,
        mark
    } mode = unset;
    int markblock = -1, markstate = -1;

    // we know that we _have_ a valid subcommand at this point, just not _which_.
    int n_opts = 0;
#define OPTION(_f, _a, _t, _p, _got, _arg) init_opts(&opts[n_opts++], _f, _a, _t, _p, _got, _arg)
    OPTION('d', true, OPTION_ARG_TYPE_STR, (void*)&dev, &got_dev, "device");

    if (subcmd) {
        int sclen = strlen(subcmd);
        if (0==strncmp(subcmd, "summary", sclen))
            mode = summary;
        else if (0==strncmp(subcmd, "list", sclen))
            mode = list;
        else if (0==strncmp(subcmd, "mark", sclen)) {
            mode = mark;
            OPTION('s', true, OPTION_ARG_TYPE_NUM, (void*) &markstate, 0, "state");
            OPTION('b', true, OPTION_ARG_TYPE_NUM, (void*) &markblock, 0, "block");
        }

        // Caution! If you are adding a new subsubcommand with options, make sure that n_opts can't become bigger than MAXOPTS !
    }
    else
        mode = dodefault;

    if (n_opts > MAXOPTS) {
        diag_printf("BUG: Too many options at %s:%d\n",__FILE__,__LINE__);
        return;
    }

    if (!scan_opts(argc, argv, 2, opts, n_opts, 0, 0, ""))
        return;

    if (got_dev) {
        cyg_nand_lookup(dev, &nand);
        if (!nand)
            diag_printf("Device %s not found.\n", dev);
    } else {
        if (count_nand_devices()==1)
            cyg_nand_lookup(cyg_nanddevtab[0].devname, &nand);
        else
            diag_printf("More than one NAND device present, please specify with -d <device>\n");
    }
    if (!nand) return;

    switch(mode) {
        case dodefault:
            diag_printf("(You didn't specify a subcommand, so I'm defaulting to `summary.'\n `nand badblocks help' lists all known subcommands.)\n");
            // FALLTHROUGH
        case summary:
            do_summary(nand);
            return;
        case list:
            do_list(nand);
            return;
#ifdef CYGSEM_REDBOOT_NAND_BADBLOCKS_MARK_CMD
        case mark:
            if (markblock==-1 || markstate==-1) {
                nbb_mark_usage();
            } else
                do_mark(nand, markblock, markstate);
            return;
#endif
        case unset:
            diag_printf("can't happen at %s %d\n", __FILE__, __LINE__);
            return;
    }
}

#endif // CYGSEM_REDBOOT_NAND_BADBLOCKS_CMD

//==========================================================================
// nand erase - erases a nand partition. (Who'd have thunk?)

#ifdef CYGSEM_REDBOOT_NAND_ERASE_CMD

#ifdef CYGPKG_IO_FILEIO
__externC cyg_mtab_entry cyg_mtab[];
__externC cyg_mtab_entry cyg_mtab_end;

static cyg_mtab_entry * find_mounted_nand(const char *dev)
{
    cyg_mtab_entry *m;
    for( m = &cyg_mtab[0]; m != &cyg_mtab_end; m++ ) {
        if( m->name == NULL || !m->valid ) continue;
        if (0==strcmp(m->devname, dev)) {
            return m;
        }
    }
    return NULL;
}
#endif

static void nand_erase(int argc, char*argv[])
{
    char *part_str=0;
    int rv;

    if (!scan_opts(argc, argv, 1, NULL, 0, &part_str, OPTION_ARG_TYPE_STR, "NAND partition, e.g. mynand/0"))
        return;

    if (!part_str) {
        err_printf("nand erase: partition required\n");
        return;
    }

    cyg_nand_device *nand=0;
    cyg_nand_partition *part=0;
    cyg_nand_resolve_device(part_str, &nand, &part);
    if (!nand) {
        err_printf("nand erase: device not found\n");
        return;
    }
    if (!part) {
        err_printf("nand erase: partition not found\n");
        return;
    }

#ifdef CYGPKG_IO_FILEIO
    cyg_mtab_entry *mte = find_mounted_nand(part_str);
    if (mte) {
        err_printf("nand erase: device %s is mounted on %s, must unmount first\n", part_str, mte->name);
        return;
    }
#endif

    diag_printf("Erasing device %s blocks %u to %u...\n", nand->devname, part->first, part->last);
    cyg_nand_block_addr blk;
    // Compute a modulus to print out about 72 x `.' as we go by
    int progmod = (1 + CYG_NAND_PARTITION_NBLOCKS(part)) / 73 + 1;

    for (blk = part->first; blk <= part->last; blk++) {
        int st = cyg_nandp_bbt_query(part, blk);
        if (st<0) {
            diag_printf("Block %d BBTI error %d\n", blk, -st);
        }
        const char *msg = 0;
        switch(st) {
            case CYG_NAND_BBT_OK:
                rv = cyg_nandp_erase_block(part, blk);
                if (rv != 0)
                    diag_printf("Block %d: error %d\n", blk, -rv);
                break;
            case CYG_NAND_BBT_WORNBAD:
                msg="worn bad"; break;
            case CYG_NAND_BBT_FACTORY_BAD:
                msg="factory bad"; break;

            case CYG_NAND_BBT_RESERVED:
                // Skip quietly
                break;
        }
        if (msg)
            diag_printf("Skipping block %d (%s)\n", blk, msg);

        if (0==(blk % progmod))
            diag_printf(".");
    }
    diag_printf("\nErase complete.\n");
}

local_cmd_entry("erase", "Erases a NAND partition", "<partition> (e.g. mynand/0)", nand_erase, NAND_cmds);

#endif // CYGSEM_REDBOOT_NAND_ERASE_CMD

//==========================================================================

// end nand.c
