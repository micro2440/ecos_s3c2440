//=============================================================================
//
//      ecos-yaffs-nand.c
//
//      Glue for YAFFS to the eCos NAND library
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
// Date:        2009-05-07
//
//####DESCRIPTIONEND####
//=============================================================================

#include "ecos-yaffs.h"
#include "ecos-yaffs-nand.h"
#include "yaffs_packedtags1.h"
#include "yaffs_packedtags2.h"
#include "yaffs_tagscompat.h"
#include <cyg/nand/util.h> // for BLOCK2PAGEADDR

int eyaffs_initialiseNAND (struct yaffs_DeviceStruct * dev)
{
    // Nothing to do - we've already inited it in mount().
    //cyg_nand_partition *part = (cyg_nand_partition*) dev->genericDevice;
    return YAFFS_OK;
}

int eyaffs_deinitialiseNAND (struct yaffs_DeviceStruct * dev)
{
    // Nothing to do
    //cyg_nand_partition *part = (cyg_nand_partition*) dev->genericDevice;
    return YAFFS_OK;
}

int eyaffs_eraseBlockInNAND (struct yaffs_DeviceStruct * dev, int blockInNAND)
{
    cyg_nand_partition *part = (cyg_nand_partition*) dev->genericDevice;
    int rv = cyg_nandp_erase_block(part, blockInNAND);
    return rv==0 ? YAFFS_OK : YAFFS_FAIL;
}

int eyaffs_writeChunkWithTagsToNAND (struct yaffs_DeviceStruct * dev,
        int chunkInNAND, const __u8 * data,
        const yaffs_ExtendedTags * tags)
{
    cyg_nand_partition *part = (cyg_nand_partition*) dev->genericDevice;
    int rv;

    if (!dev->isYaffs2) {
#ifdef CONFIG_YAFFS_NO_YAFFS1
        rv = 1;
#else
        yaffs_PackedTags1 pt1;

        if (tags->chunkDeleted) {
            memset(&pt1, 0xff, sizeof(pt1));
            pt1.deleted = 0;
            //((CYG_BYTE*)&pt1)[8] = 0; // TODO: support 9BYTE_TAGS, if there is a demand to (requires corresponding fixup in the ECC check below)
        } else {
            yaffs_PackTags1(&pt1, tags);
            yaffs_CalcTagsECC((yaffs_Tags *)&pt1);
        }
        rv = cyg_nandp_write_page(part, chunkInNAND, data, &pt1, PACKEDTAGS1_OOBSIZE);
#endif
    } else {
#ifdef CYGSEM_FS_YAFFS_OMIT_YAFFS2_CODE
        return YAFFS_FAIL;
#else
        // YAFFS2 mode.
        // N.B. We are not doing ECC - the NAND layer does that - so don't
        // need to bother with the whole tags struct, just the bit that matters.

        if (!data || !tags) {
            T(YAFFS_TRACE_ERROR,
                (TSTR("**>> yaffs2: both data and tags must be present" TENDSTR)));
            YBUG();
            return YAFFS_FAIL;
        }

        if (dev->inbandTags) {
            yaffs_PackedTags2TagsPart *pt2tp;
            pt2tp = (yaffs_PackedTags2TagsPart *)(data + dev->nDataBytesPerChunk);
            yaffs_PackTags2TagsPart(pt2tp, tags);
            rv = cyg_nandp_write_page(part, chunkInNAND, data, 0, 0);
        } else {
            yaffs_PackedTags2 pt;
            void *packed_tags_ptr = dev->noTagsECC ? (void*) &pt.t : (void*) &pt;
            yaffs_PackTags2(dev, &pt, tags);
            rv = cyg_nandp_write_page(part, chunkInNAND, data, packed_tags_ptr, PACKEDTAGS2_OOBSIZE(dev));
        }
#endif
    }

    // all done:
    return rv==0 ? YAFFS_OK : YAFFS_FAIL;
}

