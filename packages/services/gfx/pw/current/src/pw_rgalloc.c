/*===========================================================================
//
//        pw_rgalloc.c
//
//        PW graphic library region allocation code 
//
//===========================================================================
//===========================================================================
// This file is part of PW graphic library.
// Copyright (C) 2004 Savin Zlobec 
//
// PW is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// PW is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with PW; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
//===========================================================================
//===========================================================================
//
//  Author(s): Savin Zlobec 
//
//=========================================================================== */

#include <i_pwin.h>

/* -------------------------------------------------------------------------- */

#ifdef IPW_USE_ASSERTS

#define _CHECK_INIT()                                       \
    IPW_MACRO_START                                         \
        IPW_ASSERT(_free_reg_initialized == TRUE,           \
                    ("Free region list not initialized!")); \
    IPW_MACRO_END

#else /* IPW_USE_ASSERTS */

#define _CHECK_INIT()

#endif /* not IPW_USE_ASSERTS */

#if IPW_CHECK_EN(REGION_ALLOC)

#define _CHECK_REGION(_reg_)                                            \
    IPW_MACRO_START                                                     \
        IPW_ASSERT((Pw_Addr)(_reg_) >=                                  \
                        (Pw_Addr)&_free_reg_pool[0] &&                  \
                   (Pw_Addr)(_reg_) <=                                  \
                        (Pw_Addr)&_free_reg_pool[IPW_REGION_MAX_NUM-1], \
                   ("Region (%d) not from pool!", (Pw_Addr)(_reg_)));   \
    IPW_MACRO_END

#else /* IPW_CHECK_EN(REGION_ALLOC) */

#define _CHECK_REGION(_reg_)

#endif /* not IPW_CHECK_EN(REGION_ALLOC) */


#if IPW_CHECK_EN(REGION_FREE_LIST)

#define _CHECK_FREE_REG_LIST()                            \
    IPW_MACRO_START                                       \
        Pw_Int16 frl = _RegionFreeListLen();              \
        IPW_ASSERT(_free_reg_cnt == frl,                  \
                  ("Free region list len error %d != %d", \
                  _free_reg_cnt, frl));                   \
    IPW_MACRO_END                   
 
#else /* IPW_CHECK_EN(REGION_FREE_LIST) */

#define _CHECK_FREE_REG_LIST()

#endif /* not IPW_CHECK_EN(REGION_FREE_LIST) */


#if IPW_CHECK_EN(REGION_MAX_USAGE)

#define _CHECK_MAX_USAGE()                                      \
    IPW_MACRO_START                                             \
        if (_free_reg_cnt < _free_reg_min)                      \
        {                                                       \
            IPW_NOTE(("%d free regions left", _free_reg_cnt));  \
            _free_reg_min = _free_reg_cnt;                      \
        }                                                       \
    IPW_MACRO_END

#else /* IPW_CHECK_EN(REGION_MAX_USAGE) */

#define _CHECK_MAX_USAGE()

#endif /* IPW_CHECK_EN(REGION_MAX_USAGE) */

/* -------------------------------------------------------------------------- */

static Pw_Region  _free_reg_pool[IPW_REGION_MAX_NUM];
static Pw_Region* _free_reg_list = PW_NULL;
static Pw_Int16   _free_reg_cnt  = 0;

#ifdef IPW_USE_ASSERTS
static Pw_Bool _free_reg_initialized = FALSE;
#endif /* IPW_USE_ASSERTS */

#if IPW_CHECK_EN(REGION_MAX_USAGE)
static Pw_Int16   _free_reg_min = IPW_REGION_MAX_NUM;
#endif /* IPW_CHECK_EN(REGION_MAX_USAGE) */

/* -------------------------------------------------------------------------- */

#if IPW_CHECK_EN(REGION_FREE_LIST)
static Pw_Int16
_RegionFreeListLen(void)
{
    Pw_Int16 i = 0;
    Pw_Region* reg  = _free_reg_list;

    while (PW_NULL != reg)
    {
        _CHECK_REGION(reg);
        reg = reg->next;
        i++;
    }
    return(i);
} 
#endif /* IPW_CHECK_EN(REGION_FREE_LIST) */

