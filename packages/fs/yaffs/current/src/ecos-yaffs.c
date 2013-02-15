//=============================================================================
//
//      ecos-eyaffs.c
//
//      eyaffs interface to the eCos filesystem layer
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
// Date:        2009-05-05
//
//####DESCRIPTIONEND####
//=============================================================================

/* Major TODO:
 *   At the moment we tend to look files up by name(dirsearch), perform
 *   our own sanity checks, then pass on to YAFFS fns which then go and
 *   repeat the last step of the lookup. IWBNI we exposed corresponding
 *   YAFFS functions which took yaffs_Object* instead of name.          */

#include "yportenv.h"
#include "yaffs_guts.h"
#include "yaffs_packedtags2.h"
#include "ecos-yaffs.h"
#include "ecos-yaffs-nand.h"
#include "ecos-yaffs-diriter.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <cyg/fileio/fileio.h>
#include <cyg/fileio/dirent.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/nand/nand.h>
#include <cyg/nand/util.h>

// convenience error test macros. x should be an ecos error code.
#define ER(x) do { int _x = (x); if (_x != 0) return _x; } while(0)
#define EG(x) do { _err = (x); if (_err != 0) goto err_exit; } while(0)
// ... variants which take a negative ecos error code
#define ERm(x) ER(-(x))
#define EGm(x) EG(-(x))
// ... variants for YAFFS ops, which return YAFFS_OK or YAFFS_FAIL
#define YER(x) do { int _x = (x); if (_x == YAFFS_FAIL) return EIO; } while(0)

// sneaky grab from fileio misc.cxx:
__externC cyg_mtab_entry cyg_mtab[];
__externC cyg_mtab_entry cyg_mtab_end;

// For yaffs_guts:
unsigned int yaffs_traceMask = CYGNUM_FS_YAFFS_TRACEMASK;

// How many attempts do we make to write a page before giving up?
// This could easily be added to CDL.
unsigned int yaffs_wr_attempts = YAFFS_WR_ATTEMPTS;

// eCos doesn't support file permissions or user IDs at all.
// We hard-code sensible values in case anybody makes decisions on
// the output from stat() et al.
// (Update: As of March 2009 there is now limited support for chmod in both
// anoncvs and eCosPro.)
#define FIXED_UID 0
#define FIXED_GID 0
#define DIRECTORY_FIXED_PERMS (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define FILE_FIXED_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
// TODO: We could make uid/gid/fixedperms configurable.

// Helper functions and forward defs -------------------------------------
static int eyaffs_count_links(yaffs_Object * node)
{
    int links = 0;
    struct ylist_head *tmp;
    ylist_for_each(tmp, &(node->hardLinks)) {
        ++links;
    }
    return links;
}

static int eyaffs_stat_object(yaffs_Object *node, struct stat *buf);

// Directory search / traversal ------------------------------------------
typedef struct ey_dirsearch
{
    yaffs_Object        *dir;   // directory to search
    const char          *path;  // path to follow
    yaffs_Object        *node;  // Node found
    const char          *name;  // last name used
    int                 namelen;// name fragment length
    cyg_bool            last;   // last name in path?
} ey_dirsearch;

static void init_dirsearch(ey_dirsearch *ds, yaffs_Object *dir, const char *name)
{
    ds->dir = dir;
    ds->path = name;
    ds->node = dir;
    ds->name = name;
    ds->namelen = 0;
    ds->last = false;
}

// Search a single directory for the next name in a path
static int find_entry(ey_dirsearch *ds)
{
    yaffs_Object *dir = ds->dir;
    const char *name = ds->path;
    const char *n = name;
    char namelen = 0;

    yaffs_Object *d;

    // check that we really have a directory
    switch (yaffs_GetObjectType(dir)) {
        case DT_DIR: break;

        case DT_LNK: // TODO: symlink support
        default: return ENOTDIR;
    }

    // Isolate the next element of the path name. 
    while( *n != '\0' && *n != '/' )
        n++, namelen++;

    // Check if this is the last path element.
    while( *n == '/') n++;
    if( *n == '\0' )
        ds->last = true;

    // update name in dirsearch object
    ds->name = name;
    ds->namelen = namelen;

    // First, special case "." and "..":
    if (name[0] == '.') {
        if (namelen == 1) { // "."
            ds->node = dir;
            return ENOERR;
        } else if (name[1] == '.' && namelen == 2) { // ".."
            ds->node = dir->parent;
            if (!ds->node) ds->node = dir; // ".." at the root stays in the root
            return ENOERR;
        }
    }

    // Otherwise, search the directory for a matching entry
    YCHAR namebuf[YAFFS_MAX_NAME_LENGTH+1];
    memcpy(namebuf, name, namelen);
    namebuf[(int)namelen]=0;

    d = yaffs_FindObjectByName(dir, namebuf);
    // TODO: Create a variant of y_FindObjectByName which takes dir, name and len. Indeed, that would be beneficial for a lot of its functions.
    if( d == NULL ) return ENOENT;
    d = yaffs_GetEquivalentObject(d); // traverse hardlinks

    // pass back the node we have found
    ds->node = d;

    return ENOERR;
}

