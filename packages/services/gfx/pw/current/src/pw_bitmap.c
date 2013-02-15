/*===========================================================================
//
//        pw_bitmap.c
//
//        PW graphic library bitmap code 
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
static const Pw_Byte _bm_bitmask[2][8] = 
{
    {127, 191, 223, 239, 247, 251, 253, 254},
    {128, 64,  32,  16,  8,   4,   2,   1  }
};

/** An array of bitmasks. Used for manipulating bits in bitmaps. */
static const Pw_Byte _bm_edgemask[2][9] =
{
    {0x00, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00},
    {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF}
};

/** Bitmap low level drawing functions. */
static Pw_LLD _bitmap_lld;

/* -------------------------------------------------------------------------- */

/**
 *  Draws a point.
 *  @param gc pointer to bitmap graphic context
 *  @param x  point's x coord
 *  @param y  point's y coord 
 *  @internal
 */ 
static void 
_BitmapDrawPoint(Pw_GC* gc, Pw_Coord x, Pw_Coord y)
{
    IPW_TRACE_ENTER(BM1);
    IPW_CHECK_PTR(gc);

    IPw_BitmapDrawPoint(gc->d.comp.bitmap, x, y, 
        ((gc->color == pw_black_pixel) ? 0 : 1));

    IPW_TRACE_EXIT(BM1);
}

/**
 *  Draws up to 8 points.
 *  For each set bit in bits the point is drawn at (bit_idx + x, y)
 *  @param gc   pointer to bitmap graphic context
 *  @param x    points start x coord
 *  @param y    points y coord
 *  @param bits byte defining wich points (of 8) to be drawn
 *  @internal
 */ 
static void 
_BitmapDrawPoints(Pw_GC*   gc, 
                  Pw_Coord x, 
                  Pw_Coord y, 
                  Pw_Byte  bits) 
{
    IPW_TRACE_ENTER(BM1);
    IPW_CHECK_PTR(gc);

    IPw_BitmapDrawPoints(gc->d.comp.bitmap, x, y, bits,
        ((gc->color == pw_black_pixel) ? 0 : 1));

    IPW_TRACE_EXIT(BM1);
}

/**
 *  Draws a horizontal line.
 *  @param gc pointer to bitmap graphic context
 *  @param x1 line start point x coord
 *  @param y1 line start point y coord 
 *  @param x2 line end point x coord
 *  @internal
 */ 
static void 
_BitmapDrawHorLine(Pw_GC*   gc, 
                   Pw_Coord x1, 
                   Pw_Coord y1, 
                   Pw_Coord x2)
{
    IPW_TRACE_ENTER(BM1);
    IPW_CHECK_PTR(gc);

    IPw_BitmapDrawHorLine(gc->d.comp.bitmap, x1, y1, x2,
        ((gc->color == pw_black_pixel) ? 0 : 1));

    IPW_TRACE_EXIT(BM1);
}

/**
 *  Draws a vertical line.
 *  @param gc pointer to bitmap graphic context
 *  @param x1 line start point x coord
 *  @param y1 line start point y coord 
 *  @param y2 line end point y coord
 *  @internal
 */
static void 
_BitmapDrawVerLine(Pw_GC*   gc, 
                   Pw_Coord x1, 
                   Pw_Coord y1, 
                   Pw_Coord y2)
{
    IPW_TRACE_ENTER(BM1);
    IPW_CHECK_PTR(gc);

    IPw_BitmapDrawVerLine(gc->d.comp.bitmap, x1, y1, y2,
        ((gc->color == pw_black_pixel) ? 0 : 1));

    IPW_TRACE_EXIT(BM1);
}

/**
 *  Fills an rectangular area.
 *  @param gc pointer to bitmap graphic context
 *  @param x  top left corner x coord
 *  @param y  top left corner y coord 
 *  @param w  width
 *  @param h  height
 *  @internal
 */
static void 
_BitmapFillRect(Pw_GC*   gc, 
                Pw_Coord x, 
                Pw_Coord y, 
                Pw_Coord w, 
                Pw_Coord h)
{
    IPW_TRACE_ENTER(BM1);
    IPW_CHECK_PTR(gc);

    IPw_BitmapFillRect(gc->d.comp.bitmap, x, y, w, h,
        ((gc->color == pw_black_pixel) ? 0 : 1));

    IPW_TRACE_EXIT(BM1);
}

