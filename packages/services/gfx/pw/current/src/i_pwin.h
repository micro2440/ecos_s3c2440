/*===========================================================================
//
//        i_pwin.h
//
//        PW graphic library INTERNAL defs 
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

#ifndef I_PWIN_H
#define I_PWIN_H

/* -------------------------------------------------------------------------- */

#include <pwin.h>
#include <pw_diag.h>

/* -------------------------------------------------------------------------- */

/** Maximum number of regions. */
#define IPW_REGION_MAX_NUM 128

/* -------------------------------------------------------------------------- */

#define IPW_MACRO_START     do {
#define IPW_MACRO_END       } while (0)
#define IPW_EMPTY_STATEMENT IPW_MACRO_START IPW_MACRO_END

/* -------------------------------------------------------------------------- */

/**
 *  Makes rectangle @p r an empty rectangle.
 *  @param _r_ rectangle @p r pointer
 *  @internal
 */ 
#define IPW_RECT_EMPTY(_r_)  \
    ((_r_)->x = 0,           \
     (_r_)->y = 0,           \
     (_r_)->w = 0,           \
     (_r_)->h = 0)        

/**
 *  Copies rectangle @p b to rectangle @p a.
 *  @param _a_ rectangle @p a pointer
 *  @param _b_ rectangle @p b pointer
 *  @internal
 */ 
#define IPW_RECT_COPY(_a_, _b_)  \
    ((_a_)->x = (_b_)->x,        \
     (_a_)->y = (_b_)->y,        \
     (_a_)->w = (_b_)->w,        \
     (_a_)->h = (_b_)->h)

/**
 *  Copies segment @p b to segment @p a.
 *  @param _a_ segment @p a pointer
 *  @param _b_ segment @p b pointer
 *  @internal
 */ 
#define IPW_SEGMENT_COPY(_a_, _b_)  \
    ((_a_)->x1 = (_b_)->x1,         \
     (_a_)->y1 = (_b_)->y1,         \
     (_a_)->x2 = (_b_)->x2,         \
     (_a_)->y2 = (_b_)->y2)

/**
 *  Checks if @p r is a valid rectangle.
 *  An rectangle is valid if its width and height are greater then zero.
 *  @param _r_ rectangle @p r pointer
 *  @return true is rectangle @p r is a valid rectangle
 *  @internal
 */ 
#define IPW_RECT_IS_VALID(_r_)       \
    ((_r_)->w > 0 && (_r_)->h > 0)

/**
 *  Checks if rectangle @p a is equal to rectangle @p b.
 *  @param _a_ rectangle @p a pointer
 *  @param _b_ rectangle @p b pointer
 *  @return true if rectangle @p a equals rectangle @p b
 *  @internal
 */ 
#define IPW_RECT_IS_EQUAL(_a_, _b_)  \
    (((_a_)->x == (_b_)->x) &&       \
     ((_a_)->y == (_b_)->y) &&       \
     ((_a_)->w == (_b_)->w) &&       \
     ((_a_)->h == (_b_)->h)) 

/**
 *  Checks if segment @p a is equal to segment @p b.
 *  @param _a_ segment @p a pointer
 *  @param _b_ segment @p b pointer
 *  @return true if segment @p a equals segment @p b
 *  @internal
 */ 
#define IPW_SEGMENT_IS_EQUAL(_a_, _b_)  \
    (((_a_)->x1 == (_b_)->x1) &&        \
     ((_a_)->y1 == (_b_)->y1) &&        \
     ((_a_)->x2 == (_b_)->x2) &&        \
     ((_a_)->y2 == (_b_)->y2)) 

/**
 *  Checks if point @p x, @p y is inside rectangle @p r.
 *  @param _x_ point's @p x coord
 *  @param _y_ point's @p y coord
 *  @param _r_ rectangle @p r pointer
 *  @return true if point (@p x, @p y) is inside rectangle @p r
 *  @internal
 */    
#define IPW_RECT_IS_POINT_IN(_x_, _y_, _r_)  \
    (!(((_x_) < (_r_)->x)               ||   \
       ((_y_) < (_r_)->y)               ||   \
       ((_x_) >= ((_r_)->x + (_r_)->w)) ||   \
       ((_y_) >= ((_r_)->y + (_r_)->h))))

/**
 *  Checks if rectangle @p a is over (intersects) rectangle @p b.
 *  @param _a_ rectangle @p a pointer
 *  @param _b_ rectangle @p b pointer
 *  @return true if rectangle @p a is over (intersects) rectangle @p b
 *  @internal
 */ 
#define IPW_RECT_IS_OVER(_a_, _b_)                        \
    (!((((_a_)->x + (_a_)->w) <= (_b_)->x)             || \
       (((_a_)->y + (_a_)->h) <= (_b_)->y)             || \
       ((_a_)->x            >= ((_b_)->x + (_b_)->w))  || \
       ((_a_)->y            >= ((_b_)->y + (_b_)->h))))

/**
 *  Checks is rectangle @p a is inside rectangle @p b.
 *  @param _a_ rectangle @p a pointer
 *  @param _b_ rectangle @p b pointer
 *  @return true if rectangle @p a is inside rectangle @p b
 *  @internal
 */ 
#define IPW_RECT_IS_IN(_a_, _b_)                        \
    (!(((_a_)->x            > (_b_)->x)              || \
       ((_a_)->y            > (_b_)->y)              || \
       (((_a_)->x + (a)->w) < ((_b_)->x + (_b_)->w)) || \
       (((_a_)->y + (a)->h) < ((_b_)->y + (_b_)->h))))