/* Main directory search code. Use init_dirsearch() to init the struct.
 * If we return with d->last set, d->name _might_ be null-terminated.
 * (It will iff no trailing slashes were passed in.)
 * Code that acts on the output name MUST use d->namelen and not assume
 * null-termination without testing first.
 */
static int eyaffs_find(ey_dirsearch *d)
{
    if ( *(d->path) == 0 )
        return ENOERR;
    for (;;) {
        ER(find_entry(d));
        if (d->last) return ENOERR;
        // Not done yet? Next one down...
        d->dir = d->node;
        d->path += d->namelen;
        // While we could check file perms here, eCos doesn't support them.
        while ( *(d->path) == '/' ) ++d->path; // skip separators
    }
}

// File operations --------------------------------------------------------

static int eyaffs_pathconf(yaffs_Object *node, struct cyg_pathconf_info *info)
{
    switch(info->name) {
        case _PC_LINK_MAX:
            info->value = LINK_MAX;
            return ENOERR;

        case _PC_NAME_MAX:
            info->value = YAFFS_MAX_NAME_LENGTH;
            return ENOERR;

        case _PC_PATH_MAX:
            info->value = PATH_MAX;
            return ENOERR;

        case _PC_PRIO_IO:
        case _PC_SYNC_IO:
            info->value=0;
            return ENOERR;

        case _PC_NO_TRUNC:
            info->value=1;
            return ENOERR;

        case _PC_MAX_CANON: // terminals only
        case _PC_MAX_INPUT: // terminals
        case _PC_PIPE_BUF:  // pipes
        case _PC_ASYNC_IO:  // not supported
        case _PC_CHOWN_RESTRICTED: // we don't track owners
        case _PC_VDISABLE: // terminals
        default:
            info->value = -1;
            return EINVAL;
    }
}

static int eyaffs_fo_read    (struct CYG_FILE_TAG *fp, struct CYG_UIO_TAG *uio)
{
    yaffs_Object *node = (yaffs_Object*) fp->f_data;
    int i;
    off_t pos = fp->f_offset;
    ssize_t resid = uio->uio_resid;

    for (i=0; i < uio->uio_iovcnt; i++) {
        cyg_iovec  *iov  = &uio->uio_iov[i];
        unsigned char *buf  = iov->iov_base;
        off_t       len  = iov->iov_len;

        if (pos + len > node->variant.fileVariant.fileSize)
            len = node->variant.fileVariant.fileSize - pos;

        while (len > 0 && pos < node->variant.fileVariant.fileSize) {
            off_t l = yaffs_ReadDataFromFile(node, buf, pos, len);
            len -= l;
            pos += l;
            buf += l;
            resid -= l;
        }
    }
    uio->uio_resid = resid;
    fp->f_offset = pos;
    return ENOERR;
}

static int eyaffs_fo_write   (struct CYG_FILE_TAG *fp, struct CYG_UIO_TAG *uio)
{
    yaffs_Object *node = (yaffs_Object*) fp->f_data;
    int i;
    off_t pos = fp->f_offset;
    ssize_t resid = uio->uio_resid;

    if (fp->f_flag & CYG_FAPPEND)
        pos = fp->f_offset = node->variant.fileVariant.fileSize;

    CYG_ASSERT(pos >= 0, "pos became negative");

    for (i=0; i < uio->uio_iovcnt; i++) {
        cyg_iovec  *iov  = &uio->uio_iov[i];
        unsigned char *buf  = iov->iov_base;
        off_t       len  = iov->iov_len;

        while (len > 0) {
            off_t l = yaffs_WriteDataToFile(node, buf, pos, len, 0);
            len -= l;
            buf += l;
            pos += l;
            resid -= l;

            if (l==0) goto stop; // urgh, how does this cope with dire errors?
        }
    }

    // Timestamps are updated on yaffs_FlushFile()
stop:
    uio->uio_resid = resid;
    fp->f_offset = pos;
    return ENOERR;
}


