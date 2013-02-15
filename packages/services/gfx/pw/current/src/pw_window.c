/*===========================================================================
//
//        pw_window.c 
//
//        PW graphic library window code 
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

#define _WINDOW_IS_CLIP_REGION_NON_EMPTY(_win_)  \
        ((_win_)->clip_reg.regs_num > 0)
 
#define _WINDOW_SET_CLIP_REGION_TO_ALL(_win_)                         \
    IPW_MACRO_START                                                   \
        if (PW_NULL != (_win_)->clip_reg.regs)                        \
        {                                                             \
            IPW_CHECK_COND((_win_)->clip_reg.regs_num > 0);           \
            IPw_RegionFreeList((_win_)->clip_reg.regs);               \
        }                                                             \
        (_win_)->clip_reg.regs = IPw_RegionNew();                     \
        IPW_FAIL((_win_)->clip_reg.regs != PW_NULL,                   \
                 ("Out of free regions!"));                           \
        (_win_)->clip_reg.regs_num = 1;                               \
        IPW_RECT_COPY(&(_win_)->clip_reg.regs->area, &(_win_)->area); \
        IPW_RECT_COPY(&(_win_)->clip_reg.bounds, &(_win_)->area);     \
    IPW_MACRO_END 

#define _WINDOW_SET_CLIP_REGION_TO_EMPTY(_win_)                    \
    IPW_MACRO_START                                                \
        if (PW_NULL != (_win_)->clip_reg.regs)                     \
        {                                                          \
            IPW_CHECK_COND((_win_)->clip_reg.regs_num > 0);        \
            IPw_RegionFreeList((_win_)->clip_reg.regs);            \
        }                                                          \
        (_win_)->clip_reg.regs = PW_NULL;                          \
        (_win_)->clip_reg.regs_num = 0;                            \
        IPW_RECT_EMPTY(&(_win_)->clip_reg.bounds);                 \
    IPW_MACRO_END 

#define _WINDOW_COPY_CLIP_REGION(_dst_, _src_)                     \
    IPW_MACRO_START                                                \
        (_dst_)->regs_num = (_src_)->regs_num;                     \
        (_dst_)->regs = IPw_RegionNewListFromList((_src_),         \
            &(_dst_)->regs_num, PW_NULL);                          \
        IPW_FAIL((_dst_)->regs_num == (_src_)->regs_num,           \
                 ("Out of free regions!"));                        \
        IPW_RECT_COPY(&(_dst_)->bounds, &(_src_)->bounds);         \
    IPW_MACRO_END 

#define _WINDOW_FREE_CLIP_REGION(_creg_)                           \
    IPW_MACRO_START                                                \
        if (PW_NULL != (_creg_)->regs)                             \
        {                                                          \
            IPW_CHECK_COND((_creg_)->regs_num > 0);                \
            IPw_RegionFreeList((_creg_)->regs);                    \
        }                                                          \
    IPW_MACRO_END
 
/* -------------------------------------------------------------------------- */

/**
 *  Next free client data id.
 *  @see Pw_WindowGetNewClientDataId
 */ 
static Pw_Int _cdata_next_id = 1;

/* -------------------------------------------------------------------------- */

/**
 *  Initializes basic new window attributes.
 *  @param dpy pointer to display of window
 *  @param win pointer to window to create
 *  @return a newly created window (same as @p win)
 *  @internal
 */ 
static Pw_Window* 
_WindowNew(Pw_Display* dpy, Pw_Window* win)
{ 
    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(dpy);
    IPW_CHECK_PTR(win);
    IPW_TRACE3(WN2, p, win, win->parent, dpy);

    /* Quick check for empty window space */
    IPW_FAIL(PW_NULL == win->parent, ("dirty or used window space %p", win));

    /* Set the display */
    win->dpy = dpy;

    /* Set the window area */
    IPW_RECT_EMPTY(&win->area);

    /* Set other window attributes */
    win->parent       = PW_NULL;
    win->children     = PW_NULL;
    win->children_num = 0;
    win->prev         = PW_NULL;
    win->next         = PW_NULL;
    win->focus_accept = FALSE;
    win->mapped       = FALSE;

    /* Set window clip region to empty */
    win->clip_reg.regs_num = 0;
    win->clip_reg.regs = PW_NULL;
    IPW_RECT_EMPTY(&win->clip_reg.bounds);
    
    /* Set window callback functions to null */
    win->event_cb   = PW_NULL;
    win->repaint_cb = PW_NULL;

    /* Set client data to null */
    win->cdata_id = -1;
    win->cdata    = PW_NULL;

    IPW_TRACE_EXIT_V(WN2, p, win);
    return(win);
}

/**
 *  Adds window @p win to parent window @p parent.
 *  @param win    pointer to window
 *  @param parent pointer to parent window
 *  @internal
 */ 
static void 
_WindowAddToParent(Pw_Window* win, Pw_Window* parent)
{
    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(win);
    IPW_CHECK_PTR(parent);
    IPW_TRACE2(WN2, p, win, parent);
    IPW_TRACE3(WN1, p, win->next, win->prev, win->parent);
    IPW_TRACE1(WN1, d, parent->children_num);

    /* 
     * Window should not be added to another window 
     * if it already has a parent
     */
    IPW_CHECK_COND(PW_NULL == win->parent); 

    /* Increase parent window child count */
    parent->children_num++;

    /* Check if this window is the first child of parent window */ 
    if (PW_NULL == parent->children)
    {
        /* First child */
        parent->children = win;
        win->prev = PW_NULL;
        win->next = PW_NULL;
    }
    else
    {
        /* Not the first child - add the window on top (start of the list) */
        parent->children->prev = win;
        win->next = parent->children;
        win->prev = PW_NULL;
        parent->children = win;
    }

    /* Set the window parent pointer */
    win->parent = parent;

    IPW_TRACE3(WN1, p, win->next, win->prev, win->parent);
    IPW_TRACE1(WN1, d, parent->children_num);
    IPW_TRACE_EXIT(WN2);
}

/**
 *  Removes window @p win from parent window.
 *  @param win pointer to window
 *  @internal
 */  
