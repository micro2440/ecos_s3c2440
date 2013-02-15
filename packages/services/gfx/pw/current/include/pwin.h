/*===========================================================================
//
//        pwin.h
//
//        PW graphic library defs 
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

#ifndef PWIN_H
#define PWIN_H

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* -------------------------------------------------------------------------- */

/** False definition. */
#ifndef FALSE
  #define FALSE 0
#endif

/** True definition. */
#ifndef TRUE
  #define TRUE (!FALSE)
#endif

/** NULL definition. */
#define PW_NULL 0

/* -------------------------------------------------------------------------- */

/** Maximum number of extra input events. */
#define PW_XINPUTS_MAX_NUM 22

/* -------------------------------------------------------------------------- */

#define PW_UP_KEY_CODE     1 
#define PW_DOWN_KEY_CODE   2 
#define PW_LEFT_KEY_CODE   3
#define PW_RIGHT_KEY_CODE  4 
#define PW_NEXT_KEY_CODE   5 
#define PW_PREV_KEY_CODE   6 
#define PW_YES_KEY_CODE    7 
    
/* -------------------------------------------------------------------------- */

/**
 *  Returns the greater of two numbers.
 *  @param _a_ number 1
 *  @param _b_ number 2
 *  @return the greater of @p a and @p b
 */
#define PW_MAX(_a_, _b_) ((_a_) > (_b_) ? (_a_) : (_b_))

/**
 *  Returns the smaller of two numbers.
 *  @param _a_ number 1
 *  @param _b_ number 2
 *  @return the smaller of @p a and @p b
 */
#define PW_MIN(_a_, _b_) ((_a_) < (_b_) ? (_a_) : (_b_))

/**
 *  Returns the absolute value of @p a.
 *  @param _a_ number 
 *  @return the absolute value of @p a 
 */
#define PW_ABS(_a_)      ((_a_) <  0  ? (-(_a_)) : (_a_))

/* -------------------------------------------------------------------------- */

typedef unsigned char      Pw_Byte;    /**< Byte type. */
typedef char               Pw_Char;    /**< Charachter type. */
typedef unsigned char      Pw_uChar;   /**< Unsigned charachter type. */
typedef short              Pw_Short;   /**< Short integer type. */
typedef unsigned short     Pw_uShort;  /**< Unsigned short integer type. */
typedef int                Pw_Int;     /**< Integer type. */
typedef signed char        Pw_Int8;    /**< 8 bit integer type. */
typedef short              Pw_Int16;   /**< 16 bit integer type. */
typedef int                Pw_Int32;   /**< 32 bit integer type. */
typedef unsigned int       Pw_uInt;    /**< Unsigned integer type. */
typedef unsigned char      Pw_uInt8;   /**< 8 bit unsigned integer type. */
typedef unsigned short     Pw_uInt16;  /**< 16 bit unsigned integer type. */
typedef unsigned int       Pw_uInt32;  /**< 32 bit unsigned integer type. */
typedef long               Pw_Long;    /**< Long integer type. */
typedef unsigned long      Pw_uLong;   /**< Unsigned long integer type. */
typedef int                Pw_Bool;    /**< Boolean type. */
typedef void*              Pw_Pointer; /**< Generic pointer type. */
typedef Pw_uInt32          Pw_Addr;    /**< Address type. */
typedef Pw_Int16           Pw_Coord;   /**< Coordinate type. */

/* -------------------------------------------------------------------------- */

typedef struct _Pw_Point        Pw_Point;        /**< Point type. */
typedef struct _Pw_Segment      Pw_Segment;      /**< Line segment type. */
typedef struct _Pw_Rectangle    Pw_Rectangle;    /**< Rectangle type. */
typedef struct _Pw_Region       Pw_Region;       /**< Region type. */
typedef struct _Pw_RegionList   Pw_RegionList;   /**< Region list type. */
typedef struct _Pw_Color        Pw_Color;        /**< Color type. */
typedef struct _Pw_Bitmap       Pw_Bitmap;       /**< Bitmap type. */
typedef struct _Pw_Font         Pw_Font;         /**< Font type. */
typedef struct _Pw_FontCharInfo Pw_FontCharInfo; /**< Font char info type. */
typedef struct _Pw_Drawable     Pw_Drawable;     /**< Drawable type. */
typedef struct _Pw_DD           Pw_DD;           /**< Display driver fn type. */
typedef struct _Pw_LLD          Pw_LLD;          /**< LL drawing fn type. */
typedef struct _Pw_GC           Pw_GC;           /**< Graphic context type. */
typedef struct _Pw_EventData    Pw_EventData;    /**< Event data type. */
typedef struct _Pw_KeyEvent     Pw_KeyEvent;     /**< Key event type. */
typedef struct _Pw_XInputEvent  Pw_XInputEvent;  /**< Extra input event type. */
typedef struct _Pw_FocusEvent   Pw_FocusEvent;   /**< Focus event type. */
typedef struct _Pw_MappingEvent Pw_MappingEvent; /**< Mapping event type. */
typedef struct _Pw_ReshapeEvent Pw_ReshapeEvent; /**< Reshape event type. */
typedef struct _Pw_DestroyEvent Pw_DestroyEvent; /**< Destroy event type. */
typedef union  _Pw_Event        Pw_Event;        /**< Event union type. */
typedef struct _Pw_Timeout      Pw_Timeout;      /**< Timeout type. */
typedef struct _Pw_Window       Pw_Window;       /**< Window type. */
typedef struct _Pw_Display      Pw_Display;      /**< Display type. */

/* -------------------------------------------------------------------------- */

