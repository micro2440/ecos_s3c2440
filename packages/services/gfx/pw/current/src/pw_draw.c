/*===========================================================================
//
//        pw_draw.c
//
//        PW graphic library drawing code 
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

/** An array of bitmasks. Used for manipulating bits in bitmaps. */
static const Pw_Byte _bm_edgemask[2][9] =
{
        {0x00, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00},
        {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF}
};

/* -------------------------------------------------------------------------- */

/**
 *  Draws a point with clipping.
 *  @param gc   pointer to graphic context
 *  @param x    point x coord 
 *  @param y    point y coord 
 *  @param clip pointer to clip rectangle
 *  @return true if point is visible (i.e. drawn)
 *  @internal
 */ 
static Pw_Bool
_DrawClipPoint(Pw_GC*        gc, 
               Pw_Coord      x, 
               Pw_Coord      y,
               Pw_Rectangle* clip)
{
    IPW_TRACE_ENTER(DR1);
    IPW_TRACE2(DR1, d, x, y);
    IPW_TRACE_RECT(DR1, clip);

    /*
     *  Check if point is inside clip (visible)
     *  and draw it accordingly
     */

    if (IPW_RECT_IS_POINT_IN(x, y, clip))
    {
        gc->d.lld->draw_point(gc, x, y);

        IPW_TRACE_EXIT_V(DR1, d, TRUE);
        return(TRUE);
    }
    IPW_TRACE_EXIT_V(DR1, d, FALSE);
    return(FALSE);
}

/**
 *  Draws a horizontal line with clipping.
 *  @param gc   pointer to graphic context
 *  @param x1   start point x coord 
 *  @param y1   start point y coord 
 *  @param x2   end point x coord 
 *  @param clip pointer to clip rectangle
 *  @return true if line is completely visible
 *  @internal
 */ 
static Pw_Bool 
_DrawClipHorLine(Pw_GC*        gc, 
                 Pw_Coord      x1, 
                 Pw_Coord      y1, 
                 Pw_Coord      x2,
                 Pw_Rectangle* clip)
{
    IPW_TRACE_ENTER(DR1);
    IPW_TRACE3(DR1, d, x1, y1, x2);
    IPW_TRACE_RECT(DR1, clip);

    /*
     *  First make shure that x1 is smaller than x2 
     *  (flip them if needed). Then check if the line's
     *  y coord is inside the clip rectangle, if y 
     *  falls outside the line is not visible at all. 
     *  If y falls inside than clip the left side (x1) and
     *  the right side (x2). If there is anything left
     *  of the line - draw it.
     */

    if (x1 > x2)
    {
        Pw_Coord t;
        t  = x1; x1 = x2; x2 = t;
    }

    if ((y1 >= clip->y) && (y1 < (clip->y + clip->h)))
    {
        Pw_Coord cx1 = x1;
        Pw_Coord cx2 = x2; 

        if (cx1 < clip->x)
            cx1 = clip->x;

        if (cx2 >= (clip->x + clip->w))
            cx2 = clip->x + clip->w - 1;

        IPW_TRACE2(DR1, d, cx1, cx2);

        if (cx1 == x1 && cx2 == x2)
        {
            gc->d.lld->draw_hor_line(gc, x1, y1, x2);
            IPW_TRACE_EXIT_V(DR1, d, TRUE); 
            return(TRUE); 
        }
        else if (cx1 <= cx2)
        {
            gc->d.lld->draw_hor_line(gc, cx1, y1, cx2);
            IPW_TRACE_EXIT_V(DR1, d, FALSE);
            return(FALSE);
        }
    }
    IPW_TRACE_EXIT_V(DR1, d, FALSE); 
    return(FALSE);
}

/**
 *  Draws a vertical line with clipping.
 *  @param gc   pointer to graphic context
 *  @param x1   start point x coord 
 *  @param y1   start point y coord 
 *  @param y2   end point y coord 
 *  @param clip pointer to clip rectangle.
 *  @return true if line is completely visible 
 *  @internal
 */ 
static Pw_Bool 
_DrawClipVerLine(Pw_GC*        gc, 
                 Pw_Coord      x1, 
                 Pw_Coord      y1, 
                 Pw_Coord      y2,
                 Pw_Rectangle* clip)
{
    IPW_TRACE_ENTER(DR1);
    IPW_TRACE3(DR1, d, x1, y1, y2);
    IPW_TRACE_RECT(DR1, clip);

    /*
     *  First make shure that y1 is smaller than y2 
     *  (flip them if needed). Then check if the line's
     *  x coord is inside the clip rectangle, if x 
     *  falls outside the line is not visible at all. 
     *  If x falls inside than clip the top side (y1) and
     *  the bottom side (y2). If there is anything left
     *  of the line - draw it.
     */

    if (y1 > y2)
    {
        Pw_Coord t;
        t  = y1; y1 = y2; y2 = t;
    }

    if ((x1 >= clip->x) && (x1 < (clip->x + clip->w)))
    {
        Pw_Coord cy1 = y1;
        Pw_Coord cy2 = y2; 

        if (cy1 < clip->y)
            cy1 = clip->y;

        if (cy2 >= (clip->y + clip->h))
            cy2 = clip->y + clip->h - 1;

        IPW_TRACE2(DR1, d, cy1, cy2);

        if (cy1 == y1 && cy2 == y2)
        {
            gc->d.lld->draw_ver_line(gc, x1, y1, y2);
            IPW_TRACE_EXIT_V(DR1, d, TRUE); 
            return(TRUE); 
        }
        else if (cy1 <= cy2)
        {
            gc->d.lld->draw_ver_line(gc, x1, cy1, cy2);
            IPW_TRACE_EXIT_V(DR1, d, FALSE); 
            return(FALSE); 
        }
    }
    IPW_TRACE_EXIT_V(DR1, d, FALSE); 
    return(FALSE);
} 

/**
 *  Draws a line without clipping.
 *  @param gc pointer to graphic context
 *  @param x1 start point x coord 
 *  @param y1 start point y coord 
 *  @param x2 end point x coord 
 *  @param y2 end point y coord 
 *  @internal
 */ 