static void 
_WindowRemoveFromParent(Pw_Window* win)
{
    Pw_Window* cld_win;
    Pw_Window* pre_win;

    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2(WN2, p, win, win->parent);
    IPW_CHECK_COND(PW_NULL != win->parent);
    IPW_TRACE1(WN1, d, win->parent->children_num);

    /* Find previous window of this window in its parent child list */

    cld_win = win->parent->children;
    pre_win = PW_NULL;

    while ((cld_win != win) && (PW_NULL != cld_win))
    {
        pre_win = cld_win;
        cld_win = cld_win->next;
    }

    IPW_CHECK_COND(PW_NULL != cld_win); 

    IPW_TRACE2(WN1, p, pre_win, cld_win->next);

    if (PW_NULL == pre_win) 
    {
        /* 
         * This window is the first window in its parent
         * child list - just correct parents first child pointer
         */ 
        win->parent->children = cld_win->next;
        if (PW_NULL != cld_win->next)
            cld_win->next->prev = PW_NULL;
    }
    else
    {
        /*
         * This window is not the first window in its parent
         * child list - corrent previous window next pointer
         * to point to this window next window
         */ 
        pre_win->next = cld_win->next;
        if (PW_NULL != cld_win->next)
            cld_win->next->prev = pre_win;
    }

    /* Decrease this window parent child count */
    win->parent->children_num--;

    IPW_TRACE1(WN1, d, win->parent->children_num);

    /* Set window parent to NULL */
    win->parent = PW_NULL;

    IPW_TRACE_EXIT(WN2);
}

/**
 *  Sets the clip region of all of window @p win children to empty (invisible).
 *  @param win pointer to window
 *  @internal
 */ 
static void 
_WindowSetChildrenClipRegionToEmpty(Pw_Window* win)
{
    Pw_Window* cld_win;

    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2(WN1, p, win, win->children);

    /* 
     * Set the clip region of current child window to empty and
     * then call this function recursively for all of current 
     * child window children
     */ 
    cld_win = win->children;
    while (PW_NULL != cld_win)
    {
        IPW_TRACE1(WN1, p, cld_win);
        _WINDOW_SET_CLIP_REGION_TO_EMPTY(cld_win);
        _WindowSetChildrenClipRegionToEmpty(cld_win);
        cld_win = cld_win->next;
    } 
    IPW_TRACE_EXIT(WN2);
}

/**
 *  Updates window @p win clip region and the clip region of its children.
 *  @param win pointer to window
 *  @internal
 */  
static void 
_WindowUpdateClipRegion(Pw_Window* win)
{
    Pw_Window*     par_win;  
    Pw_Window*     cur_win;    
    Pw_Window*     tmp_win;   
    Pw_RegionList* clip_reg; 
    Pw_Region*     reg;     

    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(win);
    
    /* If this window is not mapped we have nothing to do */
    if (!win->mapped)
    {
        IPW_TRACE_EXIT(WN2);
        return;
    }

    /* 
     * Start with the whole window area and then remove the obscured parts 
     */
    _WINDOW_SET_CLIP_REGION_TO_ALL(win);

    /* Set the ptr to this window clip region */
    clip_reg = &win->clip_reg;

    /*
     * Find the visible region of this window trough all of
     * its parents (up to the root window wich parent is NULL)
     */ 
    par_win = win->parent;
    while (PW_NULL != par_win)
    {
        IPW_TRACE_RECT2(WN1, &par_win->area, &clip_reg->regs->area);

        /* Calculate the visible window part trough the current parent */
        IPw_RectIntersect(&clip_reg->regs->area, &par_win->area);

        IPW_TRACE_RECT(WN1, &clip_reg->regs->area);

        if (!IPW_RECT_IS_VALID(&clip_reg->regs->area))
        {
            /* 
             * This window is not visible at all (and all
             * of its children are not visible as well) 
             */
            _WINDOW_SET_CLIP_REGION_TO_EMPTY(win);
            _WindowSetChildrenClipRegionToEmpty(win);
            IPW_TRACE_EXIT(WN2);
            return;
        }
        par_win = par_win->parent;
    }

    /*
     * Remove parts wich are obscured by windows on top of this window - this
     * are the children of this window parent that are on top of this window
     * and the children of this window parent parent that are on top of this 
     * window parent, and so on until we reach the root window
     */
    tmp_win = win; 
    par_win = win->parent; 
    while (PW_NULL != par_win)
    {
        IPW_TRACE1(WN1, p, par_win);
        /*
         *  Exclude window area of windows on top of this
         *  window or on top of one of this window parents -
         *  we stop when we reach tmp_win (in the first loop
         *  tmp_win is this window and in the following loop its
         *  one of this window parents)
         */ 
        cur_win = par_win->children; /* Child window on top */
        while (cur_win != tmp_win)
        {
            IPW_TRACE2M(WN1, p, cur_win, d, cur_win->mapped);

            /* If window is not mapped it can't obscure anything */
            if (cur_win->mapped)
            {
                /* 
                 * Exclude current window area from this window and check
                 * if the number of clip rectangles has reached the limit
                 */ 
                if (!IPw_RectExcludeFromRegionList(&(cur_win->area), clip_reg))
                {
                    Pw_Int fcnt;

                    /*
                     *  Too many clip rectangles - just make this window 
                     *  and all of its children invisible
                     */ 
                    IPW_WARN(FALSE, ("Out of free regions for window (%p)!", 
                             win));

                    IPW_TRACE1(WN1, d, win->clip_reg.regs_num);

                    _WINDOW_SET_CLIP_REGION_TO_EMPTY(win);
                    _WindowSetChildrenClipRegionToEmpty(win);

                    fcnt = IPw_RegionGetFreeCnt();
                    IPW_TRACE1(WN1, d, fcnt);
                    IPW_TRACE_EXIT(WN2);
                    return;
                }
                IPW_TRACE1(WN1, d, clip_reg->regs_num);
            }
            cur_win = cur_win->next;
        }
        tmp_win = par_win;
        par_win = par_win->parent;
    }

    /* Calculate the bounds of this window clip region */
    if (PW_NULL != clip_reg->regs) 
    {
        reg = clip_reg->regs;
        IPW_RECT_COPY(&clip_reg->bounds, &reg->area);

        IPW_TRACE_RECT(WN1, &reg->area);

        reg = reg->next;
        while (PW_NULL != reg)
        {
            IPW_TRACE_RECT(WN1, &reg->area);

            IPw_RectUnionize(&clip_reg->bounds, &reg->area);
            reg = reg->next;
        }
    }
    else
    {
        IPW_RECT_EMPTY(&clip_reg->bounds);
    }

    IPW_TRACE_RECT(WN1, &clip_reg->bounds);
    IPW_CHECK_RLIST_HARD(&win->clip_reg);

    /* Update the clip region for all children of this window */ 
    cur_win = win->children;
    while (PW_NULL != cur_win)
    {
        _WindowUpdateClipRegion(cur_win);
        cur_win = cur_win->next;
    }

    IPW_TRACE_EXIT(WN2);
}