/**
 *  A type name for event callback.
 *  @param Pw_Event event
 */
typedef Pw_Bool (*Pw_EventCb)
(
    Pw_Event*
);

/**
 *  A type name for repaint callback.
 *  @param Pw_GC graphic context
 */
typedef void (*Pw_RepaintCb)
(
    Pw_GC*
);

/**
 *  A type name for timeout callback.
 *  @param Pw_Display display
 *  @param Pw_Pointer client data
 */
typedef Pw_Bool (*Pw_TimeoutCb)
(
    Pw_Display*,
    Pw_Pointer
);

/* -------------------------------------------------------------------------- */

/**
 *  Types of drawables.
 */
typedef enum _Pw_DrawableType
{
    Pw_DrawableWindowType, /**< Type of drawable wich applies to windows. */
    Pw_DrawableBitmapType  /**< Type of drawable wich applies to bitmaps. */
} Pw_DrawableType;

/**
 *  Types of events.
 */
typedef enum _Pw_EventType
{
    Pw_KeyPressEventType,   /**< Type of key pressed event. */
    Pw_KeyReleaseEventType, /**< Type of key released event. */
    Pw_XInputEventType,     /**< Type of xinput event. */
    Pw_FocusInEventType,    /**< Type of focus in (gained) event. */
    Pw_FocusOutEventType,   /**< Type of focus out (lost) event. */
    Pw_MapEventType,        /**< Type of window mapped event. */
    Pw_UnmapEventType,      /**< Type of window unmapped event. */
    Pw_ReshapeEventType,    /**< Type of window reshaped event. */
    Pw_DestroyEventType     /**< Type of window destroyed event. */
} Pw_EventType;

/* -------------------------------------------------------------------------- */

/**
 *  Point.
 *  A 2D point.
 *  @see Pw_Segment
 *  @see Pw_Rectangle
 */
struct _Pw_Point
{
    Pw_Coord x; /**< Point's x coord. */
    Pw_Coord y; /**< Point's y coord. */
};

/**
 *  Line segment.
 *  A 2D line segment.
 *  @see Pw_Point
 *  @see Pw_Rectangle
 */
struct _Pw_Segment
{
    Pw_Coord x1; /**< Segment start point x coord. */
    Pw_Coord y1; /**< Segment start point y coord. */
    Pw_Coord x2; /**< Segment end point x coord. */
    Pw_Coord y2; /**< Segment end point y coord. */
};

/**
 *  Rectangle.
 *  A 2D rectangle.
 *  @see Pw_Point
 *  @see Pw_Segment
 */
struct _Pw_Rectangle
{
    Pw_Coord x; /**< Rectangle top left corner x coord. */
    Pw_Coord y; /**< Rectangle top left corner y coord. */
    Pw_Coord w; /**< Rectangle width. */
    Pw_Coord h; /**< Rectangle height. */
};

/**
 *  Rectangular region.
 *  Rectangular region element used in forming a list of regions.
 *  @see Pw_Rectangle
 *  @see Pw_RegionList
 */
struct _Pw_Region
{
    Pw_Rectangle area; /**< Rectangular area. */
    Pw_Region*   next; /**< Pointer to next region in list. */
};

/**
 *  Region List.
 *  Region list is a list of regions including regions count and bounds.
 *  @see Pw_Region
 */
struct _Pw_RegionList
{
    Pw_Region*   regs;     /**< List of rectangular regions. */
    Pw_Int       regs_num; /**< Number of regions in list. */ 
    Pw_Rectangle bounds;   /**< Bounds of regions in list. */
};

/**
 *  Color.
 *  A color defined by its red, green and blue components.
 */
struct _Pw_Color
{
    Pw_Byte red;   /**< Red component. */
    Pw_Byte green; /**< Green component. */
    Pw_Byte blue;  /**< Blue component. */
};

/**
 *  Bitmap.
 *  A bitmap is a B/W picture, the pixels are represented by bits.
 *  NOTE: each line ends at byte border.
 */
struct _Pw_Bitmap
{
    Pw_uInt  width;  /**< Bitmap width. */
    Pw_uInt  height; /**< Bitmap height. */
    Pw_uInt  width8; /**< Bitmap width in bytes. */
    Pw_Byte* bits;   /**< Array of bitmap bits. */
};

/**
 *  Font.
 *  A font carries information about its vertical position
 *  according to the 0 line, information about each charachter
 *  position according to the previous charachter and the 
 *  bitmaps of all charachters.
 *  @see Pw_FontCharInfo
 */
struct _Pw_Font
{
    Pw_uInt           ascent;    /**< Font charachter ascent. */
    Pw_uInt           descent;   /**< Font charachter descent. */
    Pw_uInt           chars_num; /**< Number of charachters in font. */
    Pw_FontCharInfo*  info;      /**< Array of charachters info. */
    Pw_Byte*          bits;      /**< Array of charachters bitmaps bits. */
};

/**
 *  Font charachter info.
 *  A Font charachter info describes the charachters bounds,
 *  the location of this charachter according to the location of
 *  previous charachter in the string and the charachter bitmap
 *  location in its font bits array.
 *  @see Pw_Font
 */
struct _Pw_FontCharInfo
{
    Pw_Int8   bbx, bby;    /**< Charachter x and y offset from origin. */
    Pw_Int8   bbw, bbh;    /**< Charachter width and height. */
    Pw_Int8   dwx;         /**< X offset of next charachter origin. */
    Pw_uInt32 bits_offset; /**< Offset of bitmap bits in font bits array. */
};

