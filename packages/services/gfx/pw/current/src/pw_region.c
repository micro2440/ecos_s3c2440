/*===========================================================================
//
//        pw_region.c 
//
//        PW graphic library region code 
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

#define NO_GAP 0 /**< No gap. */ 
#define L_GAP  1 /**< Gap on left. */
#define T_GAP  2 /**< Gap on top. */
#define R_GAP  4 /**< Gap on right. */
#define B_GAP  8 /**< Gap on bottom. */

#define OUT_LEFT   1 /**< Point - Rectangle out code (left). */
#define OUT_RIGHT  2 /**< Point - Rectangle out code (right). */
#define OUT_TOP    4 /**< Point - Rectangle out code (top). */
#define OUT_BOTTOM 8 /**< Point - Rectangle out code (bottom). */

/* -------------------------------------------------------------------------- */

/**
 *  Gets the position of the point @p x, @p y according to 
 *  the rectangle @p r.
 *  @param r pointer to rectangle r 
 *  @param x point's x coord
 *  @param y point's y coord
 *  @return point position code
 *  @internal
 */ 
Pw_Int 
IPw_RectOutCode(Pw_Rectangle* r, Pw_Coord x, Pw_Coord y) 
{ 
    Pw_Int code = 0; 

    IPW_TRACE_ENTER(RE1);
    IPW_TRACE_RECT(RE1, r);
    IPW_TRACE2(RE1, d, x, y);

    IPW_CHECK_PTR(r);

    if (x < r->x) 
        code = OUT_LEFT; 
    else if (x >= (r->x + r->w)) 
        code = OUT_RIGHT; 

    if (y < r->y) 
        code |= OUT_TOP; 
    else if (y >= (r->y + r->h)) 
        code |= OUT_BOTTOM; 

    IPW_TRACE_EXIT_V(RE1, d, code);
    return(code); 
}

/**
 *  Calculates the intersection of rectangles @p a and @p b. 
 *  The result is returned as rectangle @p a (the old rectangle @p a
 *  is overwritten).
 *  @param a pointer to rectangle a 
 *  @param b pointer to rectangle b 
 *  @return true if the two rectangles intersect
 *  @internal
 */ 
Pw_Bool 
IPw_RectIntersect(Pw_Rectangle* a, Pw_Rectangle* b)
{
    Pw_Coord nx; 
    Pw_Coord ny; 

    IPW_TRACE_ENTER(RE1);
    IPW_TRACE_RECT2(RE1, a, b);
    IPW_CHECK_PTR2(a, b);

    nx = PW_MAX(a->x, b->x);
    ny = PW_MAX(a->y, b->y);

    a->w = PW_MIN(a->x + a->w, b->x + b->w) - nx;
    a->h = PW_MIN(a->y + a->h, b->y + b->h) - ny;
    
    a->x = nx;
    a->y = ny;

    IPW_TRACE_RECT(RE1, a);
    IPW_TRACE_EXIT_V(RE1, d, (a->w > 0 && a->h > 0));
    return(a->w > 0 && a->h > 0);
}

/**
 *  Calculates the union of rectangles @p a and @p b.
 *  The result is returned as rectangle @p a (the old rectangle @p a
 *  is overwritten).
 *  @param a pointer to rectangle a 
 *  @param b pointer to rectangle b
 *  @internal 
 */ 