/**
 *  Creates window @p win graphic context @p gc with clip region @p clip.
 *  @param win  pointer to window
 *  @param clip pointer to clip rectangle
 *  @param gc   pointer to gc
 *  @return true if gc applays to a visible region 
 *  @internal
 */ 
static Pw_Bool 
_WindowCreateGC(Pw_Window* win, Pw_Rectangle* clip, Pw_GC* gc)
{
    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR2(win, gc);
    IPW_TRACE3(WN1, p, win, clip, gc);


    gc->d.type = Pw_DrawableWindowType; /* Set the type */
    gc->d.comp.window = win;          /* Set the window */
    gc->d.lld = win->dpy->dd->lld;    /* Set low level drawing routines */
    gc->xoff  = win->area.x;          /* Set the x offset (origin) */
    gc->yoff  = win->area.y;          /* Set the y offset (origin) */
    gc->color = pw_black_pixel;       /* Set the color */
    gc->font  = PW_NULL;              /* Set the font REMIND: default font?!? */

    /* 
     * Check if this GC applays to whole window 
     * area or just a part of window area
     */ 
    if (PW_NULL == clip)
    {
        /* GC applays to whole window area */

        gc->clip_reg.regs     = win->clip_reg.regs;
        gc->clip_reg.regs_num = win->clip_reg.regs_num;
        IPW_RECT_COPY(&gc->clip_reg.bounds, &win->clip_reg.bounds);
    }
    else
    {
        /* GC applays only to a part of window area */

        Pw_Rectangle r;
        Pw_Region*   wreg;
        Pw_Region*   greg;
    
        IPW_TRACE_RECT(WN1, clip);

        gc->clip_reg.regs = PW_NULL;
        gc->clip_reg.regs_num = 0;
        IPW_RECT_EMPTY(&gc->clip_reg.bounds);

        IPW_TRACE_RLIST(WN1, &win->clip_reg);

        greg = PW_NULL;
        wreg = win->clip_reg.regs;

        while (PW_NULL != wreg)
        {
            IPW_RECT_COPY(&r, &wreg->area);
            if (IPw_RectIntersect(&r, clip))
            {
                if (0 == gc->clip_reg.regs_num)
                {
                    IPW_RECT_COPY(&gc->clip_reg.bounds, &r);
                    greg = gc->clip_reg.regs = IPw_RegionNew();
                }
                else
                {
                    IPw_RectUnionize(&gc->clip_reg.bounds, &r);
                    greg->next = IPw_RegionNew();
                    greg = greg->next;
                }
                IPW_FAIL(PW_NULL != greg, ("Out of free regions!"));
                IPW_RECT_COPY(&greg->area, &r);
                gc->clip_reg.regs_num++;

                IPW_TRACE_RECT(WN1, &greg->area);
                IPW_TRACE_RLIST(WN1, &gc->clip_reg); 
            }    
            wreg = wreg->next;
        }
    }
    IPW_TRACE_RLIST(WN1, &gc->clip_reg);
    IPW_CHECK_GC(gc);
    IPW_TRACE_EXIT_V(WN2, d, (gc->clip_reg.regs_num > 0));
    return(gc->clip_reg.regs_num > 0);
}

/**
 *  Destroys graphic context @p gc.
 *  @param gc pointer to gc
 *  @internal
 */ 
static void 
_WindowDestroyGC(Pw_GC* gc)
{
    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(gc);
    IPW_CHECK_GC(gc);
    IPW_TRACE2(WN1, p, gc->clip_reg.regs, gc->d.comp.window->clip_reg.regs);

    /* 
     * If gc doesn't have any extra clip, then the clip region
     * points to its window clip region - so we must not free it!
     * See _WindowCreateGC
     */
    if (gc->clip_reg.regs != gc->d.comp.window->clip_reg.regs)
        IPw_RegionFreeList(gc->clip_reg.regs);

    IPW_TRACE_EXIT(WN2);
}

/**
 *  Repaints the region @p clip of window @p win and its children if requested.
 *  If the region @p clip equals NULL the whole window is repainted.
 *  @param win pointer to window
 *  @param clip pointer to clip rectangle (area of window @p win to repaint)
 *  @param repaint_childs if true repaint all window @p win children 
 *  @internal
 */ 
static void 
_WindowRepaint(Pw_Window* win, Pw_Rectangle* clip, Pw_Bool repaint_childs)
{
    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE3M(WN1, p, win, p, clip, d, repaint_childs);

    /* Start repaint only if this window is visible and mapped */ 
    if (_WINDOW_IS_CLIP_REGION_NON_EMPTY(win) && win->mapped)
    {
        Pw_GC gc;

        /*
         * Get this window gc and continue with repaint
         * only if gc's clip region is nonempty
         */ 
        if (_WindowCreateGC(win, clip, &gc))
        {
            /* Send repaint event */
            IPw_EventSendRepaint(win, &gc); 

            /* Repaint this window children if requested */
            if (repaint_childs)
            {
                Pw_Window* cld_win; /* Child window */

                cld_win = win->children;
                while (PW_NULL != cld_win)
                {
                    IPW_TRACE1(WN1, p, cld_win);

                    _WindowRepaint(cld_win, clip, TRUE);
                    cld_win = cld_win->next;
                }
            }
            _WindowDestroyGC(&gc);
        }
    }
    IPW_TRACE_EXIT(WN2);
}

/**
 *  Updates the clip region of all windows and their children 
 *  below window @p win.
 *  @param win pointer to window
 *  @internal
 */ 