/* -------------------------------------------------------------------------- */

/**
 *  Initializes the region pool.
 *  This function must be called before any other
 *  region allocation/deallocation functions are used.
 *  @internal
 */
void
IPw_RegionInit(void)
{
    Pw_Int i;

    IPW_TRACE_ENTER(RA2);

#ifdef IPW_USE_ASSERTS
    _free_reg_initialized = TRUE;
#endif /* IPW_USE_ASSERTS */

    /*
     *  Go trough the region array and link regions
     *  together - the result is a link list of
     *  regions. Then initialize pointer to
     *  first free region and the free region counter.
     */

    for (i = 0; i < IPW_REGION_MAX_NUM-1; i++)
    {
        _free_reg_pool[i].next = &_free_reg_pool[i+1];
    }

    _free_reg_pool[IPW_REGION_MAX_NUM-1].next = PW_NULL;

    _free_reg_list = &_free_reg_pool[0];
    _free_reg_cnt  = IPW_REGION_MAX_NUM;

    IPW_TRACE1(RA2, d, _free_reg_cnt);
    _CHECK_FREE_REG_LIST(); 
    IPW_TRACE_EXIT(RA2);
}

/**
 *  Tries to allocate a region.
 *  This function tries to allocate a region
 *  in case there are no free regions left a NULL
 *  pointer is returned.
 *  @return pointer to region if allocation was 
 *          successful, otherwise a NULL pointer is
 *          returned
 *  @internal
 */
Pw_Region* 
IPw_RegionNew(void)
{
    Pw_Region* reg;

    IPW_TRACE_ENTER(RA2);

    _CHECK_INIT();
    _CHECK_FREE_REG_LIST(); 

    IPW_TRACE2M(RA1, p, _free_reg_list, d, _free_reg_cnt);

    /*
     *  Check the free region pointer if NULL return NULL,
     *  otherwise return the first free region in list and
     *  move free region pointer to the next region in list.
     */

    if (PW_NULL == _free_reg_list)
    {
        IPW_TRACE_EXIT_V(RA2, p, PW_NULL);
        return(PW_NULL);
    }
    else
    {
        reg = _free_reg_list;
        _free_reg_list = _free_reg_list->next;
        _free_reg_cnt--;
    }
    reg->next = PW_NULL;

    IPW_TRACE2M(RA1, p, _free_reg_list, d, _free_reg_cnt);

    _CHECK_MAX_USAGE();
    _CHECK_FREE_REG_LIST(); 
    IPW_TRACE_EXIT_V(RA2, p, reg);
    return(reg);
} 

/**
 *  Tries to allocate a linked list of regions.
 *  This function tries to allocate a linked list of regions
 *  of length @p len. In case there are not enough free regions
 *  left a shorter list then requested is returned, the length is
 *  passed back trough the @p len parameter. In case there are no 
 *  free regions left a NULL pointer is returned.
 *  @param len  pointer to requested list length
 *  @param link pointer to region that the next pointer of last
 *              region in allocated list should point to (can be NULL)
 *  @return pointer to first region in list if allocation was successful 
 *          (or semi successful), otherwise a NULL pointer is returned
 *  @internal
 */
Pw_Region*
IPw_RegionNewList(Pw_Int*     len, 
                  Pw_Region*  link)
{
    Pw_Int i, cnt;
    Pw_Region* preg;
    Pw_Region* reg;

    IPW_TRACE_ENTER(RA2);
    IPW_CHECK_PTR(len);
    IPW_CHECK_COND(*len >= 0);
    _CHECK_INIT();
    _CHECK_FREE_REG_LIST();

    /*
     *  First determine the length of list that should be
     *  allocated (check if we have enough space left),
     *  then construct the list to be returned - always
     *  taking the first free region in free regions list
     */

    if (*len <= _free_reg_cnt)
        cnt = *len;
    else
        cnt = _free_reg_cnt;

    reg = preg = link;

    IPW_TRACE3(RA1, d, *len, _free_reg_cnt, cnt);
    IPW_TRACE3(RA1, p, reg, preg, _free_reg_list);

    for (i = cnt - 1; i >= 0; i--)
    {
        reg = _free_reg_list;
        _free_reg_list = _free_reg_list->next;
        _free_reg_cnt--;
        reg->next = preg;
        preg = reg;
    }        

    *len = cnt;

    IPW_TRACE3(RA1, d, *len, _free_reg_cnt, cnt);
    IPW_TRACE3(RA1, p, reg, preg, _free_reg_list);
    _CHECK_MAX_USAGE();
    _CHECK_FREE_REG_LIST();
    IPW_TRACE_EXIT_V(RA2, p, reg);
    return(reg);
}