/* -------------------------------------------------------------------------- */

/**
 *  Draws a point.
 *  @param bitmap pointer to bitmap
 *  @param x      point's x coord
 *  @param y      point's y coord 
 *  @param mode   draw mode
 *  @internal
 */ 
void 
IPw_BitmapDrawPoint(Pw_Bitmap* bitmap, 
                    Pw_Coord   x, 
                    Pw_Coord   y, 
                    Pw_uInt    mode)
{
    IPW_TRACE_ENTER(BM2);
    IPW_CHECK_PTR(bitmap);
    IPW_CHECK_BITMAP(bitmap);
    IPW_TRACE3(BM1, d, x, y, mode);

    /*
     *  calculate the points location in bitmap bits array
     *  and the set or reset it using the predefined bitmasks
     */

    if (mode >= 1)
    {
        bitmap->bits[((x >> 3) + bitmap->width8 * y)] |= 
            _bm_bitmask[1][(x & 0x07)]; 
    }
    else
    {
        bitmap->bits[((x >> 3) + bitmap->width8 * y)] &= 
            _bm_bitmask[0][(x & 0x07)]; 
    }

    IPW_TRACE_EXIT(BM2);
}

/**
 *  Draws up to 8 points.
 *  For each set bit in bits the point is drawn at (bit_idx + x, y)
 *  @param bitmap pointer to bitmap
 *  @param x      points start x coord
 *  @param y      points y coord
 *  @param bits   byte defining wich points (of 8) to be drawn 
 *  @param mode   draw mode
 *  @internal
 */ 
void 
IPw_BitmapDrawPoints(Pw_Bitmap* bitmap,
                     Pw_Coord   x, 
                     Pw_Coord   y, 
                     Pw_Byte    bits, 
                     Pw_uInt    mode)
{
    Pw_uInt16 mask;
    Pw_Byte*  bm_bits;

    IPW_TRACE_ENTER(BM2);
    IPW_CHECK_PTR(bitmap);
    IPW_CHECK_BITMAP(bitmap);
    IPW_TRACE4(BM1, d, x, y, bits, mode);

    /* 
     *  calculate the location of the first byte 
     *  (can be 2 depending of the x coord) to mask 
     *  with specified bits (or part of) and then
     *  calculate the 16 bit mask to use 
     */
 
    bm_bits = bitmap->bits + (x >> 3) + bitmap->width8 * y;
    mask = bits;
    mask = (mask << 8) >> (x & 0x07);

#ifdef IPW_USE_ASSERTS
    if (((x >> 3) + bitmap->width8 * y) == 
        ((bitmap->width8 * bitmap->height) -1))
    {
        IPW_ASSERT((mask & 0x00FF) == 0, 
            ("Bitmap edge case - mask = %x!", mask));
    }
#endif /* IPW_USE_ASSERTS */

    /*
     *  mask the first byte with mask's high byte and test 
     *  the mask's low byte - if it equals 0x00 (or 0xFF in 
     *  the second case) do nothing - the clipping layer 
     *  guarantees only that the first bit and the bits wich 
     *  are set to 1 fall inside this bitmap.
     */ 
    if (mode >= 1) 
    {
        *bm_bits |= mask >> 8;
        mask &= 0x00FF;
        if (mask != 0x00)
        {
            bm_bits++;
            *bm_bits |= mask;
        }
    }
    else 
    {
        mask ^= 0xFFFF;
        *bm_bits &= mask >> 8;
        mask &= 0x00FF;
        if (mask != 0xFF)
        {
            bm_bits++; 
            *bm_bits &= mask;
        }
    }
    IPW_TRACE_EXIT(BM2);
}

/**
 *  Draws a horizontal line.
 *  @param bitmap pointer to bitmap
 *  @param x1     line start point x coord
 *  @param y1     line start point y coord 
 *  @param x2     line end point x coord
 *  @param mode   draw mode
 *  @internal
 */ 