/**
 *  Drawable.
 *  A drawable describes a component that supports drawing.
 *  @see Pw_DrawableType
 *  @see Pw_LLD
 *  @see Pw_GC
 */
struct _Pw_Drawable
{
    Pw_DrawableType type;  /**< Type of this drawable. */
    union
    {
        Pw_Window* window; /**< Window component. */
        Pw_Bitmap* bitmap; /**< Bitmap component. */
    } comp;                /**< Component to which this drawable reffers. */
    Pw_LLD* lld;           /**< Low level drawing funs of this drawable. */
};

/**
 *  Display driver functions.
 *  Display specific functions. 
 *  @see Pw_LLD
 */
struct _Pw_DD
{
    /** Low level drawing funs used to draw on this display. */
    Pw_LLD* lld;

    /** Function that refreshes the display. */
    void (*display_refresh)(Pw_Display* dpy, Pw_Rectangle* area);

    /** Function that gets the current value of given xinput. */
    Pw_Int (*get_xinput_value)(Pw_Display* dpy, Pw_uInt num);

    /** Function that initializes display components. */
    Pw_Bool (*init_components)(Pw_Display* dpy);
};

/**
 *  Low level drawing functions.
 *  Low level functions that draw directly into specified drawable.
 *  @see Pw_Drawable
 *  @see Pw_GC
 *  @see Pw_DD
 */
struct _Pw_LLD
{
    /** Function that draws a point. */
    void (*draw_point)(Pw_GC*   gc, 
                       Pw_Coord x, 
                       Pw_Coord y);

    /** Function that draws up to 8 points (in row). */
    void (*draw_points)(Pw_GC*   gc, 
                        Pw_Coord x, 
                        Pw_Coord y, 
                        Pw_Byte  bits);

    /** Function that draws a horizontal line. */
    void (*draw_hor_line)(Pw_GC*   gc, 
                          Pw_Coord x1, 
                          Pw_Coord y1, 
                          Pw_Coord x2);

    /** Function that draws a vertical line. */
    void (*draw_ver_line)(Pw_GC*   gc, 
                          Pw_Coord x1, 
                          Pw_Coord y1, 
                          Pw_Coord y2);

    /** Function that fills a rectangular area. */
    void (*fill_rect)(Pw_GC*   gc, 
                      Pw_Coord x, 
                      Pw_Coord y, 
                      Pw_Coord w, 
                      Pw_Coord h);
};

/**
 *  Graphic context.
 *  A Graphic context carries all the information we need
 *  in order to draw something. Before we can draw anything 
 *  we need to obtain a graphic contex. There are several 
 *  types of graphic contextes depending on where we want 
 *  to draw (display, bitmaps, ...).
 *  @see Pw_Drawable
 */
struct _Pw_GC
{
    /** Drawable to wich this graphic context belongs. */
    Pw_Drawable    d;     
    Pw_Int         xoff;     /**< X offset (origin). */
    Pw_Int         yoff;     /**< Y offset (origin). */
    Pw_RegionList  clip_reg; /**< Clip region. */
    Pw_Color*      color;    /**< Current color. */
    Pw_Font*       font;     /**< Current font. */
};

/**
 *  General event data.
 */
struct _Pw_EventData
{
    Pw_EventType type; /**< Event type. */
    Pw_Window*   win;  /**< Window from wich this event originated. */
};

/**
 *  Key event.
 *  @see Pw_EventData
 */
struct _Pw_KeyEvent
{
    /* The location and type of 'data' var must not change! */
    Pw_EventData data;    /**< General event data. */
    Pw_uInt      keycode; /**< Key code of the key pressed/released. */
};

/**
 *  XInput event.
 *  @see Pw_EventData
 */
struct _Pw_XInputEvent
{
    /* The location and type of 'data' var must not change! */
    Pw_EventData data; /**< General event data. */
    Pw_uInt      num;  /**< Xinput number. */
    Pw_Int       val;  /**< Xinput value. */
};

/**
 *  Focus event.
 *  @see Pw_EventData
 */
struct _Pw_FocusEvent
{
    /* The location and type of 'data' var must not change! */
    Pw_EventData data; /**< General event data. */
};

/**
 *  Mapping event.
 *  @see Pw_EventData
 */
struct _Pw_MappingEvent
{
    /* The location and type of 'data' var must not change! */
    Pw_EventData data; /**< General event data. */
};

/**
 *  Reshape event.
 *  @see Pw_EventData
 */
struct _Pw_ReshapeEvent
{
    /* The location and type of 'data' var must not change! */
    Pw_EventData data; /**< General event data. */
};

/**
 *  Destroy event.
 *  @see Pw_EventData
 */
struct _Pw_DestroyEvent
{
    /* The location and type of 'data' var must not change! */
    Pw_EventData data; /**< General event data. */
};

/**
 *  Event union.
 *  @see Pw_EventData
 *  @see Pw_KeyEvent
 *  @see Pw_XInputEvent
 *  @see Pw_FocusEvent
 *  @see Pw_MappingEvent
 *  @see Pw_ReshapeEvent
 *  @see Pw_DestroyEvent
 */
union _Pw_Event
{
    Pw_EventData    data;    /**< General event data. */
    Pw_KeyEvent     key;     /**< Key event. */
    Pw_XInputEvent  xinput;  /**< Xinput event. */
    Pw_FocusEvent   focus;   /**< Focus event. */
    Pw_MappingEvent mapping; /**< Mapping event. */
    Pw_ReshapeEvent reshape; /**< Reshape event. */
    Pw_DestroyEvent destroy; /**< Destroy event. */
};