static void 
_DrawLine(Pw_GC*   gc, 
          Pw_Coord x1, 
          Pw_Coord y1, 
          Pw_Coord x2,
          Pw_Coord y2)
{
    Pw_Int dx, dy;
    Pw_Int stepx, stepy;

    IPW_TRACE_ENTER(DR2);
    IPW_TRACE4(DR2, d, x1, x2, y1, y2); 

    /* Bresenham's line drawing algorithm */

    dx = x2 - x1;
    dy = y2 - y1;
    
    if (dy < 0) 
    { 
        dy = -dy;  
        stepy = -1; 
    } 
    else 
    { 
        stepy = 1; 
    }
    if (dx < 0) 
    { 
        dx = -dx;  
        stepx = -1; 
    } 
    else 
    { 
        stepx = 1; 
    }
    dy <<= 1;
    dx <<= 1;

    gc->d.lld->draw_point(gc, x1, y1);
    if (dx > dy) 
    {
        Pw_Int fraction = dy - (dx >> 1);
        while (x1 != x2) 
        {
            if (fraction >= 0) 
            {
                y1 += stepy;
                fraction -= dx;
            }
            x1 += stepx;
            fraction += dy;
            gc->d.lld->draw_point(gc, x1, y1);
        }
    } 
    else 
    {
        Pw_Int fraction = dx - (dy >> 1);
        while (y1 != y2) 
        {
            if (fraction >= 0) 
            {
                x1 += stepx;
                fraction -= dy;
            }
            y1 += stepy;
            fraction += dx;
            gc->d.lld->draw_point(gc, x1, y1);
        }
    }
    IPW_TRACE_EXIT(DR2);
} 

/**
 *  Draws a line with clipping.
 *  @param gc   pointer to graphic context
 *  @param x1   start point x coord 
 *  @param y1   start point y coord 
 *  @param x2   end point x coord 
 *  @param y2   end point y coord
 *  @param clip pointer to clip rectangle
 *  @todo make this smarter than point to point clipping
 *  @internal
 */ 
static void 
_DrawClipLine(Pw_GC*        gc, 
              Pw_Coord      x1, 
              Pw_Coord      y1, 
              Pw_Coord      x2,
              Pw_Coord      y2,
              Pw_Rectangle* clip)
{
    Pw_Int dx, dy;
    Pw_Int stepx, stepy;

    IPW_TRACE_ENTER(DR2);
    IPW_TRACE4(DR2, d, x1, y1, x2, y2);
    IPW_TRACE_RECT(DR2, clip);

    /* Bresenham's line drawing algorithm */

    dx = x2 - x1;
    dy = y2 - y1;
    
    if (dy < 0) 
    { 
        dy = -dy;  
        stepy = -1; 
    } 
    else 
    { 
        stepy = 1; 
    }
    if (dx < 0) 
    { 
        dx = -dx;  
        stepx = -1; 
    } 
    else 
    { 
        stepx = 1; 
    }
    dy <<= 1;
    dx <<= 1;

    _DrawClipPoint(gc, x1, y1, clip);
    if (dx > dy) 
    {
        Pw_Int fraction = dy - (dx >> 1);
        while (x1 != x2) 
        {
            if (fraction >= 0) 
            {
                y1 += stepy;
                fraction -= dx;
            }
            x1 += stepx;
            fraction += dy;
            _DrawClipPoint(gc, x1, y1, clip);
        }
    } 
    else 
    {
        Pw_Int fraction = dx - (dy >> 1);
        while (y1 != y2) 
        {
            if (fraction >= 0) 
            {
                x1 += stepx;
                fraction -= dy;
            }
            y1 += stepy;
            fraction += dx;
            _DrawClipPoint(gc, x1, y1, clip);
        }
    }
    IPW_TRACE_EXIT(DR2);
}

/**
 *  Draws an ellipse without clipping.
 *  @param gc   pointer to graphic context
 *  @param x    ellipse center x coord 
 *  @param y    ellipse center y coord 
 *  @param rx   x radius
 *  @param ry   y radius
 *  @param fill if true the ellipse will be filled
 *  @internal
 */ 
static void 
_DrawEllipse(Pw_GC*   gc, 
             Pw_Coord x, 
             Pw_Coord y, 
             Pw_Coord rx, 
             Pw_Coord ry,
             Pw_Bool  fill)
{

    /* Algorithm from IEEE CG&A Sept 1984 p.24 */

    Pw_Long t1 = rx * rx, t2 = t1 << 1, t3 = t2 << 1;
    Pw_Long t4 = ry * ry, t5 = t4 << 1, t6 = t5 << 1;
    Pw_Long t7 = rx * t5, t8 = t7 << 1, t9 = 0;
    Pw_Long d1 = t2 - t7 + (t4 >> 1);
    Pw_Long d2 = (t1 >> 1) - t8 + t5;

    Pw_Int ex = rx, ey = 0;

    IPW_TRACE_ENTER(DR2);
    IPW_TRACE4(DR2, d, x, y, rx, ry);

    while (d2 < 0)
    {
        if (fill)
        {
            gc->d.lld->draw_hor_line(gc, x - ex, y + ey, x + ex);
            gc->d.lld->draw_hor_line(gc, x - ex, y - ey, x + ex);
        }
        else
        {
            gc->d.lld->draw_point(gc, x + ex, y + ey);
            gc->d.lld->draw_point(gc, x + ex, y - ey);
            gc->d.lld->draw_point(gc, x - ex, y + ey);
            gc->d.lld->draw_point(gc, x - ex, y - ey);
        }

        ey++;        
        t9 += t3;    
        if (d1 < 0)
        {
            d1 += t9 + t2;
            d2 += t9;
        }
        else
        {
            ex--;
            t8 -= t6;
            d1 += t9 + t2 - t8;
            d2 += t9 + t5 - t8;
        }
    }

    do
    {
        if (fill)
        {
            gc->d.lld->draw_hor_line(gc, x - ex, y + ey, x + ex);
            gc->d.lld->draw_hor_line(gc, x - ex, y - ey, x + ex);
        }
        else
        {
            gc->d.lld->draw_point(gc, x + ex, y + ey);
            gc->d.lld->draw_point(gc, x + ex, y - ey);
            gc->d.lld->draw_point(gc, x - ex, y + ey);
            gc->d.lld->draw_point(gc, x - ex, y - ey);
        }

        ex--;    
        t8 -= t6;    
        if (d2 < 0)
        {
            ey++;
            t9 += t3;
            d2 += t9 + t5 - t8;
        }
        else
        {
            d2 += t5 - t8;
        }
    } 
    while (ex >= 0);

    IPW_TRACE_EXIT(DR2);
}