void 
IPw_BitmapDrawHorLine(Pw_Bitmap* bitmap,
                      Pw_Coord   x1,
                      Pw_Coord   y1,
                      Pw_Coord   x2,
                      Pw_uInt    mode)
{
    Pw_Int   i, start_bits, stop_bits, width;
    Pw_Byte* bits;

    IPW_TRACE_ENTER(BM2);
    IPW_CHECK_PTR(bitmap);
    IPW_CHECK_BITMAP(bitmap);
    IPW_TRACE4(BM1, d, x1, y1, x2, mode);

    /*
     * calculate the start byte of the line in bitmap's bits
     * array, the line start bits in start byte and the width
     * of the line. Then check the width and choose the drawing
     * strategy. If width > 8 we have some (or none) start
     * bits, some (or none) bytes in the middle and some (or none)
     * stop bits. If width < 8 the line should fit into 1 or 2 bytes. 
     */

    bits = bitmap->bits + (x1 >> 3) + (bitmap->width8 * y1);
    start_bits = x1 & 0x07; 
    width = x2 - x1 + 1;

    if (width > 8)
    {
        width -= ((8 - start_bits) & 0x07);
        if (width > 0)
        {
            stop_bits = width & 0x07;
            width >>= 3;
        }
        else
        {
            stop_bits = 0;
            width = 0;
        }

        if (mode >= 1)
        {
            if (start_bits != 0)
                *bits++ |= _bm_edgemask[0][start_bits];
    
            for (i = 0; i < width; i++)
                *bits++ = 0xFF;
    
            if (stop_bits != 0)
                *bits |= _bm_edgemask[1][stop_bits];
        }
        else
        {
            if (start_bits != 0)
                *bits++ &= _bm_edgemask[1][start_bits];

            for (i = 0; i < width; i++)
                *bits++ = 0x00;

            if (stop_bits != 0)
                *bits &= _bm_edgemask[0][stop_bits];
        }
    }
    else
    {
        Pw_uInt16 mask = _bm_edgemask[1][width]; 

        mask = (mask << 8) >> start_bits;

        if (mode >= 1)
        {
            *bits |= (mask >> 8);
            bits++;
            *bits |= (mask & 0x00FF);
        }
        else
        {
            *bits &= (0x00FF ^ (mask >> 8)); 
            bits++;
            *bits &= (0x00FF ^ (mask & 0x00FF));
        }
    }
    IPW_TRACE_EXIT(BM2);
}

/**
 *  Draws a horizontal line.
 *  @param bitmap pointer to bitmap 
 *  @param x1     line start point x coord
 *  @param y1     line start point y coord 
 *  @param y2     line end point y coord
 *  @param mode   draw mode
 *  @internal
 */
void 
IPw_BitmapDrawVerLine(Pw_Bitmap* bitmap,
                      Pw_Coord   x1,
                      Pw_Coord   y1,
                      Pw_Coord   y2,
                      Pw_uInt    mode)
{
    Pw_uInt   i, step, len;
    Pw_uInt16 mask;
    Pw_Byte*  bits;

    IPW_TRACE_ENTER(BM2);
    IPW_CHECK_PTR(bitmap);
    IPW_CHECK_BITMAP(bitmap);
    IPW_TRACE4(BM1, d, x1, y1, y2, mode);

    /*
     *  calculate the line start byte in bitmap's bits
     *  array, get the step (num of bytes between the
     *  bytes that should be masked to draw the line -
     *  ie the byte width of bitmap). Then mask the line 
     *  bytes with calculated mask.
     */

    bits = bitmap->bits + (x1 >> 3) + (y1 * bitmap->width8);
    step = bitmap->width8;
    len  = (Pw_uInt)(y2 - y1) + 1;

    if (mode >= 1)
    {
        mask = _bm_bitmask[1][(x1 & 0x07)];

        for (i = 0; i < len; i++)
        {
            *bits |= mask;    
            bits += step;
        }
    }
    else
    {
        mask = _bm_bitmask[0][(x1 & 0x07)];

        for (i = 0; i < len; i++)
        {
            *bits &= mask;    
            bits += step;
        }
    }
    IPW_TRACE_EXIT(BM2);
} 

/**
 *  Fills an rectangular area.
 *  @param bitmap pointer to bitmap
 *  @param x      top left corner x coord
 *  @param y      top left corner y coord 
 *  @param w      width
 *  @param h      height
 *  @param mode   fill mode
 *  @internal
 */