/**
 *  Timeout.
 *  @see Pw_Display
 */
struct _Pw_Timeout
{
    Pw_TimeoutCb cb;        /**< Callback function. */
    Pw_Pointer   data;      /**< Client data. */
    Pw_uLong     time;      /**< Time interval. */
    Pw_uLong     time_left; /**< Time left to callback trigger. */
    Pw_Timeout*  next;      /**< Next timeout in list. */
};

/**
 *  Window.
 *  @see Pw_Display
 */
struct _Pw_Window
{
    Pw_Display*   dpy;            /**< Display of this window. */
    Pw_Rectangle  area;           /**< Window area. */
    Pw_Window*    parent;         /**< Parent window. */
    Pw_Window*    children;       /**< First children in child windows list. */
    Pw_uInt       children_num;   /**< Number of children windows. */
    Pw_Window*    prev;           /**< Prev window in list. */
    Pw_Window*    next;           /**< Next window in list. */
    Pw_RegionList clip_reg;       /**< Visible region of window. */
    Pw_Bool       mapped;         /**< True if this window is mapped. When
                                       a window is mapped it is visible on
                                       display. When it is not mapped is
                                       invisible. */
    Pw_Bool       focus_accept;   /**< True if this window accept focus.
                                       If a window accepts focus and it gets
                                       focus, it will receive key events. */
    Pw_EventCb    event_cb;       /**< Window event callback. */
    Pw_RepaintCb  repaint_cb;     /**< Repaint callback. */
    Pw_Int        cdata_id;       /**< Id of client data. */
    Pw_Pointer    cdata;          /**< Clients using this window can store
                                       pointer to some misc data in here. */
};

/**
 *  Display.
 */
struct _Pw_Display
{
    Pw_Coord     width;     /**< Width of display. */
    Pw_Coord     height;    /**< Height of display. */
    Pw_Window    root_win;  /**< Root window of display. */
    Pw_Window*   focus_win; /**< Window that has focus. A window in focus
                                 receives key events. */
    Pw_Timeout*  timeouts;  /**< Display timeouts. */

    /** Windows that will receive specific xinput events. */
    Pw_Window*   xinput_win[PW_XINPUTS_MAX_NUM]; 
                                                
    Pw_DD*       dd;        /**< Display driver functions.*/
    Pw_Bool      close;     /**< True if this display is going to close now. */
};

/* -------------------------------------------------------------------------- */

extern Pw_Color* pw_white_pixel; /**< White pixel definition. */

extern Pw_Color* pw_black_pixel; /**< Black pixel definition. */

/* -------------------------------------------------------------------------- */

/**
 *  Define macro start.
 */
#define PW_MACRO_START do {

/**
 *  Define macro end.
 */
#define PW_MACRO_END   } while (0)

/* -------------------------------------------------------------------------- */

/**
 *  Gets the width of display @p dpy.
 *  @param _dpy_ display @p dpy pointer 
 *  @return the width of display
 *  @see Pw_Display
 */
#define Pw_DisplayGetWidth(_dpy_) \
    ((_dpy_)->width)

/**
 *  Gets the height of display @p dpy.
 *  @param _dpy_ display @p dpy pointer 
 *  @return the height of display
 *  @see Pw_Display
 */
#define Pw_DisplayGetHeight(_dpy_) \
    ((_dpy_)->height)

/**
 *  Gets the root window of display @p dpy.
 *  @param _dpy_ display @p dpy pointer
 *  @return pointer to root window of display
 *  @see Pw_Display
 *  @see Pw_Window
 */
#define Pw_DisplayGetRootWindow(_dpy_) \
    (&(_dpy_)->root_win)

/**
 *  Gets the window in focus of display @p dpy.
 *  @param _dpy_ display @p dpy pointer
 *  @return pointer to window in focus of display
 *  @see Pw_Display
 *  @see Pw_Window
 */
#define Pw_DisplayGetFocusWindow(_dpy_) \
    ((_dpy_)->focus_win)

/**
 *  Closes the display @p dpy.
 *  @param _dpy_ display @p dpy pointer 
 *  @see Pw_Display
 */
#define Pw_DisplayClose(_dpy_) \
    ((_dpy_)->close = TRUE)

/* -------------------------------------------------------------------------- */

/**
 *  Gets the display of window @p win.
 *  @param _win_ window @p win pointer
 *  @return pointer to display of window
 *  @see Pw_Display
 *  @see Pw_Window
 */
#define Pw_WindowGetDisplay(_win_) \
    ((_win_)->dpy)

/**
 *  Gets the x coordinate of window @p win.
 *  @param _win_ window @p win pointer
 *  @return the x coordinate window
 *  @see Pw_Window
 */
#define Pw_WindowGetX(_win_) \
    ((_win_)->area.x)

/**
 *  Sets the x coordinate of window @p win.
 *  @param _win_ window @p win pointer
 *  @param _x_   new x coordinate of window 
 *  @see Pw_Window
 */
#define Pw_WindowSetX(_win_, _x_)         \
    Pw_WindowSetShape((_win_),            \
                      (_x_),              \
                      (_win_)->area.y,    \
                      (_win_)->area.w,    \
                      (_win_)->area.h)

/**
 *  Gets the y coordinate of window @p win.
 *  @param _win_ window @p win pointer
 *  @return the y coordinate window
 *  @see Pw_Window
 */
#define Pw_WindowGetY(_win_) \
    ((_win_)->area.y)

/**
 *  Sets the y coordinate of window @p win.
 *  @param _win_ window @p win pointer
 *  @param _y_   new y coordinate of window 
 *  @see Pw_Window
 */