static void 
_WindowUpdateClipRegionsBelow(Pw_Window* win)
{
    Pw_Window* blw_win;

    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(win);

    blw_win = win->next;
    while (PW_NULL != blw_win)
    {
        IPW_TRACE1(WN1, p, blw_win);

        _WindowUpdateClipRegion(blw_win);
        blw_win = blw_win->next;
    }    
    IPW_TRACE_EXIT(WN2);
}

/**
 *  Repaint the region @p clip of all windows and their children 
 *  below window @p win area.
 *  @param win pointer to window
 *  @param clip pointer to clip rectangle
 *  @internal
 */ 
static void 
_WindowRepaintBelow(Pw_Window* win, Pw_Rectangle* clip)
{
    Pw_Window* blw_win; 

    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2(WN1, p, win, clip);

    blw_win = win->next;
    while (PW_NULL != blw_win)
    {
        IPW_TRACE1(WN1, p, blw_win);

        if (PW_NULL == clip || IPW_RECT_IS_OVER(clip, &blw_win->area))
            _WindowRepaint(blw_win, clip, TRUE);
        blw_win = blw_win->next;
    }
    IPW_TRACE_EXIT(WN2);
}

/**
 *  Refreshes the display. 
 *  This function should be called after all repainting is done -
 *  two separate areas can be specified, if areas intersect the 
 *  refreshing will be optimized.
 *  @param dpy pointer to display
 *  @param r1  pointer to display area 1
 *  @param r2  pointer to display area 2
 *  @internal
 */ 
static void 
_DisplayRefresh(Pw_Display* dpy, Pw_Rectangle* r1, Pw_Rectangle* r2)
{
    Pw_Rectangle rect;

    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR2(dpy, dpy->dd);
    IPW_TRACE2(WN1, p, r1, r2);

    if (PW_NULL == r1)
    {
        IPW_TRACE_EXIT(WN2);
        return;
    }

    IPW_RECT_COPY(&rect, r1);
    
    if (PW_NULL == r2)
    {
        IPW_TRACE_RECT(WN1, &rect);

        dpy->dd->display_refresh(dpy, &rect);
        IPW_TRACE_EXIT(WN2);
        return;
    }

    if (IPW_RECT_IS_OVER(r1, r2))
    {
        IPw_RectUnionize(&rect, r2);

        IPW_TRACE_RECT(WN1, &rect);

        dpy->dd->display_refresh(dpy, &rect);
    }
    else
    {
        IPW_TRACE_RECT(WN1, &rect);

        dpy->dd->display_refresh(dpy, &rect);

        IPW_RECT_COPY(&rect, r2);

        IPW_TRACE_RECT(WN1, &rect);

        dpy->dd->display_refresh(dpy, &rect);
    }
    IPW_TRACE_EXIT(WN2);
}

/**
 *  Sets the mapped flag of all window win children to @p mapped flag.
 *  @param win    pointer to window
 *  @param mapped mapped flag 
 *  @internal
 */ 
static void 
_WindowSetChildrensMappedFlag(Pw_Window* win, Pw_Bool mapped)
{
    Pw_Window* cld_win; 

    IPW_TRACE_ENTER(WN2);
    IPW_CHECK_PTR(win);
    IPW_TRACE2M(WN1, p, win, d, mapped);

    cld_win = win->children;

    /* Set mapped flag for all of this window children */
    while (PW_NULL != cld_win)
    {
        IPW_TRACE1(WN1, p, cld_win);

        cld_win->mapped = mapped;
        _WindowSetChildrensMappedFlag(cld_win, mapped);
        cld_win = cld_win->next;
    }
    IPW_TRACE_EXIT(WN2);
}

/* -------------------------------------------------------------------------- */

/**
 *  Creates the root window for new display.
 *  @param dpy    pointer to new display 
 *  @param width  new display width 
 *  @param height new display height
 *  @return the pointer to root window of new display
 *  @internal 
 */ 
Pw_Window*
IPw_RootWindowCreate(Pw_Display* dpy, Pw_Coord width, Pw_Coord height)
{
    Pw_Window* root;

    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(dpy);
    IPW_CHECK_COND(width > 0 && height > 0);
    IPW_TRACE2(WN2, d, width, height);

    /* Create new window */
    root = _WindowNew(dpy, &dpy->root_win);

    /* Set root window area */
    root->area.x = 0;
    root->area.y = 0;
    root->area.w = width;
    root->area.h = height;

    /* Set root window clip region to all */
    _WINDOW_SET_CLIP_REGION_TO_ALL(root);

    /* Set the mapped flag */
    root->mapped = TRUE;

    IPW_CHECK_WINDOW(root);

    IPW_TRACE_EXIT_V(WN3, p, root);
    return(root);
}

/**
 *  Clears window @p win event hooks.
 *  @param win pointer to window
 *  @internal
 */ 
void 
IPw_WindowClearEventHooks(Pw_Window* win)
{
	Pw_uInt i;

    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE1(WN2, p, win);
 
	/* Clear window win xinput hooks (if any) */
	for (i = 0; i < PW_XINPUTS_MAX_NUM; i++)
	{
		if (win->dpy->xinput_win[i] == win)
        {
            IPW_TRACE1(WN1, d, i);
			win->dpy->xinput_win[i] = PW_NULL;
        }
	}
    IPW_TRACE_EXIT(WN2);
}