void 
IPw_RectUnionize(Pw_Rectangle* a, Pw_Rectangle* b)
{
    Pw_Coord nx;
    Pw_Coord ny;

    IPW_TRACE_ENTER(RE1);
    IPW_TRACE_RECT2(RE1, a, b);
    IPW_CHECK_PTR2(a, b);

    /* If b is empty nothing to do */
    if (b->w <= 0 || b->h <= 0)
    {
        IPW_TRACE_RECT(RE1, a);
        IPW_TRACE_EXIT(RE1);
        return;
    }

    /* If a is empty just copy b into a */
    if (a->w <= 0 || a->h <= 0)
    {
        IPW_RECT_COPY(a, b);    

        IPW_TRACE_RECT(RE1, a);
        IPW_TRACE_EXIT(RE1);
        return;
    }

    nx = PW_MIN(a->x, b->x);
    ny = PW_MIN(a->y, b->y);

    a->w = PW_MAX(a->x + a->w, b->x + b->w) - nx;
    a->h = PW_MAX(a->y + a->h, b->y + b->h) - ny;
    a->x = nx;
    a->y = ny;

    IPW_TRACE_RECT(RE1, a);
    IPW_TRACE_EXIT(RE1);
}

/**
 *  Excludes the area of rectangle @p z from rectangle @p a.
 *  The result is returned as rectangle @p a, in case of rectangle
 *  @p a breaking into several rectangles, the newly generated 
 *  rectangles are returned as @p b, @p c and @p d.
 *  @param z pointer to rectangle z 
 *  @param a pointer to rectangle a 
 *  @param b pointer to rectangle b 
 *  @param c pointer to rectangle c 
 *  @param d pointer to rectangle d 
 *  @return the number of newly generated rectangles
 *  @internal
 */ 