#define Pw_WindowSetY(_win_, _y_)         \
    Pw_WindowSetShape((_win_),            \
                      (_win_)->area.x,    \
                      (_y_),              \
                      (_win_)->area.w,    \
                      (_win_)->area.h)

/**
 *  Gets the width of window @p win.
 *  @param _win_ window @p win pointer
 *  @return the width of window
 *  @see Pw_Window
 */
#define Pw_WindowGetWidth(_win_) \
    ((_win_)->area.w)

/**
 *  Sets the width of window @p win.
 *  @param _win_   window @p win pointer
 *  @param _width_ new width of window 
 *  @see Pw_Window
 */
#define Pw_WindowSetWidth(_win_, _width_) \
    Pw_WindowSetShape((_win_),            \
                      (_win_)->area.x,    \
                      (_win_)->area.y,    \
                      (_width_),          \
                      (_win_)->area.h)

/**
 *  Gets the height of window @p win.
 *  @param _win_ window @p win pointer
 *  @return the height of window
 *  @see Pw_Window
 */
#define Pw_WindowGetHeight(_win_) \
    ((_win_)->area.h)

/**
 *  Sets the height of window @p win.
 *  @param _win_ window @p win pointer
 *  @param _x_   new height of window 
 *  @see Pw_Window
 */
#define Pw_WindowSetHeight(_win_, _height_) \
    Pw_WindowSetShape((_win_),              \
                      (_win_)->area.x,      \
                      (_win_)->area.y,      \
                      (_win_)->area.w,      \
                      (_height_))          

/**
 *  Gets the position of window @p win.
 *  @param _win_ window @p win pointer
 *  @param _xp_  pointer to x position of window (returned)
 *  @param _yp_  pointer to y position of window (returned)
 *  @see Pw_Window
 */
#define Pw_WindowGetPosition(_win_, _xp_, _yp_) \
    PW_MACRO_START                              \
        *(_xp_) = (_win_)->area.x;              \
        *(_yp_) = (_win_)->area.y;              \
    PW_MACRO_END

/**
 *  Sets the position of window @p win.
 *  @param _win_ window @p win pointer
 *  @param _x_   new x position of window 
 *  @param _y_   new y position of window 
 *  @see Pw_Window
 */
#define Pw_WindowSetPosition(_win_, _x_, _y_)  \
    Pw_WindowSetShape((_win_),                 \
                      (_x_),                   \
                      (_y_),                   \
                      (_win_)->area.w,         \
                      (_win_)->area.h)

/**
 *  Gets the size of window @p win.
 *  @param _win_ window @p win pointer
 *  @param _wp_  pointer to width of window (returned)
 *  @param _hp_  pointer to height of window (returned)
 *  @see Pw_Window
 */
#define Pw_WindowGetSize(_win_, _wp_, _hp_)    \
    PW_MACRO_START                             \
        *(_wp_) = (_win_)->area.w;             \
        *(_hp_) = (_win_)->area.h;             \
    PW_MACRO_END

/**
 *  Sets the size of window @p win.
 *  @param _win_ window @p win pointer
 *  @param _w_   new width of window 
 *  @param _h_   new height of window 
 *  @see Pw_Window
 */
#define Pw_WindowSetSize(_win_, _w_, _h_)      \
    Pw_WindowSetShape((_win_),                 \
                      (_win_)->area.x,         \
                      (_win_)->area.y,         \
                      (_w_),                   \
                      (_h_))

/**
 *  Gets the shape of window @p win.
 *  @param _win_ window @p win pointer
 *  @param _xp_  pointer to x position of window (returned)
 *  @param _yp_  pointer to y position of window (returned)
 *  @param _wp_  pointer to width of window (returned)
 *  @param _hp_  pointer to height of window (returned)
 *  @see Pw_Window
 */
#define Pw_WindowGetShape(_win_, _xp_, _yp_, _wp_, _hp_) \
    PW_MACRO_START                                       \
        *(_xp_) = (_win_)->area.x;                       \
        *(_yp_) = (_win_)->area.y;                       \
        *(_wp_) = (_win_)->area.w;                       \
        *(_hp_) = (_win_)->area.h;                       \
    PW_MACRO_END

/**
 *  Sets the focus acceptance of window @p win.
 *  @param _win_ window @p win pointer
 *  @see Pw_Window
 */   
#define Pw_WindowSetFocusAccept(_win_, _accept_) \
    ((_win_)->focus_accept = (_accept_))

/**
 *  Checks if window @p win is in focus.
 *  @param  _win_ window @p win pointer
 *  @return true if window is in focus 
 *  @see Pw_Window
 */ 
#define Pw_WindowIsInFocus(_win_) \
    ((_win_)->dpy->focus_win == (_win_))

/**
 *  Checks if window @p win is mapped.
 *  @param  _win_ window @p win pointer
 *  @return true if window is mapped 
 *  @see Pw_Window
 */ 
#define Pw_WindowIsMapped(_win_) \
    ((_win_)->mapped)

/**
 *  Gets the repaint callback of window @p win.
 *  @param  _win_ window @p win pointer
 *  @return the repaint callback of window 
 *  @see Pw_Window
 */ 
#define Pw_WindowGetRepaintCallback(_win_) \
    ((_win_)->repaint_cb)

/**
 *  Sets the repaint callback for window @p win.
 *  @param _win_ window @p win pointer
 *  @param _cb_  new repaint callback 
 *  @see Pw_Window
 */ 
#define Pw_WindowSetRepaintCallback(_win_, _cb_) \
    ((_win_)->repaint_cb = (_cb_))