/* -------------------------------------------------------------------------- */

void       IPw_RegionInit(void);

Pw_Region* IPw_RegionNew(void);

Pw_Region* IPw_RegionNewList(Pw_Int* len, Pw_Region* link);

Pw_Region* IPw_RegionNewListFromArray(Pw_Rectangle* rarray,
                                      Pw_Int*       len,
                                      Pw_Region*    link);

Pw_Region* IPw_RegionNewListFromList(Pw_RegionList* rlist,
                                     Pw_Int*        len,
                                     Pw_Region*     link);

void       IPw_RegionFree(Pw_Region* reg);

void       IPw_RegionFreeList(Pw_Region* reg);

Pw_Int16   IPw_RegionGetFreeCnt(void);

/* -------------------------------------------------------------------------- */

Pw_Int  IPw_RectOutCode(Pw_Rectangle* r, Pw_Coord x, Pw_Coord y);

Pw_Bool IPw_RectIntersect(Pw_Rectangle* a, Pw_Rectangle* b);

void    IPw_RectUnionize(Pw_Rectangle* a, Pw_Rectangle* b);

Pw_Int  IPw_RectExclude(Pw_Rectangle* z,
                        Pw_Rectangle* a,
                        Pw_Rectangle* b,
                        Pw_Rectangle* c,
                        Pw_Rectangle* d);

Pw_Bool IPw_RectExcludeFromRegionList(Pw_Rectangle*  z,
                                      Pw_RegionList* rlist);

/* -------------------------------------------------------------------------- */

void IPw_BitmapInit(void);

void IPw_BitmapDrawPoint(Pw_Bitmap* bitmap,
                         Pw_Coord   x,
                         Pw_Coord   y,
                         Pw_uInt    mode);

void IPw_BitmapDrawPoints(Pw_Bitmap* bitmap,
                          Pw_Coord   x,
                          Pw_Coord   y,
                          Pw_Byte    bits,
                          Pw_uInt    mode);

void IPw_BitmapDrawHorLine(Pw_Bitmap* bitmap,
                           Pw_Coord   x1,
                           Pw_Coord   y1,
                           Pw_Coord   x2,
                           Pw_uInt    mode);

void IPw_BitmapDrawVerLine(Pw_Bitmap* bitmap,
                           Pw_Coord   x1,
                           Pw_Coord   y1,
                           Pw_Coord   y2,
                           Pw_uInt    mode);

void IPw_BitmapFillRect(Pw_Bitmap* bitmap,
                        Pw_Coord   x,
                        Pw_Coord   y,
                        Pw_Coord   w,
                        Pw_Coord   h,
                        Pw_uInt    mode);

void IPw_BitmapDiff(Pw_Bitmap*    bm1,
                    Pw_Bitmap*    bm2,
                    Pw_Rectangle* area,
                    void (*diff_func)(Pw_uInt baddr, Pw_Byte dbyte));

void IPw_BitmapPixDiff(Pw_Bitmap*    bm1,
                       Pw_Bitmap*    bm2,
                       Pw_Rectangle* area,
                       void (*diff_func)(Pw_uInt x, Pw_uInt y, Pw_uInt mode));

/* -------------------------------------------------------------------------- */

Pw_Bool IPw_EventSendKeyPress(Pw_Window* win, Pw_uInt keycode);

Pw_Bool IPw_EventSendKeyRelease(Pw_Window* win, Pw_uInt keycode);

Pw_Bool IPw_EventSendXInput(Pw_Window* win, Pw_uInt num, Pw_Int val);

Pw_Bool IPw_EventSendFocusIn(Pw_Window* win);

Pw_Bool IPw_EventSendFocusOut(Pw_Window* win);

Pw_Bool IPw_EventSendMapped(Pw_Window* win);

Pw_Bool IPw_EventSendUnmapped(Pw_Window* win);

Pw_Bool IPw_EventSendReshaped(Pw_Window* win);

Pw_Bool IPw_EventSendDestroy(Pw_Window* win);

Pw_Bool IPw_EventSendRepaint(Pw_Window* win, Pw_GC* gc);

Pw_Bool IPw_EventKeyPressed(Pw_Display* dpy, Pw_uInt keycode);

Pw_Bool IPw_EventKeyReleased(Pw_Display* dpy, Pw_uInt keycode);

Pw_Bool IPw_EventXInputChanged(Pw_Display* dpy, Pw_uInt num, Pw_Int val);

/* -------------------------------------------------------------------------- */

void IPw_TimeoutProcess(Pw_Display* dpy, Pw_uLong time_elp);

void IPw_TimeoutRemoveAll(Pw_Display* dpy);

/* -------------------------------------------------------------------------- */

Pw_Window* IPw_RootWindowCreate(Pw_Display* dpy, 
                                Pw_Coord    width, 
                                Pw_Coord    height);

void       IPw_WindowClearEventHooks(Pw_Window* win);

/* -------------------------------------------------------------------------- */

void    IPw_InitInternals(void);

Pw_Bool IPw_InitComponents(Pw_Display* dpy);

/* -------------------------------------------------------------------------- */

Pw_Display* IPw_DisplayOpen(Pw_Coord    width, 
                            Pw_Coord    height, 
                            Pw_DD*      dd, 
                            Pw_Display* dpy);

void        IPw_DisplayClose(Pw_Display* dpy);

void        IPw_DisplayRefresh(Pw_Display* dpy);

/* -------------------------------------------------------------------------- */

#endif /* not I_PWIN_H */

/*---------------------------------------------------------------------------
// end of i_pwin.h 
//--------------------------------------------------------------------------- */