/**
 *  Draws an ellipse with clipping.
 *  @param gc   pointer to graphic context
 *  @param x    ellipse center x coord 
 *  @param y    ellipse center y coord 
 *  @param rx   x radius
 *  @param ry   y radius
 *  @param fill if true the ellipse will be filled
 *  @param clip pointer to clip rectangle
 *  @todo make this smarter than point to point clipping
 *  @internal
 */ 
static void
_DrawClipEllipse(Pw_GC*        gc,
                 Pw_Coord      x,
                 Pw_Coord      y,
                 Pw_Coord      rx,
                 Pw_Coord      ry,
                 Pw_Bool       fill,
                 Pw_Rectangle* clip)
{

    /* Algorithm from IEEE CG&A Sept 1984 p.24 */

    Pw_Long t1 = rx * rx, t2 = t1 << 1, t3 = t2 << 1;
    Pw_Long t4 = ry * ry, t5 = t4 << 1, t6 = t5 << 1;
    Pw_Long t7 = rx * t5, t8 = t7 << 1, t9 = 0;
    Pw_Long d1 = t2 - t7 + (t4 >> 1);
    Pw_Long d2 = (t1 >> 1) - t8 + t5;

    Pw_Int ex = rx, ey = 0;

    IPW_TRACE_ENTER(DR2);
    IPW_TRACE4(DR2, d, x, y, rx, ry);
    IPW_TRACE_RECT(DR2, clip);

    while (d2 < 0)
    {
        if (fill)
        {
            _DrawClipHorLine(gc, x - ex, y + ey, x + ex, clip);
            _DrawClipHorLine(gc, x - ex, y - ey, x + ex, clip);
        }
        else
        {
            _DrawClipPoint(gc, x + ex, y + ey, clip);
            _DrawClipPoint(gc, x + ex, y - ey, clip);
            _DrawClipPoint(gc, x - ex, y + ey, clip);
            _DrawClipPoint(gc, x - ex, y - ey, clip);
        }

        ey++;        
        t9 += t3;    
        if (d1 < 0)
        {
            d1 += t9 + t2;
            d2 += t9;
        }
        else
        {
            ex--;
            t8 -= t6;
            d1 += t9 + t2 - t8;
            d2 += t9 + t5 - t8;
        }
    }

    do
    {
        if (fill)
        {
            _DrawClipHorLine(gc, x - ex, y + ey, x + ex, clip);
            _DrawClipHorLine(gc, x - ex, y - ey, x + ex, clip);
        }
        else
        {
            _DrawClipPoint(gc, x + ex, y + ey, clip);
            _DrawClipPoint(gc, x + ex, y - ey, clip);
            _DrawClipPoint(gc, x - ex, y + ey, clip);
            _DrawClipPoint(gc, x - ex, y - ey, clip);
        }

        ex--;    
        t8 -= t6;    
        if (d2 < 0)
        {
            ey++;
            t9 += t3;
            d2 += t9 + t5 - t8;
        }
        else
        {
            d2 += t5 - t8;
        }
    } 
    while (ex >= 0);

    IPW_TRACE_EXIT(DR2);
} 

/**
 *  Draws a bitmap without clipping.
 *  @param gc   pointer to graphic context
 *  @param x    x coord of bitmap's top left corner
 *  @param y    y coord of bitmap's top left corner
 *  @param w    bitmap's width 
 *  @param h    bitmap's height
 *  @param bits bitmap bits array
 *  @internal
 */ 
static void 
_DrawBitmap(Pw_GC*    gc, 
            Pw_Coord  x, 
            Pw_Coord  y, 
            Pw_Coord  w, 
            Pw_Coord  h, 
            Pw_Byte*  bits) 
{
    Pw_uInt  i, j;
    Pw_Coord w8, rx;

    IPW_TRACE_ENTER(DR2);
    IPW_TRACE4(DR2, d, x, y, w, h);

    /*
     *  Calculate bitmap's width in bytes 
     *  (bitmap lines end always on byte border - and the unused 
     *  bits are just 0) and than draw each byte using draw_points
     */

    w8 = (w + 0x07) >> 3;

    for (j = 0; j < h; j++)
    {
        rx = x;
        for (i = 0; i < w8; i++)
        {
            gc->d.lld->draw_points(gc, rx, y, *bits); 
            bits++;
            rx += 8;
        }
        y++;
    }

    IPW_TRACE_EXIT(DR2);
}

/**
 *  Draws a bitmap with clipping.
 *  @param gc   pointer to graphic context
 *  @param x    x coord of bitmap's top left corner
 *  @param y    y coord of bitmap's top left corner
 *  @param w    bitmap's width 
 *  @param h    bitmap's height
 *  @param bits bitmap bits array
 *  @param clip pointer to clip rectangle
 *  @internal
 */ 