/**
 *  Gets the event callback of window @p win.
 *  @param  _win_ window @p win pointer
 *  @return the event callback of window 
 *  @see Pw_Window
 */ 
#define Pw_WindowGetEventCallback(_win_) \
    ((_win_)->event_cb)

/**
 *  Sets the event callback for window @p win.
 *  @param _win_ window @p win pointer
 *  @param _cb_  new event callback 
 *  @see Pw_Window
 */ 
#define Pw_WindowSetEventCallback(_win_, _cb_) \
    ((_win_)->event_cb = (_cb_))

/**
 *  Gets the client data of window @p win.
 *  @param _win_ window @p win pointer
 *  @return the client data of window
 *  @see Pw_Window
 */     
#define Pw_WindowGetClientData(_win_) \
    ((_win_)->cdata)

/**
 *  Sets the client data for window @p win.
 *  @param _win_ window @p win pointer
 *  @param _data_ new client data
 *  @see Pw_Window
 */    
#define Pw_WindowSetClientData(_win_, _data_) \
    ((_win_)->cdata = (_data_))

/**
 *  Gets the client data id of window @p win.
 *  @param _win_ window @p win pointer
 *  @return the client data id of window
 *  @see Pw_Window
 */ 
#define Pw_WindowGetClientDataId(_win_) \
    ((_win_)->cdata_id)

/**
 *  Sets the client data id for window @p win.
 *  @param _win_ window @p win pointer
 *  @param _data_ new client data id
 *  @see Pw_Window
 */ 
#define Pw_WindowSetClientDataId(_win_, _id_) \
    ((_win_)->cdata_id = (_id_))

/**
 *  Gets the top child window of window @p win.
 *  @param _win_ window @p win pointer
 *  @return the top child window of window
 *  @see Pw_Window
 */     
#define Pw_WindowGetChildWindowOnTop(_win_) \
    ((_win_)->children)

/**
 *  Gets the window above window @p win.
 *  @param _win_ window @p win pointer
 *  @return the window above window
 *  @see Pw_Window
 */    
#define Pw_WindowGetWindowAbove(_win_) \
    ((_win_)->prev)

/**
 *  Gets the window below window @p win.
 *  @param _win_ window @p win pointer
 *  @return the window below window
 *  @see Pw_Window
 */ 
#define Pw_WindowGetWindowBelow(_win_) \
    ((_win_)->next)

/* -------------------------------------------------------------------------- */

/**
 *  Gets the height of font @p font.
 *  @param _font_ font @p font pointer
 *  @return the height of font 
 *  @see Pw_Font
 */ 
#define Pw_FontGetHeight(_font_) \
    ((_font_)->ascent + (_font_)->descent)

/**
 *  Gets the ascent of font @p font.
 *  @param _font_ font @p font pointer
 *  @return the ascent of font 
 *  @see Pw_Font
 */ 
#define Pw_FontGetAscent(_font_) \
    ((_font_)->ascent)

/**
 *  Gets the descent of font @p font.
 *  @param _font_ font @p font pointer
 *  @return the descent of font 
 *  @see Pw_Font
 */ 
#define Pw_FontGetDescent(_font_) \
    ((_font_)->descent)

/* -------------------------------------------------------------------------- */

/**
 *  Gets the type of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @return the type of graphic context 
 *  @see Pw_GC
 */ 
#define Pw_GCGetType(_gc_) \
    ((_gc_)->d.type)

/**
 *  Gets the window component of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @return pointer to window component of graphic context 
 *  @see Pw_GC
 */ 
#define Pw_GCGetWindow(_gc_) \
    ((_gc_)->d.comp.window)

/**
 *  Gets the bitmap component of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @return pointer to bitmap component of graphic context 
 *  @see Pw_GC
 */ 
#define Pw_GCGetBitmap(_gc_) \
    ((_gc_)->d.comp.bitmap)

/**
 *  Gets the current font width of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @return the current font width of graphic context 
 *  @see Pw_GC
 */ 
#define Pw_GCGetFontWidth(_gc_) \
    ((_gc_)->font->width)

/**
 *  Gets the current font height of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @return the current font height of graphic context 
 *  @see Pw_GC
 */ 
#define Pw_GCGetFontHeight(_gc_) \
    ((_gc_)->font->ascent + (_gc_)->font->descent)

/**
 *  Gets the current font ascent of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @return the current font ascent of graphic context 
 *  @see Pw_GC
 */ 
#define Pw_GCGetFontAscent(_gc_) \
    ((_gc_)->font->ascent)

/**
 *  Gets the current font descent of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @return the current font descent of graphic context 
 *  @see Pw_GC
 */ 
#define Pw_GCGetFontDescent(_gc_) \
    ((_gc_)->font->descent)

/**
 *  Gets the current color of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @return pointer to current color of graphic context 
 *  @see Pw_GC
 */ 
#define Pw_GCGetColor(_gc_) \
    ((_gc_)->color)

/**
 *  Sets the new color @p color of graphic context @p gc.
 *  @param _gc_    graphic context @p gc pointer
 *  @param _color_ new color @p color pointer
 *  @see Pw_GC
 */ 
#define Pw_GCSetColor(_gc_, _color_) \
    ((_gc_)->color = (_color_))

/**
 *  Gets the current font of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @return pointer to current font of graphic context 
 *  @see Pw_GC
 */ 
#define Pw_GCGetFont(_gc_) \
    ((_gc_)->font)