#if IPW_CHECK_EN(WINDOW_STRUCT)
Pw_Bool
IPw_WindowCheck(Pw_Window* win)
{
    Pw_Int     i;
    Pw_Window* cld_win;

    if (PW_NULL == win->dpy)
    {
        IPW_ASSERT(FALSE, ("Window (%p) display == NULL!", win));
        return(FALSE);
    } 
    if (win->area.w < 0 || win->area.h < 0)
    {
        IPW_ASSERT(FALSE, ("Window (%p) area w = %d h = %d!", 
                   win, win->area.w,  win->area.h));
        return(FALSE);
    }
    if (win->parent == PW_NULL && win != &win->dpy->root_win)
    {
        IPW_ASSERT(FALSE, ("Window (%p) parent == NULL && win != root!",
                   win));
        return(FALSE);
    }
    if (win == win->dpy->focus_win && !win->focus_accept)
    {
        IPW_ASSERT(FALSE, ("Window (%p) in focus, but doesn't accept focus!",
                   win));
        return(FALSE);
    }
   
    i = 0;
    cld_win = win->children;
    while (PW_NULL != cld_win)
    {
        if (PW_NULL != cld_win->next && cld_win->next->prev != cld_win)
        {
            IPW_ASSERT(FALSE, ("Window (%p) child list broken at %d"
                       " (clds %d %d)!", win, i, cld_win, 
                       cld_win->next));
            return(FALSE); 
        }
        cld_win = cld_win->next;
        i++;
    }

    if (i != win->children_num)
    {
        IPW_ASSERT(FALSE, ("Window (%p) children list num error %d != %d!",
                   win, i, win->children_num));
        return(FALSE);
    }

    if (PW_NULL != win->parent)
    {
        cld_win = win->parent->children;
        while (PW_NULL != cld_win && win != cld_win)
        {
            if (PW_NULL != cld_win->next && cld_win->next->prev != cld_win)
            {
                IPW_ASSERT(FALSE, ("Window (%p) child list broken"
                           " (clds %d %d)!", win->parent, i, cld_win, 
                           cld_win->next));
                return(FALSE); 
            }
            cld_win = cld_win->next;
        }
        if (cld_win != win)
        {
            IPW_ASSERT(FALSE, ("Window (%p) not in its parent (%p) child list!",
                       win, win->parent));
            return(FALSE);
        }
    }

#if IPW_CHECK_EN(REGION_LIST)
    if (!IPw_RegionListCheck(&win->clip_reg, TRUE))
    {
        IPW_ASSERT(FALSE, ("Window (%p) clip region error!", win));
        return(FALSE);
    }
#endif /* IPW_CHECK_EN(REGION_LIST) */

    return(TRUE);
}
#endif /* IPW_CHECK_EN(WINDOW_STRUCT) */

/* -------------------------------------------------------------------------- */

/**
 *  Creates a new window.
 *  @param parent pointer to new window parent
 *  @param x new window top left corner x coord
 *  @param y new window top left corner y coord
 *  @param w new window width
 *  @param h new window height 
 *  @param win pointer to window to create
 *  @return pointer to newly created window
 */ 
Pw_Window* 
Pw_WindowCreate(Pw_Window* parent, 
                Pw_Coord   x, 
                Pw_Coord   y, 
                Pw_Coord   w, 
                Pw_Coord   h,
                Pw_Window* win)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR2(parent, win);
    IPW_TRACE3(WN2, p, win, parent, win->parent);
    IPW_CHECK_COND(win->parent == PW_NULL);

    /* Create new window */
    _WindowNew(parent->dpy, win);

    /* Add window to parent */
    _WindowAddToParent(win, parent);

    /* Set the window rectangle */
    win->area.x  = x + parent->area.x;
    win->area.y  = y + parent->area.y;
    win->area.w  = w;
    win->area.h  = h;

    IPW_TRACE_RECT(WN1, &win->area);
    IPW_CHECK_WINDOW(win);

    /* 
     * This window is not yet mapped, so the 
     * calculation of clip region is left to
     * the map window function
     */ 

    IPW_TRACE_EXIT_V(WN3, p, win);
    return(win);
}

/**
 *  Destroys a window @p win.
 *  @param win pointer to window to be destroyed
 */ 
void 
Pw_WindowDestroy(Pw_Window* win)
{
    Pw_Window* cld_win;
    Pw_Window* tmp_win; 

    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE1(WN2, p, win);

    /* Send destroy event */
    IPw_EventSendDestroy(win);

    /* 
     * Unmap window to be destroyed -
     * this will take care of clipping
     * regions and repaints of other windows 
     */
    Pw_WindowUnmap(win);

    /* Destroy all of this window children */
    cld_win = win->children;
    while (PW_NULL != cld_win)
    {
        IPW_TRACE1(WN1, p, cld_win);

        tmp_win = cld_win;
        cld_win = cld_win->next;
        Pw_WindowDestroy(tmp_win);
    }

    /* Clear event hooks */
    IPw_WindowClearEventHooks(win);

    /* If this is the root window then we are done */
    if (win == &win->dpy->root_win)
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    IPW_CHECK_PTR(win->parent);
    tmp_win = win->parent;
    
    /* Remove it from its parent */
    _WindowRemoveFromParent(win);

    IPW_CHECK_WINDOW(tmp_win);
    IPW_TRACE_EXIT(WN3);
} 

/**
 *  Sets window @p win in focus.
 *  @param win pointer to window
 *  @return true if focus was granted
 */ 
Pw_Bool 
Pw_WindowSetFocus(Pw_Window* win)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE2(WN2, d, win->focus_accept, win->mapped);

    /* 
     * Start focus transfer only if target window (win) 
     * accepts focus and it is mapped
     */ 
    if (win->focus_accept && win->mapped)
    {
        IPW_TRACE1(WN1, p, win->dpy->focus_win);

        /* 
         * If another window had focus send focus out
         * event to it
         */ 
        if (PW_NULL != win->dpy->focus_win)
        {
            Pw_Window* fwin = win->dpy->focus_win;

            win->dpy->focus_win = PW_NULL;
            IPw_EventSendFocusOut(fwin);
        }

        /* Set the window in focus for this window display */
        win->dpy->focus_win = win;

        /* Send focus in event to this window */
        IPw_EventSendFocusIn(win);

        IPW_TRACE1(WN1, p, win->dpy->focus_win);

        IPW_TRACE_EXIT_V(WN3, d, TRUE);
        return(TRUE);
    }
    IPW_TRACE_EXIT_V(WN3, d, FALSE);
    return(FALSE);
}

/**
 *  Sets window @p win out of focus.
 *  @param win pointer to window
 *  @return TRUE if focus has left @p win  
 */ 
Pw_Bool 
Pw_WindowLeaveFocus(Pw_Window* win)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE2(WN2, p, win->dpy->focus_win, win);

    if (win->dpy->focus_win == win)
    {
        win->dpy->focus_win = PW_NULL;
        IPw_EventSendFocusOut(win);

        IPW_TRACE_EXIT_V(WN3, d, TRUE);
        return(TRUE);
    }
    else
    {
        IPW_TRACE_EXIT_V(WN3, d, FALSE);
        return(FALSE);
    }
}

