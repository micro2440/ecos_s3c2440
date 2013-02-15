/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2007 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//=============================================================================
//
//      ecos-yaffs-diriter.c
//
//      Directory Iterator support for the eCos-YAFFS adaptation layer
//
//=============================================================================
//
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   wry
// Date:        2009-05-07
// Note:        This file is a derivative work of direct/yaffsfs.c and so
//              should be taken as inheriting its license terms - hence the
//              copyright banner above.
//
//####DESCRIPTIONEND####
//=============================================================================

#include "ecos-yaffs-diriter.h"
#include <cyg/infra/cyg_ass.h>

// Directory operations --------------------------------------------------

struct ylist_head eyaffs_all_diriters;

void eyaffs_diriter_advance(eyaffs_diriter *ctx)
{
    CYG_ASSERT(ctx && ctx->node && ctx->node->variantType == YAFFS_OBJECT_TYPE_DIRECTORY, "node not a directory");
    if ( !ctx->nextret ) return; // already finished, nothing to do

    struct ylist_head * top = &ctx->node->variant.directoryVariant.children;

    if (ylist_empty(top)) {
        ctx->nextret = NULL;
        return;
    }

    struct ylist_head *next = ctx->nextret->siblings.next;

    if( next == top ) // finished
        ctx->nextret = NULL;
    else
        ctx->nextret = ylist_entry(next,yaffs_Object,siblings); // works backwards from 'next' to its containing y_Object
}

void eyaffs_diriter_rewind(eyaffs_diriter *ctx)
{
    CYG_ASSERT(ctx && ctx->node && ctx->node->variantType == YAFFS_OBJECT_TYPE_DIRECTORY, "node not a directory");
    
    struct ylist_head * top = &ctx->node->variant.directoryVariant.children;
    if (ylist_empty(top))
        ctx->nextret = NULL;
    else
        ctx->nextret = ylist_entry(top->next, yaffs_Object, siblings);
}

void eyaffs_diriter_objremove_callback(yaffs_Object *obj)
{
    struct ylist_head *i;
    eyaffs_diriter *cur;

    if(!eyaffs_all_diriters.next) return; // nothing to do

    ylist_for_each(i, &eyaffs_all_diriters) { // foreach ($i) @eyaffs_all_diriters
        if (i) {
            // 'i' points to the 'iterlist' member of an eyaffs_diriter, sort out cur to point to it
            cur = ylist_entry(i, eyaffs_diriter, iterlist);

            if(cur->nextret == obj) // then fix it up
                eyaffs_diriter_advance(cur);
        }
    }
}