/**
 *  Tries to allocate a linked list of regions and initialize them to the
 *  values in rectangle array @p rarray.
 *  This function tries to allocate a linked list of regions
 *  of length @p len and at the same time initializeing them to the
 *  values stored in rectangle array @p rarray. In case there are not 
 *  enough free regions left a shorter list then requested is returned, 
 *  the length is passed back trough the @p len parameter. 
 *  In case there are no free regions left a NULL pointer is returned.
 *  @param rarray rectangle array to wich the list should be initialized
 *                (the array length should be at least the length of
 *                 requested list)
 *  @param len  pointer to requested list length
 *  @param link pointer to region that the next pointer of last
 *              region in allocated list should point to (can be NULL)
 *  @return pointer to first region in list if allocation was successful 
 *          (or semi successful), otherwise a NULL pointer is returned
 *  @internal
 */
Pw_Region*
IPw_RegionNewListFromArray(Pw_Rectangle* rarray, 
                           Pw_Int*       len, 
                           Pw_Region*    link)
{
    Pw_Int     i, cnt;
    Pw_Region* preg;
    Pw_Region* reg;

    IPW_TRACE_ENTER(RA2);
    IPW_CHECK_PTR2(len, rarray);
    IPW_CHECK_COND(*len >= 0);
    _CHECK_INIT();
    _CHECK_FREE_REG_LIST();

    /*
     *  First determine the length of list that should be
     *  allocated (check if we have enough space left),
     *  then construct the list to be returned - always
     *  taking the first free region in free regions list
     *  and initializeing it to the current rectangle in 
     *  specified rectangle array
     */

    if (*len <= _free_reg_cnt)
        cnt = *len;
    else
        cnt = _free_reg_cnt;

    reg = preg = link;

    IPW_TRACE3(RA1, d, *len, _free_reg_cnt, cnt);
    IPW_TRACE3(RA1, p, reg, preg, _free_reg_list);

    for (i = cnt - 1; i >= 0; i--)
    {
        reg = _free_reg_list;
        _free_reg_list = _free_reg_list->next;
        _free_reg_cnt--;
        IPW_RECT_COPY(&reg->area, &rarray[i]);
        reg->next = preg;
        preg = reg;
    }        

    *len = cnt;

    IPW_TRACE3(RA1, d, *len, _free_reg_cnt, cnt);
    IPW_TRACE3(RA1, p, reg, preg, _free_reg_list);
    _CHECK_MAX_USAGE();
    _CHECK_FREE_REG_LIST();
    IPW_TRACE_EXIT_V(RA2, p, reg);
    return(reg);
}

/**
 *  Tries to allocate a linked list of regions and initialize them to the
 *  values in region list @p rlist.
 *  This function tries to allocate a linked list of regions
 *  of length @p len and at the same time initializeing them to the
 *  values stored in region list @p rlist. In case there are not 
 *  enough free regions left a shorter list then requested is returned, 
 *  the length is passed back trough the @p len parameter. 
 *  In case there are no free regions left a NULL pointer is returned.
 *  @param rlist a list of regions to wich the requested list should be 
 *               initialized (the specified list length should be at least
 *               the length of requested list)
 *  @param len  pointer to requested list length
 *  @param link pointer to region that the next pointer of last
 *              region in allocated list should point to (can be NULL)
 *  @return pointer to first region in list if allocation was successful 
 *          (or semi successful), otherwise a NULL pointer is returned
 *  @internal
 */