/**
 *  Sets the new font @p font of graphic context @p gc.
 *  @param _gc_   graphic context @p gc pointer 
 *  @param _font_ new font @p font pointer
 *  @see Pw_GC
 */ 
#define Pw_GCSetFont(_gc_, _font_) \
    ((_gc_)->font = (_font_))

/**
 *  Gets the string width in graphic context @p gc.
 *  @param _gc_     graphic context @p gc pointer
 *  @param _string_ zero terminated string 
 *  @return string width in pixels 
 *  @see Pw_GC
 */    
#define Pw_GCGetStringWidth(_gc_, _string_) \
    Pw_FontGetStringWidth((_gc_)->font, _string_)

/**
 *  Changes the position of (0, 0) point of graphic context @p gc.
 *  @param _gc_ graphic context @p gc pointer
 *  @param _xt_ x axis translation 
 *  @param _yt_ y axis translation 
 *  @see Pw_GC
 */    
#define Pw_GCTranslate(_gc_, _xt_, _yt_) \
    ((_gc_)->xoff += (_xt_), (_gc_)->yoff += (_yt_))

/* -------------------------------------------------------------------------- */

/**
 *  Gets the width of bitmap @p bitmap.
 *  @param _bitmap_ bitmap @p bitmap pointer
 *  @return the width of bitmap.
 *  @see Pw_Bitmap
 */ 
#define Pw_BitmapGetWidth(_bitmap_) \
    ((_bitmap_)->width)

/**
 *  Gets the height of bitmap @p bitmap.
 *  @param _bitmap_ bitmap @p bitmap pointer
 *  @return the height of bitmap.
 *  @see Pw_Bitmap
 */ 
#define Pw_BitmapGetHeight(_bitmap_) \
    ((_bitmap_)->height)

/* -------------------------------------------------------------------------- */

/**
 *  Gets the timeout trigger time @p to.
 *  @param _to_ timeout @p to pointer
 *  @return the trigger time of timeout.
 *  @see Pw_Timeout
 */ 
#define Pw_TimeoutGetTime(_to_) \
    ((_to_)->time)

/**
 *  Sets the timeout @p to trigger time.
 *  @param _to_   timeout @p to pointer
 *  @param _time_ new trigger time 
 *  @see Pw_Timeout
 */ 
#define Pw_TimeoutSetTime(_to_, _time_) \
    ((_to_)->time_left = (_to_)->time = _time_)

/**
 *  Gets the time left to timeout @p to trigger
 *  @param _to_ timeout @p to pointer
 *  @return the time left to timeout trigger
 *  @see Pw_Timeout
 */ 
#define Pw_TimeoutGetLeftTime(_to_) \
    ((_to_)->time_left)

/**
 *  Refreshes the timeout @p to (i.e. sets the time left to trigger time)
 *  @param _to_ timeout @p to pointer
 *  @see Pw_Timeout
 */     
#define Pw_TimeoutRefresh(_to_) \
    ((_to_)->time_left = (_to_)->time)

/* -------------------------------------------------------------------------- */

/**
 *  Gets the type of event @p ev.
 *  @param _ev_ event @p ev pointer
 *  @return the type of event
 *  @see Pw_Event
 */ 
#define Pw_EventGetType(_ev_) \
    ((_ev_)->data.type)

/**
 *  Gets the window from wich event @p ev originated.
 *  @param _ev_ event @p ev pointer
 *  @return the window of event
 *  @see Pw_Event
 */ 
#define Pw_EventGetWindow(_ev_) \
    ((_ev_)->data.win)

/**
 *  Casts the event @p ev to key event.
 *  @param _ev_ event @p ev pointer
 *  @return pointer to event cast to key event
 *  @see Pw_Event
 */     
#define Pw_EventCastToKeyEvent(_ev_) \
    ((Pw_KeyEvent*)&((_ev_)->key))

/**
 *  Casts the event @p ev to xinput event.
 *  @param _ev_ event @p ev pointer
 *  @return pointer to event cast to xinput event
 *  @see Pw_Event
 */ 
#define Pw_EventCastToXInputEvent(_ev_) \
    ((Pw_XInputEvent*)&((_ev_)->xinput))

/**
 *  Casts the event @p ev to focus event.
 *  @param _ev_ event @p ev pointer
 *  @return pointer to event cast to focus event
 *  @see Pw_Event
 */    
#define Pw_EventCastToFocusEvent(_ev_) \
    ((Pw_FocusEvent*)&((_ev_)->focus))

/**
 *  Casts the event @p ev to mapping event.
 *  @param _ev_ event @p ev pointer
 *  @return pointer to event cast to mapping event
 *  @see Pw_Event
 */ 
#define Pw_EventCastToMappingEvent(_ev_) \
    ((Pw_MappingEvent*)&((_ev_)->mapping))

/**
 *  Casts the event @p ev to reshape event.
 *  @param _ev_ event @p ev pointer
 *  @return pointer to event cast to reshape event
 *  @see Pw_Event
 */ 
#define Pw_EventCastToReshapeEvent(_ev_) \
    ((Pw_ReshapeEvent*)&((_ev_)->reshape))

/**
 *  Casts the event @p ev to destroy event.
 *  @param _ev_ event @p ev pointer
 *  @return pointer to event cast to destroy event
 *  @see Pw_Event
 */ 
#define Pw_EventCastToDestroyEvent(_ev_) \
    ((Pw_DestroyEvent*)&((_ev_)->destroy))

/* -------------------------------------------------------------------------- */

/**
 *  Gets the key code of key event @p ev.
 *  @param _ev_ key event @p ev pointer
 *  @return key code of key event 
 *  @see Pw_KeyEvent
 */ 