void 
IPw_BitmapFillRect(Pw_Bitmap* bitmap,
                   Pw_Coord   x,
                   Pw_Coord   y,
                   Pw_Coord   w,
                   Pw_Coord   h,
                   Pw_uInt    mode)
{
    Pw_Int   i, j, start_bits, stop_bits, width, step;
    Pw_Byte  start_mask, stop_mask;
    Pw_Byte* bits;

    IPW_TRACE_ENTER(BM2);
    IPW_CHECK_PTR(bitmap);
    IPW_CHECK_BITMAP(bitmap);
    IPW_TRACE5(BM1, d, x, y, w, h, mode);

    /*
     *  this is like drawing a number (h)
     *  of horizontal lines of width w.
     *  See IPw_BitmapDrawHorLine for comments
     *  on drawing horizontal lines
     */

    bits = bitmap->bits + (x >> 3) + bitmap->width8 * y;
    start_bits = x & 0x07; 

    if (w > 8)
    {
        width = w - ((8 - start_bits) & 0x07);
        if (width > 0)
        {
            stop_bits = width & 0x07;
            width >>= 3;
        }
        else
        {
            stop_bits = 0;
            width = 0;
        }

        step = bitmap->width8 - width;
        if (start_bits != 0)
            step--;

        if (mode >= 1)
        {
            start_mask = _bm_edgemask[0][start_bits];
            stop_mask  = _bm_edgemask[1][stop_bits];
            for (j = 0; j < h; j++)
            {
                if (start_bits != 0)
                    *bits++ |= start_mask;

                for (i = 0; i < width; i++)
                    *bits++ = 0xFF;

                if (stop_bits != 0)
                    *bits |= stop_mask;
                bits += step;
            }
        }
        else
        {
            start_mask = _bm_edgemask[1][start_bits];
            stop_mask  = _bm_edgemask[0][stop_bits];
            for (j = 0; j < h; j++)
            {    
                if (start_bits != 0)
                    *bits++ &= start_mask;

                for (i = 0; i < width; i++)
                    *bits++ = 0x00;

                if (stop_bits != 0)
                    *bits &= stop_mask;
                bits += step;
            }    
        }
    }
    else
    {
        Pw_uInt16 mask; 

        if (mode >= 1)
        {
            mask = _bm_edgemask[1][w];

            mask = (mask << 8) >> start_bits;

            start_mask = mask >> 8;
            stop_mask  = (mask & 0x00FF);

            step = bitmap->width8 - 1;

            for (i = 0; i < h; i++)
            {
                *bits |= start_mask;
                bits++;
                *bits |= stop_mask;
                bits += step;
            }
        }
        else
        {
            mask = _bm_edgemask[1][w];

            mask = (mask << 8) >> start_bits;

            start_mask = 0x00FF ^ (mask >> 8);
            stop_mask  = 0x00FF ^ (mask & 0x00FF);

            step = bitmap->width8 - 1;

            for (i = 0; i < h; i++)
            {
                *bits &= start_mask;
                bits++;
                *bits &= stop_mask;
                bits += step;
            }
        }
    }
    IPW_TRACE_EXIT(BM2);
}

/**
 *  Compares the area @p area of bitmap @p bm1 and @p bm2.
 *  When a different byte is found the function @p diff_func
 *  is called and @p bm2 is set to equal @p bm1 at that byte.
 *  NOTE: the bitmaps @bm1 and @bm2 must be of same size.
 *  @param bm1       pointer to bitmap 1
 *  @param bm2       pointer to bitmap 2
 *  @param area      pointer to rectangular area
 *  @param diff_func function to call on different bytes
 *  @internal 
 */ 