static void 
_DrawClipBitmap(Pw_GC*        gc, 
                Pw_Coord      x, 
                Pw_Coord      y, 
                Pw_Coord      w, 
                Pw_Coord      h, 
                Pw_Byte*      bits,
                Pw_Rectangle* clip) 
{
    Pw_Coord w8;

    IPW_TRACE_ENTER(DR2);
    IPW_TRACE4(DR2, d, x, y, w, h);
    IPW_TRACE_RECT(DR2, clip);

    /*
     *  Calculate bitmap's width in bytes and clip the top of the bitmap
     */

    w8 = (w + 0x07) >> 3;       
    bits += (clip->y - y) * w8; 

    /*
     * When drawing a clipped bitmap we have two different cases:
     * 
     * 1. the visible part of bitmap is greater then 8 pixels - then
     *    we have some or none bits on left, 1 or more bytes in the
     *    middle and some or none bits on the right.
     *
     * 2. the visible part is less then or equal 8 pixels - then the
     *    visible bitmap bits can be all in one bitmap byte or can span 
     *    over two bytes.
     */ 

    if (clip->w > 8)
    {
        /* The visible part is greater then 8 pixels */

        Pw_uInt  left_bytes, right_bytes, middle_bytes;
        Pw_uInt  left_bits, right_bits, i, j;
        Pw_Coord cx;

        /* Calc bitmap's left side bytes and bits to clip */
        left_bits   = clip->x - x;
        left_bytes  = left_bits >> 3;
        left_bits  &= 0x07;

        /* Calc bitmap's right side bytes and bits to clip */
        right_bits   = x + (w8 << 3) - clip->x - clip->w;
        right_bytes  = right_bits >> 3;
        right_bits  &= 0x07;

        /* Calc bitmap's visible bytes */
        middle_bytes = w8 - left_bytes - right_bytes;

        IPW_TRACE2(DR2, d, left_bits, right_bits);
        IPW_TRACE3(DR2, d, left_bytes, right_bytes, middle_bytes);

        if (left_bits != 0)
            middle_bytes--;

        if (right_bits != 0)
        {
            right_bits = 8 - right_bits;
            middle_bytes--;
        }

        /* Set x and y to top left corner of bitmap's visible part */
        y = clip->y;
        x = clip->x - left_bits;

        /* Get left and right side visible bits mask */ 
        left_bits  = _bm_edgemask[0][left_bits];
        right_bits = _bm_edgemask[1][right_bits];

        /* Draw bitmap lines */
        for (j = 0; j < clip->h; j++)
        {
            cx = x;

            /* Skip invisible bytes on left */
            bits += left_bytes;            

            if (left_bits != 0)
            {
                /* Draw visible bits on left */
                gc->d.lld->draw_points(gc, cx, y, (*bits & left_bits));
                bits++;
                cx += 8;
            }

            /* Draw visible bytes */
            for (i = 0; i < middle_bytes; i++)
            {
                gc->d.lld->draw_points(gc, cx, y, *bits);
                bits++;
                cx += 8;
            }

            /* Draw visible bits on right */
            if (right_bits != 0)
            {
                gc->d.lld->draw_points(gc, cx, y, (*bits & right_bits));
                bits++;
            }
            /* Skip invisible bytes on right */
            bits += right_bytes; 
            y++;
        }
    }
    else 
    {
        /* The visible part is less then or equal 8 pixels */

        Pw_uInt i, mask;
        Pw_Byte left_bits, right_bits;

        /* Clip invisible bytes on left (in first visible line) */
        bits += (clip->x - x) >> 3;

        /* Calc invisible bits on left */
        left_bits = (clip->x - x) & 0x07;

        /* Calc the visible bits mask */
        mask = _bm_edgemask[1][clip->w];
        mask = (mask << 8) >> left_bits;
    
        /* Set x and y to top left corner of bitmap's visible part */
        y = clip->y;
        x = clip->x - left_bits;

        /* Get the left and right part of visible bits mask */
        left_bits  = mask >> 8;
        right_bits = mask & 0x00FF;

        IPW_TRACE2(DR2, d, left_bits, right_bits); 

        /* Draw bitmap lines */
        for (i = 0; i < clip->h; i++)
        {
            /* If left part of mask has any visible bits draw them */
            if (left_bits != 0)
                gc->d.lld->draw_points(gc, x, y, (*bits & left_bits));

            /* Next bitmap's byte */
            bits++;

            /* If right part of mask has any visible bits draw them */
            if (right_bits != 0)
                gc->d.lld->draw_points(gc, x+8, y, (*bits & right_bits));

            /* Skip invisible bytes on right and left */
            bits += w8 - 1;
            y++;
        }
    }
    IPW_TRACE_EXIT(DR2);
}

/* -------------------------------------------------------------------------- */

#if IPW_CHECK_EN(GC_STRUCT)
Pw_Bool
IPw_GCCheck(Pw_GC* gc)
{
    if (PW_NULL == gc->d.lld)
    {
        IPW_ASSERT(FALSE, ("GC (%p) lld == NULL!", gc));
        return(FALSE);
    }
    if (PW_NULL == gc->d.lld->draw_point    ||
        PW_NULL == gc->d.lld->draw_points   ||
        PW_NULL == gc->d.lld->draw_hor_line ||
        PW_NULL == gc->d.lld->draw_ver_line ||
        PW_NULL == gc->d.lld->fill_rect)
    {
        IPW_ASSERT(FALSE, ("GC (%p) some lld funcs == NULL!", gc));
        return(FALSE);
    }
    if (Pw_DrawableWindowType == gc->d.type && PW_NULL == gc->d.comp.window)
    {
        IPW_ASSERT(FALSE, ("GC (%p) window == NULL!", gc));
        return(FALSE);
    }
    if (Pw_DrawableBitmapType == gc->d.type && PW_NULL == gc->d.comp.bitmap)
    {
        IPW_ASSERT(FALSE, ("GC (%p) bitmap == NULL!", gc));
        return(FALSE);
    }
    if (PW_NULL == gc->color)
    {
        IPW_ASSERT(FALSE, ("GC (%p) color == NULL!", gc));
        return(FALSE);
    }
    if (PW_NULL != gc->font)
    {
        if (PW_NULL == gc->font->bits)
        {
            IPW_ASSERT(FALSE, ("GC (%p) font bits == NULL!", gc));
            return(FALSE);
        }
        if (PW_NULL == gc->font->info)
        {
            IPW_ASSERT(FALSE, ("GC (%p) font info == NULL!", gc));
            return(FALSE);
        }
    }
#if IPW_CHECK_EN(REGION_LIST)
    if (!IPw_RegionListCheck(&gc->clip_reg, TRUE))
    {
        IPW_ASSERT(FALSE, ("GC (%p) clip region check failed!", gc));
        return(FALSE);
    }
#endif /* IPW_CHECK_EN(REGION_LIST) */
    return(TRUE);
}
#endif /* IPW_CHECK_EN(GC_STRUCT) */

/* -------------------------------------------------------------------------- */

/**
 *  Gets the width of string @p string according 
 *  to the @p font.
 *  @param font   pointer to font  
 *  @param string string
 *  @return width of string in screen pixels
 */ 
Pw_uInt 
Pw_FontGetStringWidth(Pw_Font* font, Pw_Char* string)
{
    Pw_uInt w = 0;

    IPW_TRACE_ENTER(DR3);
    IPW_CHECK_PTR2(font, string);

    /* Sum widths of all charachters in string */

    while (*string != '\0')
    {
        w += font->info[(Pw_Byte)*string].dwx;
        string++;
    }

    IPW_TRACE_EXIT_V(DR3, d, w);
    return(w);
}