static int eyaffs_fo_lseek   (struct CYG_FILE_TAG *fp, off_t *apos, int whence )
{
    yaffs_Object *node = (yaffs_Object*) fp->f_data;
    off_t pos = *apos;

    switch (whence)
    {
        case SEEK_SET:
            // Pos is already where we want to be
            break;
         case SEEK_CUR:
            // Add pos to current offset
            pos += fp->f_offset;
            break;
         case SEEK_END:
            // Add pos to file size
            pos += node->variant.fileVariant.fileSize;
            break;
         default:
            return EINVAL;
    }

    // Check that pos is still within current file size, 
    // or at the very end
    if (pos < 0 || pos > node->variant.fileVariant.fileSize)
        return EINVAL;
  
    *apos = fp->f_offset = pos;
    return ENOERR;
}

static int eyaffs_fo_ioctl   (struct CYG_FILE_TAG *fp, CYG_ADDRWORD com,
                              CYG_ADDRWORD data)
{
    return EINVAL; // No ioctls defined
}

static int eyaffs_fo_fsync   (struct CYG_FILE_TAG *fp, int mode )
{
    yaffs_Object *node = (yaffs_Object*) fp->f_data;
    int ymode = (mode == CYG_FDATASYNC) ? 1 : 0;
    YER(yaffs_FlushFile(node,1,ymode));
    return ENOERR;
}

static int eyaffs_fo_close   (struct CYG_FILE_TAG *fp)
{
    yaffs_Object *node = (yaffs_Object*) fp->f_data;

    if (node->inUse) {
        YER(yaffs_FlushFile(node,1,0));
        node->inUse--;
        if (node->inUse <= 0 && node->unlinked) {
            YER(yaffs_DeleteObject(node));
        }
    }
    fp->f_data = 0;
    return ENOERR;
}

static int eyaffs_fo_fstat   (struct CYG_FILE_TAG *fp, struct stat *buf )
{
    yaffs_Object *node = (yaffs_Object*) fp->f_data;
    
    return eyaffs_stat_object(node, buf);
}

static int eyaffs_fo_getinfo (struct CYG_FILE_TAG *fp, int key, void *buf, int len )
{
    yaffs_Object *node = (yaffs_Object*) fp->f_data;

    switch(key) {
        case FS_INFO_CONF:
            return eyaffs_pathconf(node, (struct cyg_pathconf_info *)buf );
    }
    return EINVAL;
}
static int eyaffs_fo_setinfo (struct CYG_FILE_TAG *fp, int key, void *buf, int len )
{
    return EINVAL; // None defined
}

static cyg_fileops eyaffs_fileops = 
{
    eyaffs_fo_read, eyaffs_fo_write, 
    eyaffs_fo_lseek, eyaffs_fo_ioctl, 
    cyg_fileio_seltrue,
    eyaffs_fo_fsync, eyaffs_fo_close, 
    eyaffs_fo_fstat, 
    eyaffs_fo_getinfo, eyaffs_fo_setinfo
};

// Directory entrypoints --------------

static int eyaffs_fo_dirread (struct CYG_FILE_TAG *fp, struct CYG_UIO_TAG *uio)
{
    eyaffs_diriter *diriter = (eyaffs_diriter*) fp->f_data;
    struct dirent *ent = (struct dirent *)uio->uio_iov[0].iov_base;
    unsigned offset = fp->f_offset;
    off_t len = uio->uio_iov[0].iov_len;

    if( len < sizeof(struct dirent) )
        return EINVAL;    

    if (offset == 0) {
        strcpy(ent->d_name, ".");
#ifdef CYGPKG_FS_YAFFS_RET_DIRENT_DTYPE
        ent->d_type = __stat_mode_DIR;
#endif
        goto done;
    }
    if (offset == 1) {
        strcpy(ent->d_name, "..");
#ifdef CYGPKG_FS_YAFFS_RET_DIRENT_DTYPE
        ent->d_type = __stat_mode_DIR;
#endif
        goto done;
    }

    // else use the iter:
    yaffs_Object * node = diriter->nextret;

    if (!node) goto done_eof;

    yaffs_GetObjectName(node, ent->d_name, sizeof(ent->d_name));
#ifdef CYGPKG_FS_YAFFS_RET_DIRENT_DTYPE
    ent->d_type = yaffs_GetObjectType(node);
#endif
    eyaffs_diriter_advance(diriter);

done:
    uio->uio_resid         -= sizeof(struct dirent);
    fp->f_offset++;
done_eof:
    return ENOERR;
}

static int eyaffs_fo_dirlseek (struct CYG_FILE_TAG *fp, off_t *pos, int whence )
{
    // This is easy, we only allow a full rewind.
    if (whence != SEEK_SET || *pos != 0)
        return EINVAL;
    // get and rewind the diriter
    eyaffs_diriter *diriter = (eyaffs_diriter*) fp->f_data;
    eyaffs_diriter_rewind(diriter);
    fp->f_offset = 0;
    return ENOERR;
}