Pw_Region*
IPw_RegionNewListFromList(Pw_RegionList* rlist, 
                          Pw_Int*        len, 
                          Pw_Region*     link)
{
    Pw_Int     i, cnt;
    Pw_Region* preg;
    Pw_Region* reg;
    Pw_Region* lreg;

    IPW_TRACE_ENTER(RA2);
    IPW_CHECK_PTR2(len, rlist);
    IPW_CHECK_COND((*len >= 0) && (*len <= rlist->regs_num));
    _CHECK_INIT();
    _CHECK_FREE_REG_LIST();
    IPW_TRACE_RLIST(RA1, rlist);

    /*
     *  First determine the length of list that should be
     *  allocated (check if we have enough space left),
     *  then construct the list to be returned - always
     *  taking the first free region in free regions list
     *  and initializeing it to the current region in 
     *  specified region list
     */

    if (*len <= _free_reg_cnt)
        cnt = *len;
    else
        cnt = _free_reg_cnt;

    lreg = rlist->regs;
    reg = preg = link;

    IPW_TRACE3(RA1, d, *len, _free_reg_cnt, cnt);
    IPW_TRACE4(RA1, p, reg, preg, lreg, _free_reg_list);

    for (i = cnt - 1; i >= 0; i--)
    {
        IPW_CHECK_PTR(lreg);
        reg = _free_reg_list;
        _free_reg_list = _free_reg_list->next;
        _free_reg_cnt--;
        IPW_RECT_COPY(&reg->area, &lreg->area);
        reg->next = preg;
        preg = reg;
        lreg = lreg->next;
    }        

    *len = cnt;

    IPW_TRACE3(RA1, d, *len, _free_reg_cnt, cnt);
    IPW_TRACE4(RA1, p, reg, preg, lreg, _free_reg_list);
    _CHECK_MAX_USAGE();
    _CHECK_FREE_REG_LIST();
    IPW_TRACE_EXIT_V(RA2, p, reg);
    return(reg);
}

/**
 *  Frees the specified region. 
 *  @param reg pointer to region to be freed 
 *  @internal
 */
void 
IPw_RegionFree(Pw_Region* reg)
{
    IPW_TRACE_ENTER(RA2);
    IPW_CHECK_PTR(reg);
    _CHECK_REGION(reg);
    _CHECK_INIT();
    _CHECK_FREE_REG_LIST(); 
    IPW_TRACE2M(RA1, p, _free_reg_list, d, _free_reg_cnt);

    /*
     *  Put the specified region at the beginning of free regions list
     */

    reg->next = _free_reg_list;
    _free_reg_list = reg;
    _free_reg_cnt++;

    IPW_TRACE2M(RA1, p, _free_reg_list, d, _free_reg_cnt);
    _CHECK_FREE_REG_LIST(); 
    IPW_TRACE_EXIT(RA2);
}

/**
 *  Frees the specified region list. 
 *  @param reg pointer to the first region in list to be freed 
 *  @internal
 */
void 
IPw_RegionFreeList(Pw_Region* reg)
{
    Pw_Region* tmp;

    IPW_TRACE_ENTER(RA2);
    IPW_CHECK_PTR(reg);
    _CHECK_INIT();
    _CHECK_FREE_REG_LIST(); 
    IPW_TRACE2M(RA1, p, _free_reg_list, d, _free_reg_cnt);

    /*
     *  Free all regions in list by putting them at the beginning
     *  of free region list one by one
     */

    while (PW_NULL != reg)
    {
        _CHECK_REGION(reg);

        tmp = reg->next;
        reg->next = _free_reg_list;
        _free_reg_list = reg;
        _free_reg_cnt++;
        reg = tmp;
    }

    IPW_TRACE2M(RA1, p, _free_reg_list, d, _free_reg_cnt);
    _CHECK_FREE_REG_LIST(); 
    IPW_TRACE_EXIT(RA2);
}

/**
 *  Gets the number of free regions left. 
 *  @return the number of free regions left
 *  @internal
 */
Pw_Int16
IPw_RegionGetFreeCnt(void)
{
    IPW_TRACE_ENTER(RA2);
    IPW_TRACE_EXIT_V(RA2, d, _free_reg_cnt);
    return(_free_reg_cnt);
} 

/*---------------------------------------------------------------------------
// end of pw_rgalloc.c 
//--------------------------------------------------------------------------- */