/**
 *  Draws a point.
 *  @param gc pointer to graphic context
 *  @param x  x coord 
 *  @param y  y coord 
 */
void 
Pw_DrawPoint(Pw_GC* gc, Pw_Coord x, Pw_Coord y)
{
    Pw_Region* reg;

    IPW_TRACE_ENTER(DR3);
    IPW_CHECK_PTR(gc);
    IPW_CHECK_GC(gc);
    IPW_TRACE2(DR3, d, x, y);

    /* Correct point's coords according to the graphic context origin */
    x += gc->xoff;
    y += gc->yoff;

    IPW_TRACE2(DR3, d, x, y);
    IPW_TRACE_RECT(DR3, &gc->clip_reg.bounds);

    /* Check if point is inside clip region bounds */
    if (!IPW_RECT_IS_POINT_IN(x, y, &gc->clip_reg.bounds))
    {
        IPW_TRACE_EXIT(DR3);
        return;
    }

    /* 
     * Check if the point is inside the clip region and draw it accordingly
     */

    reg = gc->clip_reg.regs;

    while (PW_NULL != reg)
    {
        IPW_TRACE_RECT(DR3, &reg->area); 
 
        if (IPW_RECT_IS_POINT_IN(x, y, &reg->area))
        {
            gc->d.lld->draw_point(gc, x, y);
            IPW_TRACE_EXIT(DR3);
            return;
        }
        reg = reg->next;
    }
    IPW_TRACE_EXIT(DR3);
}

/**
 *  Draws a rectangle.
 *  @param gc     pointer to graphic context
 *  @param x      x coord (of the top left corner) 
 *  @param y      y coord (of the top left corner)
 *  @param width  width of rectangle
 *  @param height height of rectangle
 *  @see Pw_FillRect
 */
void 
Pw_DrawRect(Pw_GC*   gc, 
            Pw_Coord x, 
            Pw_Coord y, 
            Pw_Coord w, 
            Pw_Coord h)
{
    Pw_Coord     rx2, ry2;
    Pw_Rectangle r, rc;
    Pw_Region*   reg;

    IPW_TRACE_ENTER(DR3);
    IPW_CHECK_PTR(gc);
    IPW_CHECK_GC(gc);
    IPW_TRACE4(DR3, d, x, y, w, h);

    /* Correct rectangle position according to graphic context origin */
    r.x = x + gc->xoff;
    r.y = y + gc->yoff;
    r.w = w;
    r.h = h;

    IPW_TRACE_RECT2(DR3, &r, &gc->clip_reg.bounds);

    /* Check if rectangle intersects clip region bounds */
    if (!IPW_RECT_IS_OVER(&r, &gc->clip_reg.bounds))
    {
        IPW_TRACE_EXIT(DR3);
        return;
    }

    /* Calc rectangle bottom right corner coords */
    rx2 = r.x + r.w - 1;
    ry2 = r.y + r.h - 1;

    /*
     * Check each rectangle in clip region - if our
     * rectangle intersect one of them, than draw
     * the visible part
     */

    reg = gc->clip_reg.regs;

    while (PW_NULL != reg)
    {
        IPW_TRACE_RECT(DR3, &reg->area); 

        /* Make a copy of rectangle */
        IPW_RECT_COPY(&rc, &r);

        /* 
         * If rectangle intersects current 
         * clip region rectangle than draw it 
         */ 
        if (IPw_RectIntersect(&rc, &reg->area))
        {
            IPW_TRACE_RECT(DR3, &rc); 
 
            if (IPW_RECT_IS_EQUAL(&r, &rc)) 
            {
                /* Rectangle fully visible - draw it and we are done */
                gc->d.lld->draw_ver_line(gc, r.x, r.y, ry2);
                gc->d.lld->draw_ver_line(gc, rx2, r.y, ry2);
                gc->d.lld->draw_hor_line(gc, r.x, r.y, rx2);
                gc->d.lld->draw_hor_line(gc, r.x, ry2, rx2);

                IPW_TRACE_EXIT(DR3);
                return;
            }
            else
            {
                Pw_Coord rcx2, rcy2;

                /* Calc clipped rectangle bottom right corner coords */
                rcx2 = rc.x + rc.w - 1;
                rcy2 = rc.y + rc.h - 1;

                /* Draw parts of rectangle that are visible */
                if (r.x == rc.x)
                    gc->d.lld->draw_ver_line(gc, rc.x, rc.y, rcy2);
                if (rx2 == rcx2)
                    gc->d.lld->draw_ver_line(gc, rcx2, rc.y, rcy2);
                if (r.y == rc.y)
                    gc->d.lld->draw_hor_line(gc, rc.x, rc.y, rcx2);
                if (ry2 == rcy2)
                    gc->d.lld->draw_hor_line(gc, rc.x, rcy2, rcx2);
            }
        }
        reg = reg->next;
    }
    IPW_TRACE_EXIT(DR3);
}

/**
 *  Draws a filled rectangle.
 *  @param gc     pointer to graphic context
 *  @param x      x coord (of the top left corner) 
 *  @param y      y coord (of the top left corner)
 *  @param width  width of rectangle
 *  @param height height of rectangle
 *  @see Pw_DrawRect
 */
void 
Pw_FillRect(Pw_GC*   gc, 
            Pw_Coord x, 
            Pw_Coord y, 
            Pw_Coord w, 
            Pw_Coord h)
{
    Pw_Rectangle r, rc;
    Pw_Region*   reg;

    IPW_TRACE_ENTER(DR3);
    IPW_CHECK_PTR(gc);
    IPW_CHECK_GC(gc);
    IPW_TRACE4(DR3, d, x, y, w, h);

    /* Correct rectangle position according to graphic context origin */
    r.x = x + gc->xoff;
    r.y = y + gc->yoff;
    r.w = w;
    r.h = h;

    IPW_TRACE_RECT2(DR3, &r, &gc->clip_reg.bounds);

    /*
     * Check if rectangle's bounding box intersects the clip region
     * bounding box
     */ 
    if (!IPW_RECT_IS_OVER(&r, &gc->clip_reg.bounds))
    {
        IPW_TRACE_EXIT(DR3);
        return;
    }

    /*
     * Check each rectangle in clip region - if our
     * rectangle intersect one of them, than draw
     * the visible part
     */

    reg = gc->clip_reg.regs;

    while (PW_NULL != reg)
    {
        IPW_TRACE_RECT(DR3, &reg->area);

        /* Make a copy of rectangle */
        IPW_RECT_COPY(&rc, &r);

        /* Intersect the rectangle with current clip region rectangle */
        if (IPw_RectIntersect(&rc, &reg->area))
        {
            IPW_TRACE_RECT(DR3, &rc); 

            /* Draw the intersected rectangle */
            gc->d.lld->fill_rect(gc, rc.x, rc.y, rc.w, rc.h);

            /* 
             * If intersected rectangle equals the 
             * original rectangle - we are done
             */ 
            if (IPW_RECT_IS_EQUAL(&r, &rc)) 
            {
                IPW_TRACE_EXIT(DR3);
                return;
            }
        }
        reg = reg->next;
    }
    IPW_TRACE_EXIT(DR3);
}