static int eyaffs_fo_dirclose (struct CYG_FILE_TAG *fp)
{
    eyaffs_diriter *diriter = (eyaffs_diriter*) fp->f_data;

    diriter->node->inUse--;
    ylist_del(&diriter->iterlist);
    memset(diriter, 0xbb, sizeof(eyaffs_diriter));
    fp->f_data = 0;
    YFREE(diriter);
    return ENOERR;
}


static cyg_fileops eyaffs_dirops = 
{
    eyaffs_fo_dirread, 
    (cyg_fileop_write *)cyg_fileio_enosys,
    eyaffs_fo_dirlseek,
    (cyg_fileop_ioctl *)cyg_fileio_enosys,
    cyg_fileio_seltrue,
    (cyg_fileop_fsync *)cyg_fileio_enosys,
    eyaffs_fo_dirclose,
    (cyg_fileop_fstat *)cyg_fileio_enosys,
    (cyg_fileop_getinfo *)cyg_fileio_enosys,
    (cyg_fileop_setinfo *)cyg_fileio_enosys
};

// Filesystem operations -------------------------------------------------
// N.B. Function names are prefixed with 'e' to distinguish them from
//      those internal to yaffs.


static int eyaffs_mount    ( cyg_fstab_entry *fste, cyg_mtab_entry *mte )
{
    int _err = ENOERR;

    cyg_nand_device *nand = 0;
    cyg_nand_partition *part = 0;
    ERm(cyg_nand_resolve_device(mte->devname, &nand, &part));

    if (!part || !part->dev || (part->last <= part->first) ) {
        TOUT(("eyaffs_mount: Invalid partition\n"));
        ER(EINVAL);
    }

    // Is this partition already mounted?
    cyg_mtab_entry *m = cyg_mtab;
    while (m != &cyg_mtab_end) {
        if (m->valid
                && (0==strcmp(m->fsname,"yaffs"))
                && (((yaffs_Device *)m->data)->genericDevice == part) ) {
            ER(EBUSY);
            /* TODO: Set up a refcount to allow a yaffs/nand fs
             * to be mounted multiple times. In that case,
             * re-use the existing yaffs device struct. */
        }
        m++;
    }

    // Option handling. Defaults come from CDL.
    int o_nReserved = CYGNUM_FS_YAFFS_RESERVED_BLOCKS;
    int o_nShortOpCaches = CYGNUM_FS_YAFFS_SHORTOP_CACHES;
    int o_skipCheckpointRead = 0;

#if defined(CYGSEM_FS_YAFFS_COMPATIBILITY_ECOSPRO) && (CYGSEM_FS_YAFFS_COMPATIBILITY_ECOSPRO == 1)
    if (mte->options) {
        int t;
        char buf[10];
        // reserved=<int>: Number of blocks to reserve (minimum 2)
        if (0==cyg_fs_get_option(mte->options, "reserved", buf, sizeof buf)) {
            t = atoi(buf);
            if (t>=2 && t <= (part->last - part->first))
                o_nReserved = t;
            else
                TOUT(("eyaffs_mount: ignoring invalid value `reserved=%d'", t));
        }
        // caches=<int>: Number of cache entries to use (10-20 recommended)
        if (0==cyg_fs_get_option(mte->options, "caches", buf, sizeof buf)) {
            t = atoi(buf);
            if (t>=0)
                o_nShortOpCaches = t;
            else
                TOUT(("eyaffs_mount: ignoring invalid value `caches=%d'", t));
        }
        // skip-checkpoint-read: Ignore the checkpoint restore and instead
        // do a full filesystem scan.
        if (0==cyg_fs_get_option(mte->options, "skip-checkpoint-read", buf, sizeof buf)) {
            o_skipCheckpointRead = 1;
        }
        // TODO: readonly option, runtime and/or CDL?
    }
#endif

#ifdef CYGPKG_INFRA_DEBUG
    // Best-efforts sanity check that there's enough heap.
    // The rule of thumb is 2 bytes per page.
    struct mallinfo mi = mallinfo();
    unsigned want = ( 1 << (nand->block_page_bits + 1) ) * (1 + part->last - part->first);
    if ( mi.fordblks < want) {
        TOUT(("eyaffs_mount: warning: arena reports %u bytes free; %u recommended for this device\n", mi.fordblks, want));
    }
#endif // CYGSEM_FS_YAFFS_COMPATIBILITY

    yaffs_Device *y = YMALLOC(sizeof(yaffs_Device)); // ^^^^^^^^^^^^^^^^
    if (!y) EG(ENOMEM);
    memset(y, 0, sizeof(yaffs_Device));
    y->name = mte->devname;
    y->genericDevice = (void*) part;

    y->nDataBytesPerChunk =
        y->totalBytesPerChunk = CYG_NAND_BYTES_PER_PAGE(nand);
    y->inbandTags = 0;

    y->nChunksPerBlock = CYG_NAND_PAGES_PER_BLOCK(nand);
    y->nReservedBlocks = o_nReserved;
    y->startBlock = 0;
    y->endBlock = CYG_NAND_PARTITION_NBLOCKS(part)-1;
    y->useNANDECC = 1;
    y->nShortOpCaches = o_nShortOpCaches;

    if (CYG_NAND_BYTES_PER_PAGE(nand) == 512) {
#ifdef CYGSEM_FS_YAFFS_SMALLPAGE_MODE_YAFFS1
        y->isYaffs2 = 0;
        if (CYG_NAND_APPSPARE_PER_PAGE(nand) < PACKEDTAGS1_OOBSIZE) {
            NAND_ERROR(nand,
                    "Device has %d spare per page but YAFFS1 needs %d\n",
                    CYG_NAND_APPSPARE_PER_PAGE(nand), PACKEDTAGS1_OOBSIZE);
            EG(EINVAL);
        }
#else // CYGSEM_FS_YAFFS_SMALLPAGE_MODE_YAFFS2
        y->isYaffs2 = 1;
        // 512-byte page devices normally have 8-byte spare areas.
        // This isn't big enough for YAFFS2 tags, which need 25.
        y->inbandTags = 1;
#endif
    } else {
        y->isYaffs2 = 1;
        if (CYG_NAND_APPSPARE_PER_PAGE(nand) < PACKEDTAGS2_OOBSIZE(y)) {
            NAND_ERROR(nand,
                    "Device has %d spare per page but YAFFS2 needs %u\n",
                    CYG_NAND_APPSPARE_PER_PAGE(nand), PACKEDTAGS2_OOBSIZE(y));
            EG(EINVAL);
        }
    }

#ifdef CYGSEM_FS_YAFFS_OMIT_YAFFS2_CODE
    if (y->isYaffs2) {
        NAND_ERROR(nand, "Device is set to run with yaffs2 code, which is compiled out.\n");
        EG(ENODEV);
    }
#endif

    y->eraseBlockInNAND = eyaffs_eraseBlockInNAND;
    y->initialiseNAND = eyaffs_initialiseNAND;
    y->deinitialiseNAND = eyaffs_deinitialiseNAND;

    y->writeChunkWithTagsToNAND = eyaffs_writeChunkWithTagsToNAND;
    y->readChunkWithTagsFromNAND = eyaffs_readChunkWithTagsFromNAND;
    y->markNANDBlockBad = eyaffs_markNANDBlockBad;
    y->queryNANDBlock = eyaffs_queryNANDBlock;

    y->isMounted = 0;
    y->removeObjectCallback = eyaffs_diriter_objremove_callback;

    if (o_skipCheckpointRead) {
        NAND_CHATTER(1, nand, "mounting with skip-checkpoint-read\n");
        y->skipCheckpointRead = 1;
    }

    int yerr = yaffs_GutsInitialise(y);
    if (yerr == YAFFS_FAIL) {
        NAND_CHATTER(1, nand, "yaffs_GutsInitialise failed!\n");
        EG(EIO);
    }

    yaffs_Object * root = yaffs_Root(y);
    mte->root = (cyg_dir) root;
    mte->data = (CYG_ADDRWORD) y;
    CYG_ASSERT(_err == ENOERR, "Unhandled error in initialisation");
    // At this point we could use the device struct's devList to link it
    // into a global devList. (We don't currently need to, so won't.)
    return ENOERR;
err_exit:
    if (y) YFREE(y);
    return _err;
}