Pw_Int 
IPw_RectExclude(Pw_Rectangle* z,
                Pw_Rectangle* a,
                Pw_Rectangle* b,
                Pw_Rectangle* c,
                Pw_Rectangle* d)
{
    Pw_uInt gaps;

    IPW_TRACE_ENTER(RE2);
    IPW_TRACE_RECT2(RE2, z, a);
    IPW_CHECK_PTR5(z, a, b, c, d);

    /* If rects z and a don't overlap exit */
    if (!IPW_RECT_IS_OVER(a, z))
    {
        IPW_TRACE_EXIT_V(RE2, d, 0);
        return(0);
    }

    gaps = NO_GAP;

    /* Check for gap on left */
    if (a->x < z->x) 
        gaps |= L_GAP;

    /* Check for gap on top */
    if (a->y < z->y) 
        gaps |= T_GAP;

    /* Check for gap on rigth */
    if ((a->x + a->w) > (z->x + z->w)) 
        gaps |= R_GAP;

    /* Check for gap on bottom */
    if ((a->y + a->h) > (z->y + z->h))
        gaps |= B_GAP;

    IPW_TRACE1(RE2, d, gaps);

    switch (gaps)
    {
        /*
         * No gaps
         *
         * +-----------+
         * |     Z     |
         * |   - - -   |
         * |  |     |  |
         * |     A     |
         * |  |     |  |
         * |   - - -   |
         * |           |
         * +-----------+
         */ 
        case (NO_GAP):
        {
            a->x = 0;
            a->y = 0;
            a->w = 0;
            a->h = 0;

            IPW_TRACE_RECT(RE2, a);
            IPW_TRACE_EXIT_V(RE2, d, -1);
            return(-1);
        }    
        /*
         * Gap on left
         *
         *       +------+
         * +-----|      |
         * |     |      |
         * |     |      |
         * |     |      |
         * |  A  |  Z   |
         * |     |      |
         * |     |      |
         * |     |      |
         * +-----|      |
         *       +------+
         */
        case (L_GAP):
        {
            a->w = z->x - a->x;

            IPW_TRACE_RECT(RE2, a);
            IPW_TRACE_EXIT_V(RE2, d, 0);
            return(0);
        }
        /*
         * Gap on top 
         *
         *  +-----------+
         *  |           |
         *  |     A     |
         *  |           |
         * +-------------+ 
         * |             |
         * |             |
         * |      Z      |
         * |             |
         * +-------------+
         */ 
        case (T_GAP):
        {
            a->h = z->y - a->y;

            IPW_TRACE_RECT(RE2, a);
            IPW_TRACE_EXIT_V(RE2, d, 0);
            return(0);
        }
        /*
         * Gap on right 
         *
         * +-------+
         * |       |-----+
         * |       |     |
         * |       |     |
         * |       |     |
         * |   Z   |  A  |
         * |       |     |
         * |       |     |
         * |       |     |
         * |       |-----+
         * +-------+
         */
        case (R_GAP):
        {
            Pw_Int dx = z->w + z->x - a->x;

            a->x += dx;
            a->w -= dx;

            IPW_TRACE_RECT(RE2, a);
            IPW_TRACE_EXIT_V(RE2, d, 0);
            return(0);
        }
        /*
         * Gap on bottom 
         *
         * +-------------+
         * |             |
         * |      Z      |
         * |             |
         * |             |
         * +-------------+ 
         *  |           |
         *  |     A     |
         *  |           |
         *  +-----------+
         */ 
        case (B_GAP):
        {
            Pw_Int dy = z->h + z->y - a->y;

            a->y += dy;
            a->h -= dy;

            IPW_TRACE_RECT(RE2, a);
            IPW_TRACE_EXIT_V(RE2, d, 0);
            return(0);
        }
        /*
         * Gaps on left and top 
         *       
         * +-----------+
         * |           |
         * |     A     |
         * |           |
         * +=====+------+ 
         * |     |      |
         * |  B  |      |
         * |     |   Z  |
         * +-----|      |
         *       +------+
         */ 
        case (L_GAP | T_GAP): 
        {
            b->x = a->x;
            b->y = z->y;
            b->w = z->x - a->x;
            b->h = a->y + a->h - b->y;

            a->h = z->y - a->y;

            IPW_TRACE_RECT2(RE2, a, b);
            IPW_TRACE_EXIT_V(RE2, d, 1);
            return(1);
        }
        /*
         * Gaps on left and right 
         *
         *     +---+
         * +---|   |---+
         * |   |   |   |
         * |   |   |   |
         * |   |   |   |
         * | A | Z | B |
         * |   |   |   |
         * |   |   |   |
         * |   |   |   |
         * +---|   |---+
         *     +---+
         */
        case (L_GAP | R_GAP):
        {
            b->x = z->x + z->w;
            b->y = a->y;
            b->w = a->x + a->w - b->x;
            b->h = a->h; 

            a->w = z->x - a->x;

            IPW_TRACE_RECT2(RE2, a, b);
            IPW_TRACE_EXIT_V(RE2, d, 1);
            return(1);
        }
        /*
         * Gaps on left and bottom 
         *       
         *       +------+ 
         * +-----|      |
         * |     |   Z  |
         * |  A  |      |
         * |     |      |
         * +=====+------+ 
         * |           |
         * |     B     |
         * |           |
         * +-----------+
         */ 
        case (L_GAP | B_GAP):
        {
            b->x = a->x;
            b->y = z->y + z->h;
            b->w = a->w;
            b->h = a->y + a->h - b->y;

            a->w = z->x - a->x;
            a->h = b->y - a->y;

            IPW_TRACE_RECT2(RE2, a, b);
            IPW_TRACE_EXIT_V(RE2, d, 1);
            return(1);
        }
        /*
         * Gaps on top and right 
         *       
         *  +-----------+
         *  |           |
         *  |     A     |
         *  |           |
         * +------+=====+ 
         * |      |     |
         * |      |  B  |
         * |  Z   |     |
         * |      |-----+
         * +------+
         */ 
        case (T_GAP | R_GAP):
        {
            b->x = z->x + z->w;
            b->y = z->y;
            b->w = a->x + a->w - b->x;
            b->h = a->y + a->h - b->y;

            a->h = b->y - a->y;

            IPW_TRACE_RECT2(RE2, a, b);
            IPW_TRACE_EXIT_V(RE2, d, 1);
            return(1);
        }
        /*
         * Gaps on top and bottom 
         *       
         *  +-----------+
         *  |     A     |
         *  |           |
         * +-------------+
         * |      Z      |
         * +-------------+
         *  |           |
         *  |     B     |
         *  +-----------+
         * 
         */ 
        case (T_GAP | B_GAP):
        {
            b->x = a->x;
            b->y = z->y + z->h;
            b->w = a->w;
            b->h = a->y + a->h - b->y;

            a->h = z->y - a->y;

            IPW_TRACE_RECT2(RE2, a, b);
            IPW_TRACE_EXIT_V(RE2, d, 1);
            return(1);
        }
        /*
         * Gaps on right and bottom 
         *       
         * +------+
         * |      |-----+
         * |  Z   |     |
         * |      |  A  |
         * |      |     |
         * +------+=====+ 
         *  |           |
         *  |     B     |
         *  |           |
         *  +-----------+
         */ 
        case (R_GAP | B_GAP):
        {
            Pw_Int dx = z->x + z->w - a->x;

            b->x = a->x;
            b->y = z->y + z->h;
            b->w = a->w;
            b->h = a->y + a->h - b->y;

            a->x += dx;
            a->w -= dx;
            a->h = b->y - a->y;

            IPW_TRACE_RECT2(RE2, a, b);
            IPW_TRACE_EXIT_V(RE2, d, 1);
            return(1);
        }
        /*
         * Gaps on left, top and right 
         *       
         * +-----------+
         * |     A     |
         * |           |
         * +===+---+===+
         * |   |   |   |
         * | B |   | C |
         * |   | Z |   |
         * |   |   |   |
         * +---|   |---+
         *     +---+
         */ 
        case (L_GAP | T_GAP | R_GAP): 
        {
            b->x = a->x;
            b->y = z->y;
            b->w = z->x - b->x;
            b->h = a->y + a->h - b->y;

            c->x = z->x + z->w;
            c->y = b->y;
            c->w = a->x + a->w - c->x;
            c->h = b->h;

            a->h = z->y - a->y;

            IPW_TRACE_RECT3(RE2, a, b, c);
            IPW_TRACE_EXIT_V(RE2, d, 2);
            return(2);
        }
        /*
         * Gaps on left, top and bottom 
         *       
         * +-----------+
         * |     A     |
         * |           |
         * +====+-------+
         * | B  |   Z   |
         * +====+-------+
         * |           |
         * |     C     |
         * +-----------+
         */ 
        case (L_GAP | T_GAP | B_GAP): 
        {
            b->x = a->x;
            b->y = z->y;
            b->w = z->x - b->x;
            b->h = z->y + z->h - b->y;

            c->x = a->x;
            c->y = b->y + b->h;
            c->w = a->w;
            c->h = a->y + a->h - c->y;

            a->h = z->y - a->y;

            IPW_TRACE_RECT3(RE2, a, b, c);
            IPW_TRACE_EXIT_V(RE2, d, 2);
            return(2);
        }
        /*
         * Gaps on left, right and bottom
         *
         *     +---+
         * +---|   |---+
         * |   | Z |   |
         * | A |   | B |
         * |   |   |   |
         * |   |   |   |
         * +===+---+===+
         * |           |
         * |     C     |
         * +-----------+
         */
        case (L_GAP | R_GAP | B_GAP):
        {
            b->x = z->x + z->w;
            b->y = a->y;
            b->w = a->x + a->w - b->x;
            b->h = z->y + z->h - b->y;
            
            c->x = a->x;
            c->y = z->y + z->h;
            c->w = a->w;
            c->h = a->y + a->h - c->y;

            a->w = z->x - a->x;
            a->h = b->h;

            IPW_TRACE_RECT3(RE2, a, b, c);
            IPW_TRACE_EXIT_V(RE2, d, 2);
            return(2);
        }
        /*
         * Gaps on top, right and bottom 
         *       
         *  +-----------+
         *  |     A     |
         *  |           |
         * +-------+====+
         * |   Z   |  B |
         * +-------+====+
         *  |           |
         *  |     C     |
         *  +-----------+
         */ 
        case (T_GAP | R_GAP | B_GAP):
        {
            b->x = z->x + z->w;
            b->y = z->y;
            b->w = a->x + a->w - b->x;
            b->h = z->y + z->h - b->y;

            c->x = a->x;
            c->y = z->y + z->h;
            c->w = a->w;
            c->h = a->y + a->h - c->y;

            a->h = z->y - a->y;

            IPW_TRACE_RECT3(RE2, a, b, c);
            IPW_TRACE_EXIT_V(RE2, d, 2);
            return(2);
        }
        /*
         * Gaps on all sides
         *       
         * +-----------+
         * |     A     |
         * |           |
         * +==+-----+==+ 
         * |B |  Z  | C|
         * +==+-----+==+ 
         * |           |
         * |     D     |
         * +-----------+
         */ 
        case (L_GAP | T_GAP | R_GAP | B_GAP):
        {
            b->x = a->x;
            b->y = z->y;
            b->w = z->x - b->x;
            b->h = z->y + z->h - b->y;

            c->x = z->x + z->w;
            c->y = b->y;
            c->w = a->x + a->w - c->x;
            c->h = b->h;
            
            d->x = a->x;
            d->y = z->y + z->h;
            d->w = a->w;
            d->h = a->y + a->h - d->y;

            a->h = z->y - a->y;

            IPW_TRACE_RECT4(RE2, a, b, c, d);
            IPW_TRACE_EXIT_V(RE2, d, 3);
            return(3);
        }
        default:
        {
            IPW_FAIL(FALSE, ("Unexpected gap code (%d)!", gaps));
        }
    }
    IPW_FAIL(FALSE, ("OOPS! Should not be HERE!"));
    return(0); /* Not reached */
}