/**
 *  Draws a line.
 *  @param gc pointer to graphic context
 *  @param x1 x coord (of start point) 
 *  @param y1 y coord (of start point)
 *  @param x2 x coord (of end point) 
 *  @param y2 y coord (of end point)
 */
void 
Pw_DrawLine(Pw_GC*   gc, 
            Pw_Coord x1, 
            Pw_Coord y1, 
            Pw_Coord x2, 
            Pw_Coord y2)
{
    Pw_Region* reg;

    IPW_TRACE_ENTER(DR3);
    IPW_CHECK_PTR(gc);
    IPW_CHECK_GC(gc);
    IPW_TRACE4(DR3, d, x1, y1, x2, y2);

    /* Correct the line coords according to graphic context origin */
    x1 += gc->xoff;
    x2 += gc->xoff;
    y1 += gc->yoff;
    y2 += gc->yoff;

    IPW_TRACE4(DR3, d, x1, y1, x2, y2);
    IPW_TRACE_RECT(DR3, &gc->clip_reg.bounds);

    /* If line outside clip region bounds return */
    if ((IPw_RectOutCode(&gc->clip_reg.bounds, x1, y1) &
         IPw_RectOutCode(&gc->clip_reg.bounds, x2, y2)) != 0)
    {
        IPW_TRACE_EXIT(DR3);
        return;
    }

    reg = gc->clip_reg.regs;

    /* 
     * Check if line is an horizontal or vertical line -
     * if it is use faster drawing and clipping functions
     */  
    if (x1 == x2)
    {
        /* Horizontal line */

        /*
         * Check each rectangle in clip region - if our
         * line intersect one of them, than draw the
         * visible part
         */
        while (PW_NULL != reg)
        {
            IPW_TRACE_RECT(DR3, &reg->area); 

            if (_DrawClipVerLine(gc, x1, y1, y2, &reg->area))
            {
                IPW_TRACE_EXIT(DR3);
                return;
            }
            reg = reg->next;
        }
    }
    else if (y1 == y2) 
    {
        /* Vertical line */

        /*
         * Check each rectangle in clip region - if our
         * line intersect one of them, than draw the
         * visible part
         */
        while (PW_NULL != reg)
        {
            IPW_TRACE_RECT(DR3, &reg->area);

            if (_DrawClipHorLine(gc, x1, y1, x2, &reg->area))
            {
                IPW_TRACE_EXIT(DR3);
                return;
            }
            reg = reg->next;
        }
    }
    else 
    {
        /* Universal line */

        Pw_uInt c1, c2;

        /*
         * Check each rectangle in clip region - if our
         * line intersect one of them, than draw the
         * visible part
         */
        while (PW_NULL != reg)
        {
            IPW_TRACE_RECT(DR3, &reg->area); 

            /* 
             * Calc the line position according 
             * to the current clip region rectangle
             */ 
            c1 = IPw_RectOutCode(&reg->area, x1, y1);
            c2 = IPw_RectOutCode(&reg->area, x2, y2);

            IPW_TRACE2(DR3, d, c1, c2);

            if ((c1 == 0) && (c2 == 0)) 
            {
                /* Line is inside current clip rectangle */
 
                _DrawLine(gc, x1, y1, x2, y2);
                IPW_TRACE_EXIT(DR3);
                return;
            }
            else if ((c1 & c2) == 0)
            {
                /* Line intersects current clip rectangle */

                _DrawClipLine(gc, x1, y1, x2, y2, &reg->area);
            }
            reg = reg->next;
        }
    }
    IPW_TRACE_EXIT(DR3);
}

/**
 *  Draws a circle.
 *  @param gc pointer to graphic context
 *  @param x  circle center x coord 
 *  @param y  circle center y coord
 *  @param r  circle radius 
 *  @todo use faster functions
 *  @see Pw_FillCircle
 */
void 
Pw_DrawCircle(Pw_GC*   gc, 
              Pw_Coord x, 
              Pw_Coord y, 
              Pw_Coord r) 
{
    IPW_TRACE_ENTER(DR3);

    Pw_DrawEllipse(gc, x, y, r, r);

    IPW_TRACE_EXIT(DR3);
}

/**
 *  Draws a filled circle.
 *  @param gc pointer to graphic context
 *  @param x  circle center x coord 
 *  @param y  circle center y coord
 *  @param r  circle radius 
 *  @todo use faster functions
 *  @see Pw_DrawCircle
 */
void 
Pw_FillCircle(Pw_GC*   gc, 
              Pw_Coord x, 
              Pw_Coord y, 
              Pw_Coord r) 
{
    IPW_TRACE_ENTER(DR3);

    Pw_FillEllipse(gc, x, y, r, r);

    IPW_TRACE_EXIT(DR3);
}

/**
 *  Draws an ellipse.
 *  @param gc pointer to graphic context
 *  @param x  ellipse center x coord 
 *  @param y  ellipse center y coord
 *  @param rx ellipse x radius 
 *  @param ry ellipse y radius 
 *  @see Pw_FillEllipse
 *  @see Pw_DrawCircle
 *  @see Pw_FillCircle
 */