#if defined(CYGSEM_FS_YAFFS_COMPATIBILITY_ECOSPRO) && (CYGSEM_FS_YAFFS_COMPATIBILITY_ECOSPRO == 1)
static int eyaffs_umount   ( cyg_mtab_entry *mte, cyg_bool force )
#else
static int eyaffs_umount   ( cyg_mtab_entry *mte )
#endif
{
    yaffs_Device *y = (yaffs_Device*) mte->data;
    
    yaffs_FlushEntireDeviceCache(y);
    yaffs_CheckpointSave(y);
    yaffs_Deinitialise(y);

    YFREE(y);
    return ENOERR;
}

static int eyaffs_open   ( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                             int mode,  cyg_file *file )
{
    int _err;
    ey_dirsearch ds;
    yaffs_Object *node = 0;

    // 'mode' is the open options (flags), not the permissions.
    // (eCos doesn't support file perms.)

    // fileio gives us either the root, or the current wd, to start at
    init_dirsearch(&ds, (yaffs_Object*) dir, name);
    _err = eyaffs_find(&ds);

    if (_err == ENOENT) {
        // It's not there. Are we creat()ing?
        if (ds.last && (mode & O_CREAT) ) {
            // A trailing slash implies directory, which makes no sense here
            if (ds.name[ds.namelen] == '/') return EISDIR;
            CYG_ASSERT(ds.name[ds.namelen]==0, "Length fencepost failed");
            node = yaffs_MknodFile(ds.dir, ds.name, FILE_FIXED_PERMS, FIXED_UID, FIXED_GID);
            if (!node || (node==YAFFS_FAIL)) return ENOMEM;
            _err = ENOERR;
        }
        // else we return ENOENT
    } else if (_err == ENOERR) {
        // The node exists. If the O_CREAT and O_EXCL bits are set, we
        // must fail the open.
        if( (mode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL) )
            _err = EEXIST;
        else node = ds.node;
    }

    if( _err != ENOERR ) return _err;

    if (yaffs_GetObjectType(node) == DT_LNK) {
        // TODO: symlink support
        return ENOSYS;
    }

    if(yaffs_GetObjectType(node) == DT_DIR) return EISDIR;

    // Could check perms here (and on the dir tree too)

    if( mode & O_TRUNC )
        YER(yaffs_ResizeFile(node, 0));

    node->inUse++;

    file->f_flag |= mode & CYG_FILE_MODE_MASK;
    file->f_type = CYG_FILE_TYPE_FILE;
    file->f_ops = &eyaffs_fileops;
    file->f_offset = (mode & O_APPEND) ?  yaffs_GetObjectFileLength(node) : 0;
    file->f_data = (CYG_ADDRWORD) node;
    file->f_xops = 0;

    return ENOERR;
}