void 
IPw_BitmapDiff(Pw_Bitmap*    bm1,
               Pw_Bitmap*    bm2,
               Pw_Rectangle* area,
               void (*diff_func)(Pw_uInt baddr, Pw_Byte dbyte))
{
    Pw_uInt  i, j, baddr;
    Pw_uInt  left_bytes, right_bytes, width;
    Pw_Byte* bits1;
    Pw_Byte* bits2;

    IPW_TRACE_ENTER(BM2);
    IPW_CHECK_PTR4(bm1, bm2, area, diff_func);
    IPW_CHECK_BITMAP(bm1);
    IPW_CHECK_BITMAP(bm2);
    IPW_CHECK_COND((bm1->width == bm2->width) && (bm1->height == bm2->height));
    IPW_TRACE_RECT(BM1, area);

    /*
     *  calculate the start line byte, the number of bytes to
     *  skip on left, the number of bytes to skip on rigth and
     *  the number of bytes to compare in one line (width). 
     *  The comparing goes like this: first skip bytes on top,
     *  then compare area->h lines (skip bytes on left, compare
     *  bytes in the middle (width) and skip bytes on right)
     */

    baddr = area->y * bm1->width8;
    bits1 = bm1->bits + baddr;
    bits2 = bm2->bits + baddr;

    left_bytes  = area->x >> 3;
    right_bytes = bm1->width8 - ((area->x + area->w + 0x07) >> 3);
    width       = bm1->width8 - left_bytes - right_bytes;

    IPW_TRACE4(BM1, d, bm1->width8, left_bytes, width, right_bytes);

    for (i = 0; i < area->h; i++)
    {
        bits1 += left_bytes;
        bits2 += left_bytes;
        baddr += left_bytes;
        for (j = 0; j < width; j++)
        {
            if (*bits1 != *bits2)
            {
                diff_func(baddr, *bits1);
                *bits2 = *bits1;
            }
            bits1++;
            bits2++;
            baddr++;
        }
        bits1 += right_bytes;
        bits2 += right_bytes;
        baddr += right_bytes;
    }
    IPW_TRACE_EXIT(BM2);
}

/**
 *  Compares the area @p area of bitmap @p bm1 and @p bm2.
 *  When a different pixel is found the function @p diff_func
 *  is called and bm2 is set to equal bm1 at that pixel.
 *  @param bm1       pointer to bitmap 1
 *  @param bm2       pointer to bitmap 2
 *  @param area      pointer to rectangular area
 *  @param diff_func function to call on different pixels
 *  @internal 
 */ 
void 
IPw_BitmapPixDiff(Pw_Bitmap*    bm1, 
                  Pw_Bitmap*    bm2, 
                  Pw_Rectangle* area, 
                  void (*diff_func)(Pw_uInt x, Pw_uInt y, Pw_uInt mode))
{
    Pw_uInt  i, j, x, y, sx, baddr;
    Pw_uInt  left_bytes, right_bytes, width;
    Pw_Byte  b1, b2;
    Pw_Byte* bits1;
    Pw_Byte* bits2;

    IPW_TRACE_ENTER(BM2);
    IPW_CHECK_PTR4(bm1, bm2, area, diff_func);
    IPW_CHECK_BITMAP(bm1);
    IPW_CHECK_BITMAP(bm2);
    IPW_CHECK_COND((bm1->width == bm2->width) && (bm1->height == bm2->height));
    IPW_TRACE_RECT(BM1, area);

    /*
     *  this is like comparing bytes in bitmaps -
     *  see IPw_BitmapPixDiff for comments. The only
     *  difference is that we compare each bit in
     *  current bm1 and bm2 bytes (actually there is
     *  some simple optimisation to speed up things :-)
     */

    baddr = area->y * bm1->width8;
    bits1 = bm1->bits + baddr;
    bits2 = bm2->bits + baddr;

    left_bytes  = area->x >> 3;
    right_bytes = bm1->width8 - ((area->x + area->w + 0x07) >> 3);
    width       = bm1->width8 - left_bytes - right_bytes;

    sx = x = left_bytes << 3;
    y  = area->y;

    IPW_TRACE5(BM1, d, left_bytes, width, right_bytes, x, y);

    for (i = 0; i < area->h; i++)
    {
        bits1 += left_bytes;
        bits2 += left_bytes;
        for (j = 0; j < width; j++)
        {
            if (*bits1 != *bits2)
            {
                if ((*bits1 & 0xF0) != (*bits2 & 0xF0))
                {
                    b1 = *bits1 & 0x80;
                    b2 = *bits2 & 0x80;
            
                    if (b1 != b2)
                        diff_func(x, y, (b1 > 0 ? 1 : 0));
                    x++;

                    b1 = *bits1 & 0x40;
                    b2 = *bits2 & 0x40;

                    if (b1 != b2)
                        diff_func(x, y, (b1 > 0 ? 1 : 0));
                    x++;

                    b1 = *bits1 & 0x20;
                    b2 = *bits2 & 0x20;

                    if (b1 != b2)
                        diff_func(x, y, (b1 > 0 ? 1 : 0));
                    x++;                

                    b1 = *bits1 & 0x10;
                    b2 = *bits2 & 0x10;

                    if (b1 != b2)
                        diff_func(x, y, (b1 > 0 ? 1 : 0));
                    x++;
                }
                else
                {
                    x += 4;
                }
                if ((*bits1 & 0x0F) != (*bits2 & 0x0F))
                {
                    b1 = *bits1 & 0x08;
                    b2 = *bits2 & 0x08;
                
                    if (b1 != b2)
                        diff_func(x, y, (b1 > 0 ? 1 : 0));
                    x++;
    
                    b1 = *bits1 & 0x04;
                    b2 = *bits2 & 0x04;
    
                    if (b1 != b2)
                        diff_func(x, y, (b1 > 0 ? 1 : 0));
                    x++;
    
                    b1 = *bits1 & 0x02;
                    b2 = *bits2 & 0x02;
    
                    if (b1 != b2)
                        diff_func(x, y, (b1 > 0 ? 1 : 0));
                    x++;
    
                    b1 = *bits1 & 0x01;
                    b2 = *bits2 & 0x01;

                    if (b1 != b2)
                        diff_func(x, y, (b1 > 0 ? 1 : 0));
                    x++;
                }
                else
                {
                    x += 4;
                }
                *bits2 = *bits1;
            }
            else
            {
                x += 8;
            }
            bits1++;
            bits2++;
        }
        x = sx; 
        y++;
        bits1 += right_bytes;
        bits2 += right_bytes;
    }
    IPW_TRACE_EXIT(BM2);
}