void 
Pw_DrawEllipse(Pw_GC*   gc, 
               Pw_Coord x, 
               Pw_Coord y, 
               Pw_Coord rx, 
               Pw_Coord ry)
{
    Pw_Rectangle r, rc;
    Pw_Region*   reg;

    IPW_TRACE_ENTER(DR3);
    IPW_CHECK_PTR(gc);
    IPW_CHECK_GC(gc);
    IPW_TRACE4(DR3, d, x, y, rx, ry);

    /* Correct the ellipse position according to graphic context origin */
    x += gc->xoff;
    y += gc->yoff;

    IPW_TRACE2(DR3, d, x, y);

    /* Calculate the ellipse bounding box */
    r.x = x - rx;
    r.y = y - ry;
    r.w = rx * 2 + 1;
    r.h = ry * 2 + 1;

    IPW_TRACE_RECT2(DR3, &r, &gc->clip_reg.bounds);

    /* 
     * Check if the ellipse bounding box intersects 
     * the clip region bounding box 
     */
    if (!IPW_RECT_IS_OVER(&r, &gc->clip_reg.bounds))
    {
        IPW_TRACE_EXIT(DR3);
        return;
    }

    /*
     * Check each rectangle in clip region - if our
     * ellipse intersect one of them, than draw the
     * visible part
     */

    reg = gc->clip_reg.regs;

    while (PW_NULL != reg)
    {
        IPW_TRACE_RECT(DR3, &reg->area);

        /* Make a copy of ellipse bounding box */
        IPW_RECT_COPY(&rc, &r);

        /* 
         * Intersect the ellipse bounding box with current
         * clip region rectangle
         */ 
        if (IPw_RectIntersect(&rc, &reg->area))
        {
            IPW_TRACE_RECT(DR3, &rc); 

            if (IPW_RECT_IS_EQUAL(&r, &rc))
            {
                /* Ellipse fully visible */

                _DrawEllipse(gc, x, y, rx, ry, FALSE);
                IPW_TRACE_EXIT(DR3);
                return;
            }
            else
            {
                /* Ellipse partially visible (draw with clipping) */

                _DrawClipEllipse(gc, x, y, rx, ry, FALSE, &rc);
            }
        }
        reg = reg->next;
    }
    IPW_TRACE_EXIT(DR3);
}

/**
 *  Draws a filled ellipse.
 *  @param gc pointer to graphic context
 *  @param x  ellipse center x coord 
 *  @param y  ellipse center y coord
 *  @param rx ellipse x radius 
 *  @param ry ellipse y radius 
 *  @see Pw_DrawEllipse
 *  @see Pw_DrawCircle
 *  @see Pw_FillCircle
 */
void 
Pw_FillEllipse(Pw_GC*   gc, 
               Pw_Coord x, 
               Pw_Coord y, 
               Pw_Coord rx, 
               Pw_Coord ry)
{
    Pw_Rectangle r, rc;
    Pw_Region*   reg;

    IPW_TRACE_ENTER(DR3);
    IPW_CHECK_PTR(gc);
    IPW_CHECK_GC(gc);
    IPW_TRACE4(DR3, d, x, y, rx, ry);

    /* Correct the ellipse position according to graphic context origin */
    x += gc->xoff;
    y += gc->yoff;

    IPW_TRACE2(DR3, d, x, y);

    /* Calculate the ellipse bounding box */
    r.x = x - rx;
    r.y = y - ry;
    r.w = rx * 2 + 1;
    r.h = ry * 2 + 1;

    IPW_TRACE_RECT2(DR3, &r, &gc->clip_reg.bounds);

    /* 
     * Check if the ellipse bounding box intersects 
     * the clip region bounding box 
     */
    if (!IPW_RECT_IS_OVER(&r, &gc->clip_reg.bounds))
    {
        IPW_TRACE_EXIT(DR3);
        return;
    }

    /*
     * Check each rectangle in clip region - if our
     * ellipse intersect one of them, than draw the
     * visible part
     */

    reg = gc->clip_reg.regs;

    while (PW_NULL != reg)
    {
        IPW_TRACE_RECT(DR3, &reg->area);

        /* Make a copy of ellipse bounding box */
        IPW_RECT_COPY(&rc, &r);

        /* 
         * Intersect the ellipse bounding box with current
         * clip region rectangle
         */ 
        if (IPw_RectIntersect(&rc, &reg->area))
        {
            IPW_TRACE_RECT(DR3, &rc); 

            if (IPW_RECT_IS_EQUAL(&r, &rc))  
            {
                /* Ellipse fully visible */

                _DrawEllipse(gc, x, y, rx, ry, TRUE);
                IPW_TRACE_EXIT(DR3);
                return;
            }
            else 
            {
                /* Ellipse partially visible (draw with clipping) */

                _DrawClipEllipse(gc, x, y, rx, ry, TRUE, &rc);
            }
        }
        reg = reg->next;
    }
    IPW_TRACE_EXIT(DR3);
}

/**
 *  Draws a bitmap.
 *  @param gc     pointer to graphic context
 *  @param x      x coord of bitmap top left corner
 *  @param y      y coord of bitmap top left corner
 *  @param bitmap pointer to bitmap
 */
void 
Pw_DrawBitmap(Pw_GC*     gc, 
              Pw_Coord   x, 
              Pw_Coord   y, 
              Pw_Bitmap* bitmap)
{
    Pw_Rectangle r, rc;
    Pw_Region*   reg;

    IPW_TRACE_ENTER(DR3);
    IPW_CHECK_PTR(gc);
    IPW_CHECK_PTR(bitmap);
    IPW_CHECK_BITMAP(bitmap);
    IPW_CHECK_GC(gc);

    IPW_TRACE2(DR3, d, x, y);

    /* Correct the bitmap position according to clip region bounds */
    r.x = x + gc->xoff;
    r.y = y + gc->yoff;
    r.w = bitmap->width;
    r.h = bitmap->height;

    IPW_TRACE_RECT2(DR3, &r, &gc->clip_reg.bounds);

    /* 
     * Check if the bitmap bounding box intersects clip
     * region bounding box
     */ 
    if (!IPW_RECT_IS_OVER(&r, &gc->clip_reg.bounds))
    {
        IPW_TRACE_EXIT(DR3);
        return;
    }

    /*
     * Check each rectangle in clip region - if our
     * bitmap intersect one of them, than draw the
     * visible part
     */

    reg = gc->clip_reg.regs;

    while (PW_NULL != reg)
    {
        IPW_TRACE_RECT(DR3, &reg->area);

        /* Make a copy of bitmap bounding box */
        IPW_RECT_COPY(&rc, &r);

        /* Intersect bitmap's bounds with current clip region rectangle */
        if (IPw_RectIntersect(&rc, &reg->area))
        {
            IPW_TRACE_RECT(DR3, &rc); 

            if (IPW_RECT_IS_EQUAL(&rc, &r)) 
            {
                /* Bitmap fully visible */

                _DrawBitmap(gc, r.x, r.y, r.w, r.h, bitmap->bits); 
                IPW_TRACE_EXIT(DR3);
                return;
            }
            else 
            {
                /* Bitmap partially visible - draw with clipping */

                _DrawClipBitmap(gc, r.x, r.y, r.w, r.h, bitmap->bits, &rc); 
            }
        }
        reg = reg->next;
    }
    IPW_TRACE_EXIT(DR3);
} 