static int eyaffs_unlink ( cyg_mtab_entry *mte, cyg_dir dir, const char *name )
{
    int _err;
    ey_dirsearch ds;

    init_dirsearch(&ds, (yaffs_Object*) dir, name);
    _err = eyaffs_find(&ds);
    if (_err != ENOERR) return _err;

    if(yaffs_GetObjectType(ds.node) == DT_DIR) return EISDIR;
    if (ds.name[ds.namelen] == '/') return EISDIR; // cannot unlink dirs
    YER(yaffs_Unlink(ds.dir,ds.name)); // this appears to DTRT wrt 0-linked still-open files
    return ENOERR;
}

static int eyaffs_mkdir  ( cyg_mtab_entry *mte, cyg_dir dir, const char *name )
{
    ey_dirsearch ds;

    init_dirsearch(&ds, (yaffs_Object*) dir, name);
    int _err = eyaffs_find(&ds);
    if (_err == ENOENT) {
        if (ds.last) {
            char dirname[ds.namelen+1];
            memcpy(dirname, ds.name, ds.namelen);
            dirname[ds.namelen] = 0;
            // OK, create it!
            yaffs_Object * node = yaffs_MknodDirectory(ds.dir, dirname, DIRECTORY_FIXED_PERMS, FIXED_UID, FIXED_GID);
            _err = (node == NULL) ? EIO : ENOERR;
        } else {
            // Intermediate directory doesn't exist - ENOENT
        }
    } else {
        // something there already
        if (_err == ENOERR) _err = EEXIST;
    }
    return _err;
}

static int eyaffs_rmdir  ( cyg_mtab_entry *mte, cyg_dir dir, const char *name )
{
    ey_dirsearch ds;

    init_dirsearch(&ds, (yaffs_Object*) dir, name);
    int _err = eyaffs_find(&ds);
    if (_err != ENOERR) return _err;

    if(yaffs_GetObjectType(ds.node) != DT_DIR) return ENOTDIR;
    if (0==strcmp(ds.name,".")) return EINVAL;
    if (0==ds.namelen) return EINVAL;

    char dirname[ds.namelen+1];
    memcpy(dirname, ds.name, ds.namelen);
    dirname[ds.namelen] = 0;

    if (YAFFS_FAIL == yaffs_Unlink(ds.dir,dirname)) return ENOTEMPTY;
    return ENOERR;
}