/**
 *  Maps window @p win.
 *  When window is mapped it is visible on display and can get focus.
 *  @param win pointer to window
 *  @see Pw_WindowUnmap
 *  @todo write Pw_MapWindowChildren function
 */ 
void 
Pw_WindowMap(Pw_Window* win)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE2M(WN2, p, win, d, win->mapped);

    /* If window is mapped we have nothing to do */
    if (win->mapped)
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    IPW_TRACE1(WN1, p, win->parent);

    /* If window parent isn't mapped return */
    if (PW_NULL != win->parent && !win->parent->mapped)
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Set the mapped flag to true */
    win->mapped = TRUE;

/*    _SetWindowChildrenMapped(win, TRUE); */
    
    /* Update clip region */
    _WindowUpdateClipRegion(win);

    /* Update clip region for windows below this window */
    _WindowUpdateClipRegionsBelow(win);

    /* Repaint this window */
    _WindowRepaint(win, PW_NULL, TRUE);

    /* Refresh the display */
    _DisplayRefresh(win->dpy, &win->clip_reg.bounds, PW_NULL);

    /* Send mapped event */
    IPw_EventSendMapped(win);

    IPW_CHECK_WINDOW(win);
    IPW_TRACE_EXIT(WN3);
}

/**
 *  Unmaps window @p win.
 *  When window is not mapped it is invisible and can't get focus.
 *  @param win pointer to window
 *  @see Pw_WindowMap
 */ 
void 
Pw_WindowUnmap(Pw_Window* win)
{
    Pw_RegionList clip_reg;
    Pw_Region*    reg;

    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE2M(WN2, p, win, d, win->mapped);

    /* If window is not mapped we have nothing to do */
    if (!win->mapped)
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    IPW_TRACE1(WN1, p, win->dpy->focus_win);

    /* 
     * If window in focus set window out of focus before unmapping it
     */
    if (Pw_WindowIsInFocus(win))
        Pw_WindowLeaveFocus(win);

    /*
     * Copy this window clip region - used for updateing windows
     * wich are obscured by this window
     */
    _WINDOW_COPY_CLIP_REGION(&clip_reg, &win->clip_reg);

    /* Set the mapped flag to false */
    win->mapped = FALSE;

    /* Set this window childern mapped flag to false */
    _WindowSetChildrensMappedFlag(win, FALSE);

    /* Set clip region of this window and its children to empty */
    _WINDOW_SET_CLIP_REGION_TO_EMPTY(win);        
    _WindowSetChildrenClipRegionToEmpty(win); 

    /* Update clip region for windows below this window */
    _WindowUpdateClipRegionsBelow(win);

    /* 
     * Repaint this window parent using this window clip region 
     * (if we use clip region bounds or the whole window region, 
     * we have to repaint all of its parent children)
     */
    if (PW_NULL != win->parent)
    {
        reg = clip_reg.regs;
        while (PW_NULL != reg)
        {
            IPW_TRACE_RECT(WN1, &reg->area); 

            _WindowRepaint(win->parent, &reg->area, FALSE);
            reg = reg->next;
        }
    }

    IPW_TRACE_RECT(WN1, &clip_reg.bounds); 

    /* 
     * Repaint windows below this window with this window 
     * clip region bounds (it is faster this way)
     */ 
    _WindowRepaintBelow(win, &clip_reg.bounds);

    /* Refresh the display */
    _DisplayRefresh(win->dpy, &clip_reg.bounds, PW_NULL);

    /* Send the unmapped event */
    IPw_EventSendUnmapped(win);

    /* Free window old clip region */
    _WINDOW_FREE_CLIP_REGION(&clip_reg);

    IPW_CHECK_WINDOW(win);
    IPW_TRACE_EXIT(WN3);
}

/**
 *  Repaints window @p win.
 *  @param win pointer to window
 *  @see Pw_WindowRepaintPart
 */ 
void 
Pw_WindowRepaint(Pw_Window* win)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE2(WN2, p, win, win->clip_reg.regs);
    IPW_TRACE2(WN1, d, win->clip_reg.regs_num, win->mapped);

    if (_WINDOW_IS_CLIP_REGION_NON_EMPTY(win) && win->mapped)
    {
        _WindowRepaint(win, PW_NULL, TRUE); 
        _DisplayRefresh(win->dpy, &win->clip_reg.bounds, PW_NULL);
    }
    IPW_TRACE_EXIT(WN3);
} 

/**
 *  Repaints part of window @p win.
 *  @param win  pointer to window
 *  @param clip pointer to clip rectangle
 *  @see Pw_WindowRepaint
 */ 
void 
Pw_WindowRepaintPart(Pw_Window* win, Pw_Rectangle* clip)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE3(WN2, p, win, clip, win->clip_reg.regs);
    IPW_TRACE2(WN1, d, win->clip_reg.regs_num, win->mapped);
    
    if (_WINDOW_IS_CLIP_REGION_NON_EMPTY(win) && win->mapped)
    {
        _WindowRepaint(win, clip, TRUE); 
        _DisplayRefresh(win->dpy, clip, PW_NULL);
    }
    IPW_TRACE_EXIT(WN3);
}

/**
 *  Sets the listener of xinput events to window @win.
 *  @param win pointer to window
 *  @param num xinput num 
 */  
void 
Pw_WindowSetXInputListener(Pw_Window* win, Pw_uInt num)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE2M(WN2, p, win, d, num);
 
    IPW_FAIL(num < PW_XINPUTS_MAX_NUM, ("XInput out of range (%d)!", num));
	
    IPW_TRACE1(WN1, p, win->dpy->xinput_win[num]);

	win->dpy->xinput_win[num] = win;

    IPW_TRACE_EXIT(WN3);
} 

/**
 *  Unsets the window @p win as listener of xinput events.
 *  @param win pointer to window
 *  @param num xinput num 
 */  
void 
Pw_WindowUnsetXInputListener(Pw_Window* win, Pw_uInt num)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE2M(WN2, p, win, d, num);
 
    IPW_FAIL(num < PW_XINPUTS_MAX_NUM, ("XInput out of range (%d)!", num));

    IPW_TRACE1(WN1, p, win->dpy->xinput_win[num]);
    IPW_CHECK_COND(win->dpy->xinput_win[num] == win);

	win->dpy->xinput_win[num] = PW_NULL;

    IPW_TRACE_EXIT(WN3);
}