/**
 *  Initializes the bitmap low level drawing routines. 
 *  This function must be called before any other
 *  bitmap functions are used.
 *  @internal
 */
void
IPw_BitmapInit(void)
{
    IPW_TRACE_ENTER(BM2);

    _bitmap_lld.draw_point    = _BitmapDrawPoint;
    _bitmap_lld.draw_points   = _BitmapDrawPoints;
    _bitmap_lld.draw_hor_line = _BitmapDrawHorLine;
    _bitmap_lld.draw_ver_line = _BitmapDrawVerLine;
    _bitmap_lld.fill_rect     = _BitmapFillRect;

    IPW_TRACE_EXIT(BM2);
}

#if IPW_CHECK_EN(BITMAP_STRUCT)
Pw_Bool
IPw_BitmapCheck(Pw_Bitmap* bitmap)
{
    if (PW_NULL == bitmap->bits)
    {
        IPW_ASSERT(FALSE, ("bitmap (%d) bits == NULL", (Pw_Addr)bitmap));
        return(FALSE);
    }
    if (bitmap->width <= 0)
    {
        IPW_ASSERT(FALSE, ("bitmap (%d) width = %d!", 
                    (Pw_Addr)bitmap, bitmap->width));
        return(FALSE);
    }
    if (bitmap->width8 <= 0)
    {
        IPW_ASSERT(FALSE, ("bitmap (%d) height = %d!",
                    (Pw_Addr)bitmap, bitmap->height));
        return(FALSE);
    }
    if (bitmap->width8 != ((bitmap->width + 0x07) >> 3))
    {
        IPW_ASSERT(FALSE, ("bitmap (%d) width8 = %d width = %d!",
                    (Pw_Addr)bitmap, bitmap->width8, bitmap->width));
        return(FALSE);
    }
    if (bitmap->height <= 0)
    {
        IPW_ASSERT(FALSE, ("bitmap (%d) height = %d!", 
                    (Pw_Addr)bitmap, bitmap->height));
        return(FALSE);
    }
    return(TRUE);
}
#endif /* IPW_CHECK_EN(BITMAP_STRUCT) */

/* -------------------------------------------------------------------------- */

/**
 *  Creates a new bitmap.
 *  @param width  bitmap width
 *  @param height bitmap height
 *  @param bitmap pointer to bitmap to create
 *  @param bits   pointer to bitmap bits array 
 *  @return       created bitmap (same as @p bitmap)
 *  @see Pw_BitmapDestroy
 */ 