static int eyaffs_rename ( cyg_mtab_entry *mte, cyg_dir dir1, const char *name1,
                           cyg_dir dir2, const char *name2 )
{
    ey_dirsearch ds1, ds2;

    init_dirsearch(&ds1, (yaffs_Object*) dir1, name1);
    int _err = eyaffs_find(&ds1);
    if (_err != ENOERR) return _err;

    init_dirsearch(&ds2, (yaffs_Object*) dir2, name2);
    _err = eyaffs_find(&ds2);

    if (ds2.last && _err == ENOENT)
        ds2.node = NULL, _err = ENOERR; // through-rename

    if (_err != ENOERR) return _err;

    if (ds1.node == ds2.node) return ENOERR; // identity op

    if (ds2.node) {
        int type1 = yaffs_GetObjectType(ds1.node),
            type2 = yaffs_GetObjectType(ds2.node);
        if ( (type1 != DT_DIR) && (type2 == DT_DIR) ) 
            return EISDIR;
        if ( (type1 == DT_DIR) && (type2 != DT_DIR) ) 
            return ENOTDIR;
        // YAFFS will delete the target
    }
    // Alas, if this is a directory rename, there may be trailing / involved, 
    // so we have to strip them.
    char xname1[ds1.namelen+1];
    memcpy(xname1, ds1.name, ds1.namelen);
    xname1[ds1.namelen] = 0;
    char xname2[ds2.namelen+1];
    memcpy(xname2, ds2.name, ds2.namelen);
    xname2[ds2.namelen] = 0;

    YER(yaffs_RenameObject(ds1.dir, xname1, ds2.dir, xname2));
    return ENOERR;
}

static int eyaffs_link   ( cyg_mtab_entry *mte, cyg_dir dir1, const char *name1,
                           cyg_dir dir2, const char *name2, int type )
{
    ey_dirsearch ds1, ds2;

    // Only do hard links for now
    if (type != CYG_FSLINK_HARD) return EINVAL;

    init_dirsearch(&ds1, (yaffs_Object*) dir1, name1);
    int _err = eyaffs_find(&ds1);
    if (_err != ENOERR) return _err;

    init_dirsearch(&ds2, (yaffs_Object*) dir2, name2);
    _err = eyaffs_find(&ds2);
    if (_err == ENOERR) return EEXIST; // Can't rename-over

    if (ds2.last && _err == ENOENT)
        ds2.node = 0, _err = ENOERR; // link-through

    ER(_err);

    // Forbid hard links to directories
    if (ds1.node->variantType == YAFFS_OBJECT_TYPE_DIRECTORY)
        return EPERM;

    if (ds2.name[ds2.namelen] == '/') return EISDIR; // eh? makes no sense.
    yaffs_Object * newnode = yaffs_Link(ds2.dir, ds2.name, ds1.node);
    if (newnode) return ENOERR;
    else return EIO;
}

static int eyaffs_opendir( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                           cyg_file *file )
{
    int _err;
    ey_dirsearch ds;

    init_dirsearch(&ds, (yaffs_Object*) dir, name);
    _err = eyaffs_find(&ds);
    if (_err != ENOERR) return _err;

    if(yaffs_GetObjectType(ds.node) != DT_DIR) return ENOTDIR;

    eyaffs_diriter *diriter = YMALLOC(sizeof(eyaffs_diriter));
    if (!diriter) return ENOMEM;

    ds.node->inUse++;

    YINIT_LIST_HEAD(&diriter->iterlist);
    diriter->node = ds.node;

    if (!eyaffs_all_diriters.next) YINIT_LIST_HEAD(&eyaffs_all_diriters);
    ylist_add(&diriter->iterlist, &eyaffs_all_diriters);
    eyaffs_diriter_rewind(diriter);

    file->f_type = CYG_FILE_TYPE_FILE;
    file->f_ops = &eyaffs_dirops;
    file->f_offset = 0;
    file->f_data = (CYG_ADDRWORD) diriter;
    file->f_xops = 0;
    return ENOERR;
}

static int eyaffs_chdir  ( cyg_mtab_entry *mte, cyg_dir xdir, const char *name,
                           cyg_dir *dir_out )
{
    yaffs_Object *dir = (yaffs_Object*) xdir;
    if (dir_out) {
        ey_dirsearch ds;
        init_dirsearch(&ds, dir, name);
        int _err = eyaffs_find(&ds);
        if (_err != ENOERR) return _err;
        if (yaffs_GetObjectType(ds.node) != DT_DIR)
            return ENOTDIR;
        *dir_out = (cyg_dir)ds.node;
    } else {
        // the "dec-refcount" case: nothing to do
    }
    return ENOERR;
}

static int eyaffs_stat   ( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                           struct stat *buf)
{
    int _err;
    ey_dirsearch ds;

    init_dirsearch(&ds, (yaffs_Object*) dir, name);
    _err = eyaffs_find(&ds);
    if (_err != ENOERR) return _err;

    return eyaffs_stat_object(ds.node, buf);
}