/**
 *  Excludes the area of rectangle @p z from each rectangle in
 *  the list of regions @p rlist. The newly generated rectangles
 *  are stored into @p rlist as new regions. NOTE: region list
 *  @p rlist bounds don't get updated.
 *  @param z     pointer to rectangle z 
 *  @param rlist pointer to region list rlist
 *  @return true if there was enough space for new regions 
 *  @internal
 */
Pw_Bool
IPw_RectExcludeFromRegionList(Pw_Rectangle*  z,
                              Pw_RegionList* rlist)
{
    Pw_Int       ncnt, rcnt;
    Pw_Region*   reg;
    Pw_Region*   oregs;
    Pw_Region*   last;
    Pw_Rectangle nrects[3];

    IPW_TRACE_ENTER(RE3);
    IPW_TRACE_RECT(RE3, z);
    IPW_TRACE_RLIST(RE3, rlist);
    IPW_CHECK_PTR2(z, rlist);

    /*
     *  For each rect in region rlist list we call IPw_RectExclude
     *  if the num returned is -1 - delete the current region,
     *  if the num returned is  0 - do nothing
     *  if the num returned is >0 - append newly created rectangles
     *                              as regions at the beginning on the list
     *
     *  last  is the pointer to last processed (previous) region
     *  reg   is the pointer to current region
     *  oregs is the pointer to old reglion list start (changed when we 
     *                                                  add new regions)
     */

    last = PW_NULL; 
    reg  = oregs = rlist->regs;

    while (PW_NULL != reg)
    {
        ncnt = IPw_RectExclude(z,
                               &reg->area,
                               &nrects[0],
                               &nrects[1],
                               &nrects[2]);

        IPW_TRACE1(RE3, d, ncnt);

        switch (ncnt)
        {       
            case -1:
            {
                if (PW_NULL == last)
                {
                    IPW_CHECK_COND(oregs == rlist->regs);
                    rlist->regs = reg->next;
                    IPw_RegionFree(reg);
                    reg = oregs = rlist->regs;
                }
                else
                {
                    last->next = reg->next;
                    IPw_RegionFree(reg);
                    reg = last->next;
                }
                rlist->regs_num--;
                break;
            }
            case 0:
            {
                last = reg;
                reg = reg->next;
                break;
            }
            case 1:
            case 2:
            case 3:
            {
                rcnt = ncnt;
                rlist->regs = IPw_RegionNewListFromArray(nrects, 
                                                         &rcnt, 
                                                         oregs); 
                rlist->regs_num += rcnt;

                IPW_TRACE2(RE3, d, rcnt, rlist->regs_num);

                if (rcnt != ncnt)
                {
                    IPW_CHECK_RLIST(rlist);
                    IPW_TRACE_EXIT_V(RE3, d, 0);
                    return(FALSE);
                }
                oregs = rlist->regs;
                last = reg;
                reg = reg->next;
                break;
            }
            default:
            {
                IPW_FAIL(FALSE, ("Unexpected new rect cnt = %d!", ncnt));
            }
        }
        IPW_TRACE_RLIST(RE3, rlist);
        IPW_TRACE4(RE3, p, last, reg, oregs, rlist->regs);
    }
    IPW_CHECK_RLIST(rlist);
    IPW_TRACE_EXIT_V(RE3, d, 1);
    return(TRUE);
}