/**
 *  Sets the shape of window @p win.
 *  @param win pointer to window
 *  @param x   window top left corner new x coord
 *  @param y   window top left corner new y coord
 *  @param w   window new width
 *  @param h   window new height 
 *  @see Pw_WindowSetPosition
 *  @see Pw_WindowSetSize
 */ 
void 
Pw_WindowSetShape(Pw_Window* win, 
                  Pw_Coord   x,
                  Pw_Coord   y,
                  Pw_Coord   w,
                  Pw_Coord   h)
{
    Pw_Coord      xoff, yoff; /* Move x offset and y offset */
    Pw_Window*    cld_win;    /* Child window */
    Pw_RegionList clip_old;   /* Old clip region */
    Pw_Region*    reg;

    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE1(WN2, p, win);
    IPW_TRACE4(WN1, d, x, y, w, h);
    IPW_TRACE_RECT(WN1, &win->area);

    /* Calculate move offset */
    xoff = x - win->area.x;
    yoff = y - win->area.y;

    IPW_TRACE2(WN1, d, xoff, yoff);

    /* Reshape window */
    win->area.x = x;
    win->area.y = y;
    win->area.w = w;
    win->area.h = h;

    /* Move children */
    cld_win = win->children;
    while (PW_NULL != cld_win)
    {
        IPW_TRACE1(WN1, p, cld_win);

        cld_win->area.x += xoff;
        cld_win->area.y += yoff;
        cld_win = cld_win->next;
    }

    IPW_TRACE1(WN1, d, win->mapped);

    /* If window not mapped we are done */
    if (!win->mapped)
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Copy window clip region */
    _WINDOW_COPY_CLIP_REGION(&clip_old, &win->clip_reg);

    /* Update this window clip region */
    _WindowUpdateClipRegion(win);

    /* Update clip regions for windows below this window */
    _WindowUpdateClipRegionsBelow(win);

    /* 
     * Repaint parent window using the old clip region of this window
     * (if we use the bounds of the old clip region or the old whole
     * window area we have to repaint all of this window parent children)
     */
    if (PW_NULL != win->parent)
    {
        reg = clip_old.regs;
        while (PW_NULL != reg)
        {
            IPW_TRACE_RECT(WN1, &reg->area); 

            _WindowRepaint(win->parent, &reg->area, FALSE);    
            reg = reg->next;
        }
    }

    IPW_TRACE_RECT(WN1, &clip_old.bounds); 

    /* 
     * Repaint windows below this window using this window old 
     * clip region bounds
     */
    _WindowRepaintBelow(win, &clip_old.bounds);

    /* Repaint this window */
    _WindowRepaint(win, PW_NULL, TRUE);

    /* Refresh the display */
    _DisplayRefresh(win->dpy, &clip_old.bounds, &win->clip_reg.bounds);

    /* Send window reshaped event */
    IPw_EventSendReshaped(win);

    /* Free window old clip region */
    _WINDOW_FREE_CLIP_REGION(&clip_old);

    IPW_CHECK_WINDOW(win);
    IPW_TRACE_EXIT(WN3);
}

/**
 *  Raise window @p win by one.
 *  Brings window win one window closer to the top.
 *  @param win pointer to window
 */ 