static int eyaffs_stat_object(yaffs_Object *node, struct stat *buf)
{
    switch (yaffs_GetObjectType(node)) {
        case DT_REG:
            buf->st_mode = __stat_mode_REG;
            buf->st_size = node->variant.fileVariant.fileSize;
            break;
        case DT_DIR:
            buf->st_mode = __stat_mode_DIR;
            buf->st_size = 0;
            break;
        default:
            return ENOSYS; // Don't support anything else yet
    }

    buf->st_mode |= node->yst_mode &~ S_IFMT;

    buf->st_ino         = node->objectId;
    buf->st_dev         = 0;
    buf->st_nlink       = eyaffs_count_links(node);
    buf->st_uid         = node->yst_uid;
    buf->st_gid         = node->yst_gid;

    buf->st_atime       = node->yst_atime;
    buf->st_mtime       = node->yst_mtime;
    buf->st_ctime       = node->yst_ctime;

    return ENOERR;
}

static int eyaffs_getinfo( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                           int key, void *buf, int len )
{
    int _err;
    ey_dirsearch ds;
    yaffs_Device *y = (yaffs_Device*) mte->data;

    switch(key) {
        case FS_INFO_CONF:
            break;

#ifdef CYGSEM_FILEIO_INFO_DISK_USAGE
        case FS_INFO_DISK_USAGE:
            {
                struct cyg_fs_disk_usage *usage = (struct cyg_fs_disk_usage *) buf;
                CYG_CHECK_DATA_PTR(buf, "getinfo buf");
                if (len < sizeof(struct cyg_fs_disk_usage)) return EINVAL;
                usage->total_blocks = y->endBlock - y->startBlock + 1;
                usage->free_blocks = y->nErasedBlocks;
                usage->block_size = y->nDataBytesPerChunk * y->nChunksPerBlock;
                return ENOERR;
            }
#else
            (void)y;
#endif

        case FS_INFO_GETCWD:
            // yaffs doesn't provide anything special, we'd only end up duplicating the fallback ".." method.
        case FS_INFO_ATTRIB: // not supported
#ifdef FS_INFO_CALLBACK
        case FS_INFO_CALLBACK: // not supported
#endif
        default:
            return ENOSYS;
    }

    init_dirsearch(&ds, (yaffs_Object*) dir, name);
    _err = eyaffs_find(&ds);
    if (_err != ENOERR) return _err;

    switch(key) {
        case FS_INFO_CONF:
            if (len < sizeof (struct cyg_pathconf_info)) return EINVAL;
            return eyaffs_pathconf(ds.node, (struct cyg_pathconf_info *) buf);

#ifdef FS_INFO_MBCS_TRANSLATE
        case FS_INFO_MBCS_TRANSLATE:
#endif
        default:
            return ENOSYS;
    }
}

static int eyaffs_do_sync_fs(yaffs_Device * y)
{
    yaffs_FlushEntireDeviceCache(y);
    // TODO: If there were a convenient way to enumerate all the open
    // files in a filesystem, we could fsync them all here.
    // This might involve code similar to cyg_fd_filesys_close().
    yaffs_CheckpointSave(y);
    return ENOERR;
}

static int eyaffs_setinfo( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                           int key, void *buf, int len )
{
    switch (key) {
        case FS_INFO_SYNC:
            return eyaffs_do_sync_fs( (yaffs_Device*) mte->data );
        case FS_INFO_ATTRIB: // not supported
#ifdef FS_INFO_CALLBACK
        case FS_INFO_CALLBACK: // not supported
#endif
#ifdef FS_INFO_SECURE_ERASE
        case FS_INFO_SECURE_ERASE:
#endif
#ifdef FS_INFO_MBCS_TRANSLATE
        case FS_INFO_MBCS_TRANSLATE:
#endif
            return ENOSYS;
    }
    return EINVAL;
}

FSTAB_ENTRY(eyaffs_fste, "yaffs", 0, CYG_SYNCMODE_FILE_FILESYSTEM|CYG_SYNCMODE_IO_FILESYSTEM, 
        /* N.B. If you get a build failure here (initialization
           from incompatible pointer type), it might be because you're
           using this GPL-licensed package on eCosPro and need to set
           CYGSEM_FS_YAFFS_COMPATIBILITY_ECOSPRO.
         */
        eyaffs_mount,
        eyaffs_umount,
        eyaffs_open,
        eyaffs_unlink,
        eyaffs_mkdir,
        eyaffs_rmdir,
        eyaffs_rename,
        eyaffs_link,
        eyaffs_opendir,
        eyaffs_chdir,
        eyaffs_stat,
        eyaffs_getinfo,
        eyaffs_setinfo);

// -----------------------------------------------------------------------