#if IPW_CHECK_EN(REGION_LIST)
Pw_Bool
IPw_RegionListCheck(Pw_RegionList* rlist, Pw_Bool hard_chk)
{
    if (rlist->regs_num < 0)
    {
        IPW_ASSERT(FALSE, ("Region list (%p) len = %d!", 
                   rlist, rlist->regs_num));
        return(FALSE);
    }
    if ((PW_NULL == rlist->regs && rlist->regs_num != 0) ||
        (rlist->regs_num == 0 && PW_NULL != rlist->regs))
    {
        IPW_ASSERT(FALSE, ("Region list (%p) regs error (regs = %p len = %d)!",
                   rlist, rlist->regs, rlist->regs_num));
        return(FALSE);
    }
    if (hard_chk)
    {
        Pw_Int i;
        Pw_Rectangle r;
        Pw_Region* reg;
        IPW_RECT_EMPTY(&r);

        i = 0;
        reg = rlist->regs;
        while (PW_NULL != reg)
        {
            IPW_ASSERT(reg->area.w > 0 && reg->area.h > 0,
                       ("empty region (w=%d h=%d)!", reg->area.w, reg->area.h));
            IPw_RectUnionize(&r, &reg->area);
            reg = reg->next;
            i++;
        }
        if (i != rlist->regs_num)
        {
            IPW_ASSERT(FALSE, ("Region list (%p) regs_num error %d != %d!",
                               rlist, i, rlist->regs_num));
            return(FALSE);
        }
        if (!IPW_RECT_IS_EQUAL(&r, &rlist->bounds))
        {
            IPW_ASSERT(FALSE, ("Region list (%p) bounds error "
                               "(%d %d %d %d) != (%d %d %d %d)!",
                               rlist, r.x, r.y, r.w, r.h,
                               rlist->bounds.x, rlist->bounds.y, 
                               rlist->bounds.w, rlist->bounds.h));
            return(FALSE);
        }
    }
    else
    {
        Pw_Int i;
        Pw_Region* reg;

        i = 0;
        reg = rlist->regs;
        while (PW_NULL != reg)
        {
            IPW_ASSERT(reg->area.w > 0 && reg->area.h > 0,
                       ("empty region (w=%d h=%d)!", reg->area.w, reg->area.h));
            reg = reg->next;
            i++;
        }
        if (i != rlist->regs_num)
        {
            IPW_ASSERT(FALSE, ("Region list (%p) regs_num error %d != %d!",
                               rlist, i, rlist->regs_num));
            return(FALSE);
        }
    }
    return(TRUE);
}
#endif /* IPW_CHECK_EN(REGION_LIST) */

/*---------------------------------------------------------------------------
// end of pw_region.c 
//--------------------------------------------------------------------------- */