int eyaffs_readChunkWithTagsFromNAND (struct yaffs_DeviceStruct * dev,
        int chunkInNAND, __u8 * data,
        yaffs_ExtendedTags * tags)
{
    cyg_nand_partition *part = (cyg_nand_partition*) dev->genericDevice;
    int localData = 0;
    int rv;

    if (!dev->isYaffs2) {
#ifdef CONFIG_YAFFS_NO_YAFFS1
        rv = 1;
#else
        yaffs_PackedTags1 pt1;
        memset(&pt1, 0xff, sizeof(yaffs_PackedTags1));

        rv = cyg_nandp_read_page(part, chunkInNAND, data, &pt1, PACKEDTAGS1_OOBSIZE);

        if (rv == 0) {
            if (yaffs_CheckFF((CYG_BYTE*)&pt1, 8)) {
                // block is empty
                if (tags) {
                    memset(tags, 0, sizeof(*tags));
                    tags->eccResult = YAFFS_ECC_RESULT_NO_ERROR;
                }
            } else {
                // must check and correct tags' mini-ecc.
                int deleted = !pt1.deleted; // this would break the tags ECC.
                pt1.deleted = 1;
                // TODO: support 9BYTE_TAGS if there is a demand.
                int rv2 = yaffs_CheckECCOnTags((yaffs_Tags *)&pt1);
                if (rv2 < 0) {
                    T(YAFFS_TRACE_ERROR,
                            (TSTR ("**>>yaffs ecc error unfixed on chunk %d:0"
                                   TENDSTR), chunkInNAND));
                    dev->eccUnfixed++;
                    rv = EIO;
                } else {
                    if (tags) {
                        yaffs_UnpackTags1(tags, &pt1);
                        tags->chunkDeleted = deleted;
                        tags->eccResult = YAFFS_ECC_RESULT_NO_ERROR;
                    }

                    if (rv2 > 0) {
                        T(YAFFS_TRACE_ERROR,
                                (TSTR ("**>>yaffs ecc tags error fix performed on chunk %d:0"
                                       TENDSTR), chunkInNAND));
                        dev->eccFixed++;
                        if (tags)
                            tags->eccResult = YAFFS_ECC_RESULT_FIXED;
                    }
                }
            }
        } // else reading the NAND failed; drop out
#endif

    } else { // YAFFS2 mode
#ifdef CYGSEM_FS_YAFFS_OMIT_YAFFS2_CODE
        return YAFFS_FAIL;
#else
        if (dev->inbandTags) {
            if (!data) {
                localData = 1;
                data = yaffs_GetTempBuffer(dev, __LINE__);
            }

            rv = cyg_nandp_read_page(part, chunkInNAND, data, 0, 0);
            if (tags) {
                yaffs_PackedTags2TagsPart *pt2tp;
                pt2tp = (yaffs_PackedTags2TagsPart *)&data[dev->nDataBytesPerChunk];
                yaffs_UnpackTags2TagsPart(tags, pt2tp);
            }
        } else {
            yaffs_PackedTags2 pt;
            void *packed_tags_ptr = dev->noTagsECC ? (void*) &pt.t : (void*) &pt;
            rv = cyg_nandp_read_page(part, chunkInNAND, data, packed_tags_ptr, tags ? PACKEDTAGS2_OOBSIZE(dev) : 0);
            if (tags)
                yaffs_UnpackTags2(dev, tags, &pt);
        }
#endif
    }

    // all done:
    if (localData)
        yaffs_ReleaseTempBuffer(dev, data, __LINE__);

    if (rv != 0) {
        if (tags) {
            // mark the tags as clearly useless
            memset(tags, 0, sizeof(yaffs_ExtendedTags));
            tags->eccResult = YAFFS_ECC_RESULT_UNFIXED;
            tags->blockBad = 1;
        }
        return YAFFS_FAIL;
    }
    return YAFFS_OK;
    // TODO: go through yaffs_guts and ensure that all calls to ReadChunksWithTagsFromNAND DTRT on FAIL or if the read tags aren't valid.
}

int eyaffs_markNANDBlockBad (struct yaffs_DeviceStruct * dev, int blockNo)
{
    cyg_nand_partition *part = (cyg_nand_partition*) dev->genericDevice;
    int rv = cyg_nandp_bbt_markbad(part, blockNo);
    return rv==0 ? YAFFS_OK : YAFFS_FAIL;
}

int eyaffs_queryNANDBlock (struct yaffs_DeviceStruct * dev, int blockNo,
        yaffs_BlockState * state, __u32 *sequenceNumber)
{
    yaffs_ExtendedTags tags;
    cyg_nand_page_addr pg;
    cyg_nand_partition *part = (cyg_nand_partition*) dev->genericDevice;

    int rv = cyg_nandp_bbt_query(part, blockNo);
    if (rv < 0) return YAFFS_FAIL;

    switch(rv) {
        case CYG_NAND_BBT_OK:
            // Now we need to read the tags from the OOB area of the first page
            pg = CYG_NAND_BLOCK2PAGEADDR(part->dev, blockNo);
            // TODO: This is quite inefficient on small page devices in
            // YAFFS2 mode with inbandTags enabled, as the whole page has
            // to be read. Some day, it would be nice to implement part-page
            // reads in the NAND layer where this can be efficiently done.
            int rv2 = eyaffs_readChunkWithTagsFromNAND(dev, pg, 0, &tags);
            if (rv2 == YAFFS_FAIL) {
                (void) cyg_nandp_bbt_markbad(part, blockNo);
                goto deadblock;
            }
            if (tags.chunkUsed) {
                *state = YAFFS_BLOCK_STATE_NEEDS_SCANNING;
                *sequenceNumber = tags.sequenceNumber;
                return YAFFS_OK;
            }
            *state = YAFFS_BLOCK_STATE_EMPTY;
            *sequenceNumber = 0;
            return YAFFS_OK;

        case CYG_NAND_BBT_WORNBAD:
        case CYG_NAND_BBT_RESERVED:
        case CYG_NAND_BBT_FACTORY_BAD:
        deadblock:
            *state = YAFFS_BLOCK_STATE_DEAD;
            *sequenceNumber = 0;
            return YAFFS_OK;
        default:
            YBUG(); // can't happen
            return YAFFS_FAIL;
    }
}