#define Pw_KeyEventGetKeyCode(_ev_) \
    ((_ev_)->keycode)

/* -------------------------------------------------------------------------- */

/** 
 *  Gets the number of xinput event @p ev.
 *  @param _ev_ xinput event @p ev pointer
 *  @return the number of xinput event.
 *  @see Pw_XInputEvent
 */     
#define Pw_XInputEventGetNumber(_ev_) \
    ((_ev_)->num)

/** 
 *  Gets the value of xinput event @p ev.
 *  @param _ev_ xinput event @p ev pointer
 *  @return the value of xinput event.
 *  @see Pw_XInputEvent
 */ 
#define Pw_XInputEventGetValue(_ev_) \
    ((_ev_)->val)

/* -------------------------------------------------------------------------- */

Pw_Window* Pw_WindowCreate(Pw_Window* parent,
                           Pw_Coord   x,
                           Pw_Coord   y,
                           Pw_Coord   w,
                           Pw_Coord   h,
                           Pw_Window* win);

void       Pw_WindowDestroy(Pw_Window* win);

Pw_Bool    Pw_WindowSetFocus(Pw_Window* win);

Pw_Bool    Pw_WindowLeaveFocus(Pw_Window* win);

void       Pw_WindowMap(Pw_Window* win);

void       Pw_WindowUnmap(Pw_Window* win);

void       Pw_WindowRepaint(Pw_Window* win);

void       Pw_WindowRepaintPart(Pw_Window* win, Pw_Rectangle* clip);

void       Pw_WindowSetXInputListener(Pw_Window* win, Pw_uInt num);

void       Pw_WindowUnsetXInputListener(Pw_Window* win, Pw_uInt num);

void       Pw_WindowSetShape(Pw_Window* win,
                             Pw_Coord   x,
                             Pw_Coord   y,
                             Pw_Coord   w,
                             Pw_Coord   h);

void       Pw_WindowRaiseOne(Pw_Window* win);

void       Pw_WindowLowerOne(Pw_Window* win);

void       Pw_WindowPlaceAbove(Pw_Window* win, Pw_Window* rwin);

void       Pw_WindowPlaceBelow(Pw_Window* win, Pw_Window* rwin);

void       Pw_WindowPlaceOnTop(Pw_Window* win);

void       Pw_WindowPlaceOnBottom(Pw_Window* win);

Pw_Int     Pw_WindowGetNewClientDataId(void);

/* -------------------------------------------------------------------------- */

Pw_uInt Pw_FontGetStringWidth(Pw_Font* font, Pw_Char* string);

/* -------------------------------------------------------------------------- */

void Pw_DrawPoint(Pw_GC* gc, Pw_Coord x, Pw_Coord y);

void Pw_DrawRect(Pw_GC*   gc, 
                 Pw_Coord x, 
                 Pw_Coord y, 
                 Pw_Coord w, 
                 Pw_Coord h);

void Pw_FillRect(Pw_GC*   gc, 
                 Pw_Coord x, 
                 Pw_Coord y, 
                 Pw_Coord w, 
                 Pw_Coord h);

void Pw_DrawLine(Pw_GC*   gc, 
                 Pw_Coord x1, 
                 Pw_Coord y1, 
                 Pw_Coord x2, 
                 Pw_Coord y2);

void Pw_DrawCircle(Pw_GC*   gc, 
                   Pw_Coord x, 
                   Pw_Coord y, 
                   Pw_Coord r); 

void Pw_FillCircle(Pw_GC*   gc, 
                   Pw_Coord x, 
                   Pw_Coord y, 
                   Pw_Coord r); 

void Pw_DrawEllipse(Pw_GC*   gc, 
                    Pw_Coord x, 
                    Pw_Coord y, 
                    Pw_Coord rx, 
                    Pw_Coord ry);

void Pw_FillEllipse(Pw_GC*   gc, 
                    Pw_Coord x, 
                    Pw_Coord y, 
                    Pw_Coord rx, 
                    Pw_Coord ry);

void Pw_DrawBitmap(Pw_GC*     gc, 
                   Pw_Coord   x, 
                   Pw_Coord   y, 
                   Pw_Bitmap* bitmap);

void Pw_DrawString(Pw_GC*   gc, 
                   Pw_Coord x, 
                   Pw_Coord y, 
                   Pw_Char* string);

/* -------------------------------------------------------------------------- */

Pw_Bitmap* Pw_BitmapCreate(Pw_Coord   width,
                           Pw_Coord   height,
                           Pw_Byte*   bits,
                           Pw_Bitmap* bitmap);

void       Pw_BitmapDestroy(Pw_Bitmap* bitmap);

Pw_GC*     Pw_BitmapCreateGC(Pw_Bitmap* bitmap, Pw_GC* gc);

void       Pw_BitmapDestroyGC(Pw_GC* gc);

/* -------------------------------------------------------------------------- */

Pw_Timeout* Pw_TimeoutCreate(Pw_Display*  dpy,
                             Pw_TimeoutCb cb,
                             Pw_Pointer   data,
                             Pw_uLong     time,
                             Pw_Timeout*  to);

void        Pw_TimeoutDestroy(Pw_Display* dpy, Pw_Timeout* to);

/* -------------------------------------------------------------------------- */

Pw_Int Pw_DisplayGetXInputValue(Pw_Display* dpy, Pw_uInt num);

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* -------------------------------------------------------------------------- */

#endif /* not PWIN_H */

/*---------------------------------------------------------------------------
// end of pwin.h 
//--------------------------------------------------------------------------- */