void 
Pw_WindowRaiseOne(Pw_Window* win)
{
    Pw_Window* awin;

    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE3(WN2, p, win, win->prev, win->parent);

    /* Get window above */
    awin = win->prev;      

    /* If window on top do nothing */
    if (PW_NULL == awin)  
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Switch windows */
    if (PW_NULL != awin->prev)
        awin->prev->next = win;

    if (PW_NULL != win->next)
        win->next->prev = awin;

    awin->next = win->next;
    win->next  = awin;

    win->prev  = awin->prev;
    awin->prev = win;

    /* If window on top correct parent's first child ptr */
    if (PW_NULL == win->prev && PW_NULL != win->parent)
        win->parent->children = win;

    IPW_TRACE1(WN1, p, win->parent->children);              
    IPW_TRACE2(WN1, d, win->mapped, awin->mapped);

    /* If one of the windows switched is not mapped return */
    if (!win->mapped || !awin->mapped)
    {
        IPW_CHECK_WINDOW(win);
        IPW_CHECK_WINDOW(awin);
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Correct windows clip regions */
    _WindowUpdateClipRegion(win);
    _WindowUpdateClipRegion(awin);

    /* Repaint window */
    _WindowRepaint(win, PW_NULL, TRUE);

    /* Refresh the display */
    _DisplayRefresh(win->dpy, &win->clip_reg.bounds, PW_NULL);

    IPW_CHECK_WINDOW(win);
    IPW_CHECK_WINDOW(awin);
 
    IPW_TRACE_EXIT(WN3);
} 

/**
 *  Lower window @p win by one.
 *  Brings window win one window closer to the bottom.
 *  @param win pointer to window
 */ 
void 
Pw_WindowLowerOne(Pw_Window* win)
{
    Pw_Window* bwin;

    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR(win);
    IPW_CHECK_WINDOW(win);
    IPW_TRACE3(WN2, p, win, win->next, win->parent);

    /* Get window below */
    bwin = win->next;      

    /* If window on bottom do nothing */
    if (PW_NULL == bwin)  
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Switch windows */
    if (PW_NULL != win->prev)
        win->prev->next = bwin;

    if (PW_NULL != bwin->next)
        bwin->next->prev = win;

    win->next  = bwin->next;
    bwin->next = win;     

    bwin->prev = win->prev; 
    win->prev  = bwin; 

    /* If window on top correct parent's first child ptr */
    if (PW_NULL == bwin->prev && PW_NULL != bwin->parent)
        bwin->parent->children = bwin;
                                        
    IPW_TRACE1(WN1, p, win->parent->children);              
    IPW_TRACE2(WN1, d, win->mapped, bwin->mapped);

    /* If one of the windows switched is not mapped return */
    if (!win->mapped || !bwin->mapped)
    {
        IPW_CHECK_WINDOW(win);
        IPW_CHECK_WINDOW(bwin);
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Correct windows clip regions */
    _WindowUpdateClipRegion(win);
    _WindowUpdateClipRegion(bwin);

    /* Repaint window above */
    _WindowRepaint(bwin, PW_NULL, TRUE);

    /* Refresh the display */
    _DisplayRefresh(bwin->dpy, &bwin->clip_reg.bounds, PW_NULL);

    IPW_CHECK_WINDOW(win);
    IPW_CHECK_WINDOW(bwin);
    IPW_TRACE_EXIT(WN3);
}

/**
 *  Places window @p win just above window @p rwin.
 *  @param win pointer to window
 *  @param rwin pointer to window
 */ 
void 
Pw_WindowPlaceAbove(Pw_Window* win, Pw_Window* rwin)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR2(win, rwin);
    IPW_CHECK_WINDOW(win);
    IPW_CHECK_WINDOW(rwin);
    IPW_TRACE4(WN2, p, win, win->prev, win->next, win->parent);
    IPW_TRACE4(WN2, p, rwin, rwin->prev, rwin->next, rwin->parent);
    IPW_CHECK_COND(win != rwin);
    IPW_CHECK_COND(win->parent == rwin->parent);

    if (win->parent != rwin->parent || win == rwin)
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Take window win out of the window list */

    if (PW_NULL == win->prev) 
    {
        /* Window win is at the top of the list */
        win->parent->children = win->next;
        if (PW_NULL != win->next)
            win->next->prev = PW_NULL;
    }
    else
    {
        /* Window win is somewhere in the list */
        win->prev->next = win->next;
        if (PW_NULL != win->next)
            win->next->prev = win->prev;
    }

    /* Now insert window win above window rwin */

    if (PW_NULL == rwin->prev)
    {
        /* Window rwin is at the top of the list */
        win->parent->children = win;
        win->prev = PW_NULL;
        win->next = rwin;
        rwin->prev = win;    
    }
    else
    {
        /* Window rwin is somewhere in the list */
        win->prev = rwin->prev;
        win->next = rwin;
        rwin->prev->next = win;
        rwin->prev = win;
    }

    IPW_CHECK_WINDOW(win);
    IPW_CHECK_WINDOW(rwin);

    /* If window win is not mapped return */
    if (!win->mapped)
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Correct windows clip regions */
    _WindowUpdateClipRegion(win->parent->children);
    _WindowUpdateClipRegionsBelow(win->parent->children);

    /* Repaint windows */
    _WindowRepaint(win->parent, &win->clip_reg.bounds, TRUE);

    /* Refresh the display */
    _DisplayRefresh(win->dpy, &win->clip_reg.bounds, PW_NULL);

    IPW_TRACE_EXIT(WN3);
}

/**
 *  Places window @p win just below window @p rwin.
 *  @param win pointer to window
 *  @param rwin pointer to window
 */ 
void 
Pw_WindowPlaceBelow(Pw_Window* win, Pw_Window* rwin)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR2(win, rwin);
    IPW_CHECK_WINDOW(win);
    IPW_CHECK_WINDOW(rwin);
    IPW_TRACE4(WN2, p, win, win->prev, win->next, win->parent);
    IPW_TRACE4(WN2, p, rwin, rwin->prev, rwin->next, rwin->parent);
    IPW_CHECK_COND(win != rwin);
    IPW_CHECK_COND(win->parent == rwin->parent);

    if (win->parent != rwin->parent || win == rwin)
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Take window win out of the window list */

    if (PW_NULL == win->prev) 
    {
        /* Window win is at the top of the list */
        win->parent->children = win->next;
        if (PW_NULL != win->next)
            win->next->prev = PW_NULL;
    }
    else
    {
        /* Window win is somewhere in the list */
        win->prev->next = win->next;
        if (PW_NULL != win->next)
            win->next->prev = win->prev;
    }

    /* Now insert window win below window rwin */

    if (PW_NULL == rwin->next)
    {
        /* Window rwin is at the bottom of the list */
        win->prev = rwin;
        win->next = PW_NULL;
        rwin->next = win;
    }
    else
    {
        /* Window rwin is somewhere in the list */
        win->prev = rwin;
        win->next = rwin->next;
        rwin->next->prev = win;
        rwin->next = win;
    }

    IPW_CHECK_WINDOW(win);
    IPW_CHECK_WINDOW(rwin);

    /* If window win is not mapped return */
    if (!win->mapped)
    {
        IPW_TRACE_EXIT(WN3);
        return;
    }

    /* Correct windows clip regions */
    _WindowUpdateClipRegion(win->parent->children);
    _WindowUpdateClipRegionsBelow(win->parent->children);

    /* Repaint windows */
    _WindowRepaint(win->parent, &win->clip_reg.bounds, TRUE);

    /* Refresh the display */
    _DisplayRefresh(win->dpy, &win->clip_reg.bounds, PW_NULL);

    IPW_TRACE_EXIT(WN3);
}

/**
 *  Places window @p win on top.
 *  @param win pointer to window
 */ 
void 
Pw_WindowPlaceOnTop(Pw_Window* win)
{
    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR2(win, win->parent);

    if (win != win->parent->children)
        Pw_WindowPlaceAbove(win, win->parent->children);

    IPW_TRACE_EXIT(WN3);
}

/**
 *  Places window @p win on bottom.
 *  @param win pointer to window
 */ 
void 
Pw_WindowPlaceOnBottom(Pw_Window* win)
{
    Pw_Window* bwin;

    IPW_TRACE_ENTER(WN3);
    IPW_CHECK_PTR2(win, win->parent);

    bwin = win;

    while (PW_NULL != bwin->next)
        bwin = bwin->next;

    if (win != bwin)
        Pw_WindowPlaceBelow(win, bwin);

    IPW_TRACE_EXIT(WN3);
}

/**
 *  Gets the free client data id to use when
 *  setting/retrieving client data to/from windows.
 *  @return new client data id
 *  @see Pw_WindowGetClientData
 *  @see Pw_WindowGetClientDataId
 *  @see Pw_WindowSetClientData
 */ 
Pw_Int 
Pw_WindowGetNewClientDataId(void)
{
    IPW_TRACE_ENTER(WN3);
    IPW_TRACE_EXIT_V(WN3, d, _cdata_next_id);
    return(_cdata_next_id++);
}

/*---------------------------------------------------------------------------
// end of pw_window.c
//--------------------------------------------------------------------------- */