Pw_Bitmap* 
Pw_BitmapCreate(Pw_Coord   width, 
                Pw_Coord   height, 
                Pw_Byte*   bits,
                Pw_Bitmap* bitmap)
{
    Pw_uInt i, width8, byte_size;

    IPW_TRACE_ENTER(BM3);
    IPW_CHECK_PTR2(bitmap, bits);

    width8    = (width + 0x07) >> 3;
    byte_size = width8 * height;

    bitmap->width  = width;
    bitmap->height = height;
    bitmap->width8 = width8; 
    bitmap->bits   = bits;

    IPW_TRACE3(BM2, d, width, height, width8);
    
    for (i = 0; i < byte_size; i++)
        bitmap->bits[i] = 0x00;

    IPW_CHECK_BITMAP(bitmap);
    IPW_TRACE_EXIT_V(BM3, p, bitmap);
    return(bitmap);
}

/**
 *  Destroys the bitmap.
 *  @param bitmap pointer to bitmap to destroy 
 *  @see Pw_BitmapCreate
 */ 
void 
Pw_BitmapDestroy(Pw_Bitmap* bitmap)
{
    IPW_TRACE_ENTER(BM3);
    IPW_CHECK_PTR(bitmap);
    IPW_CHECK_BITMAP(bitmap);
    IPW_TRACE1(BM2, p, bitmap);
    IPW_TRACE3(BM2, d, bitmap->width, bitmap->height, bitmap->width8);
    IPW_CHECK_COND(bitmap->width8 == ((bitmap->width + 0x07) >> 3));
    IPW_TRACE_EXIT(BM3);
}

/**
 *  Creates a new graphic context for bitmap.
 *  @param bitmap pointer to bitmap
 *  @param gc     pointer to graphics context to create
 *  @return bitmap's graphic context (same as @p gc)
 *  @see Pw_BitmapDestroyGC
 */ 
Pw_GC* 
Pw_BitmapCreateGC(Pw_Bitmap* bitmap, Pw_GC* gc)
{
    IPW_TRACE_ENTER(BM3);
    IPW_CHECK_PTR2(bitmap, gc);
    IPW_CHECK_BITMAP(bitmap);
    IPW_TRACE2(BM2, d, bitmap->width, bitmap->height);
    
    gc->d.type        = Pw_DrawableBitmapType;
    gc->d.comp.bitmap = bitmap;
    gc->d.lld         = &_bitmap_lld;

    IPW_CHECK_COND(gc->d.lld->draw_point    != PW_NULL);
    IPW_CHECK_COND(gc->d.lld->draw_points   != PW_NULL);
    IPW_CHECK_COND(gc->d.lld->draw_hor_line != PW_NULL);
    IPW_CHECK_COND(gc->d.lld->draw_ver_line != PW_NULL);
    IPW_CHECK_COND(gc->d.lld->fill_rect     != PW_NULL);

    gc->xoff = 0;
    gc->yoff = 0;
    gc->clip_reg.regs_num = 1;
    gc->clip_reg.bounds.x = 0;
    gc->clip_reg.bounds.y = 0;
    gc->clip_reg.bounds.w = bitmap->width;
    gc->clip_reg.bounds.h = bitmap->height;
    gc->clip_reg.regs = IPw_RegionNew();

    IPW_FAIL(gc->clip_reg.regs != PW_NULL, ("Out of free regions!"));

    IPW_RECT_COPY(&gc->clip_reg.regs->area, &gc->clip_reg.bounds);

    gc->color = pw_black_pixel;
    gc->font  = PW_NULL;

    IPW_CHECK_GC(gc);
    IPW_TRACE_EXIT_V(BM3, p, gc);
    return(gc);
}

/**
 *  Destroys the bitmap graphic context.
 *  @param gc graphic context to destroy 
 *  @see Pw_BitmapCreateGC
 */ 
void 
Pw_BitmapDestroyGC(Pw_GC* gc)
{
    IPW_TRACE_ENTER(BM3);
    IPW_CHECK_PTR(gc);
    IPW_CHECK_GC(gc);
    IPW_CHECK_COND(gc->d.type == Pw_DrawableBitmapType);
    IPw_RegionFree(gc->clip_reg.regs); 
    IPW_TRACE_EXIT(BM3);
}

/*---------------------------------------------------------------------------
// end of pw_bitmap.c 
//--------------------------------------------------------------------------- */