/**
 *  Draws a string.
 *  @param gc     pointer to graphic context
 *  @param x      x coord of string box top left corner
 *  @param y      y coord of string box left corner
 *  @param string string to be drawn
 */
void 
Pw_DrawString(Pw_GC*   gc, 
              Pw_Coord x, 
              Pw_Coord y, 
              Pw_Char* string)
{
    Pw_Rectangle     r, rc;
    Pw_Coord         rx;
    Pw_FontCharInfo* cinfo;
    Pw_Region*       reg;

    IPW_TRACE_ENTER(DR3);
    IPW_CHECK_PTR2(gc, string);
    IPW_CHECK_GC(gc);
    IPW_TRACE3M(DR3, d, x, d, y, s, string);

    if (gc->font == PW_NULL)
    {
        IPW_ASSERT(FALSE, ("GC font is NULL"));
        return;
    }

    /* 
     * Correct the string position according to graphic context origin
     * and calculate its bounding box
     */
    r.x = x + gc->xoff;
    r.y = y + gc->yoff - gc->font->ascent;
    r.w = Pw_GCGetStringWidth(gc, string);
    r.h = gc->font->ascent + gc->font->descent;

    IPW_TRACE_RECT2(DR3, &r, &gc->clip_reg.bounds);

    /* 
     * If the string bounding box does not intersect clip region
     * bounding box return
     */ 
    if (!IPW_RECT_IS_OVER(&r, &gc->clip_reg.bounds))
    {
        IPW_TRACE_EXIT(DR3);
        return;
    }

    /*
     *  Check if the whole string fits in one clip rectangle and
     *  draw all charachters without clipping
     */

    reg = gc->clip_reg.regs;

    while (PW_NULL != reg)
    {
        IPW_TRACE_RECT(DR3, &reg->area);

        /* Make a copy of string bounding box */
        IPW_RECT_COPY(&rc, &r);

        /* Intersect string's bounds with current clip region rectangle */
        if (IPw_RectIntersect(&rc, &reg->area))
        {
            IPW_TRACE_RECT(DR3, &rc); 

            if (IPW_RECT_IS_EQUAL(&rc, &r)) 
            {
                /* 
                 * The whole string fits in this clip rectangle -
                 * draw all charachters without clipping
                 */
                rx = x + gc->xoff;
                while (*string != '\0')
                {
                    /* Get font info for current charachter */
                    if ((Pw_Byte)*string < gc->font->chars_num)
                        cinfo = &gc->font->info[(Pw_Byte)*string];
                    else
                        cinfo = &gc->font->info[0];

                    /* Draw current charachter bitmap */
                    _DrawBitmap(gc, rx + cinfo->bbx, 
                                    y + gc->yoff - (cinfo->bby + cinfo->bbh), 
                                    cinfo->bbw, 
                                    cinfo->bbh, 
                                    &gc->font->bits[cinfo->bits_offset]);

                    rx += cinfo->dwx;
                    string++;
                }
                IPW_TRACE_EXIT(DR3);
                return;
            }
        }
        reg = reg->next;
    }

    /*
     *  The whole string didn't fit in one clip rectangle - we have to
     *  clip and draw charachter by charachter
     */ 
    rx = x + gc->xoff;
    while (*string != '\0')
    {
        /* Get font info for current charachter */
        if ((Pw_Byte)*string < gc->font->chars_num)
            cinfo = &gc->font->info[(Pw_Byte)*string];
        else
            cinfo = &gc->font->info[0];

        IPW_TRACE1(DR3, c, *string);

        /* Set the current charachter bounding box */
        r.x = rx + cinfo->bbx; 
        r.y = y + gc->yoff - (cinfo->bby + cinfo->bbh);
        r.w = cinfo->bbw; 
        r.h = cinfo->bbh;
    
        IPW_TRACE1(DR3, d, rx);
        IPW_TRACE_RECT2(DR3, &r, &gc->clip_reg.bounds);

        /* 
         * If the charachter bounding box intersect clip region
         * bounding box then try to draw it 
         */ 
        if (IPW_RECT_IS_OVER(&r, &gc->clip_reg.bounds))
        {
            reg = gc->clip_reg.regs;

            /* Try to draw the current charachter inside clip region */
            while (PW_NULL != reg)
            {
                IPW_TRACE_RECT(DR3, &reg->area);

                /* Make a copy of charachter bounding box */
                IPW_RECT_COPY(&rc, &r);

                /* 
                 * Intersect charachter's bounds with 
                 * current clip region rectangle 
                 */
                if (IPw_RectIntersect(&rc, &reg->area))
                {
                    IPW_TRACE_RECT(DR3, &rc);
              
                    if (IPW_RECT_IS_EQUAL(&rc, &r))
                    {
                        /* Charachter fully visible */
                        _DrawBitmap(gc, r.x, r.y, r.w, r.h,  
                            &gc->font->bits[cinfo->bits_offset]);
                        break;
                    }
                    else
                    {
                        /* Charachter partially visible - draw with clipping */
                        _DrawClipBitmap(gc, r.x, r.y, r.w, r.h,  
                            &gc->font->bits[cinfo->bits_offset], &rc);
                    }
                }
                reg = reg->next;
            }    
        }
        rx += cinfo->dwx;
        string++;
    }
    IPW_TRACE_EXIT(DR3);
}

/*---------------------------------------------------------------------------
// end of pw_draw.c 
//--------------------------------------------------------------------------- */
