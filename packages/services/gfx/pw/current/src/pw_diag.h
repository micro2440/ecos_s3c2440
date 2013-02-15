/*===========================================================================
//
//        pw_diag.h
//
//        PW graphic library diag defs 
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

#ifndef PW_DIAG_H
#define PW_DIAG_H

/* -------------------------------------------------------------------------- */

#include <i_pwin.h>

/* -------------------------------------------------------------------------- */

/*
#define IPW_USE_TRACE   1 
#define IPW_USE_CHECKS  1 
#define IPW_USE_ASSERTS 1 
#define IPW_USE_ERRFN   1
*/

/* -------------------------------------------------------------------------- */

/* Initialization code tracing */
#define IPW_TRC_IN1(_code_) _code_

/* Region allocation code tracing */
#define IPW_TRC_RA1(_code_) _code_
#define IPW_TRC_RA2(_code_) _code_

/* Rectangle geometry and manipulation code tracing */
#define IPW_TRC_RE1(_code_) _code_
#define IPW_TRC_RE2(_code_) _code_
#define IPW_TRC_RE3(_code_) _code_

/* Drawing code tracing */
#define IPW_TRC_DR1(_code_) _code_
#define IPW_TRC_DR2(_code_) _code_
#define IPW_TRC_DR3(_code_) _code_

/* Bitmap code tracing */
#define IPW_TRC_BM1(_code_) _code_
#define IPW_TRC_BM2(_code_) _code_
#define IPW_TRC_BM3(_code_) _code_

/* Event code tracing */
#define IPW_TRC_EV1(_code_) _code_
#define IPW_TRC_EV2(_code_) _code_

/* Window code tracing */
#define IPW_TRC_WN1(_code_) _code_
#define IPW_TRC_WN2(_code_) _code_
#define IPW_TRC_WN3(_code_) _code_

/* Display code tracing */
#define IPW_TRC_DP1(_code_) _code_
#define IPW_TRC_DP2(_code_) _code_

/* Timeout code tracing */
#define IPW_TRC_TM1(_code_) _code_
#define IPW_TRC_TM2(_code_) _code_

/* -------------------------------------------------------------------------- */

#define IPW_CHK_REGION_ALLOC(_code_)     _code_
#define IPW_CHK_REGION_FREE_LIST(_code_) _code_
#define IPW_CHK_BITMAP_STRUCT(_code_)    _code_
#define IPW_CHK_GC_STRUCT(_code_)        _code_
#define IPW_CHK_WINDOW_STRUCT(_code_)    _code_
#define IPW_CHK_REGION_LIST(_code_)      _code_
#define IPW_CHK_REGION_MAX_USAGE(_code_) _code_

/* -------------------------------------------------------------------------- */

#ifdef IPW_USE_ERRFN

#define IPW_NOTE(_args_)                                  \
    IPW_MACRO_START                                       \
        IPw_NoteStart(__FUNCTION__, __FILE__, __LINE__);  \
        IPw_Printf _args_;                                \
        IPw_NoteEnd();                                    \
    IPW_MACRO_END

#define IPW_WARN(_cond_, _args_)                              \
    IPW_MACRO_START                                           \
        if (!(_cond_))                                        \
        {                                                     \
            IPw_WarnStart(__FUNCTION__, __FILE__, __LINE__);  \
            IPw_Printf _args_;                                \
            IPw_WarnEnd();                                    \
        }                                                     \
    IPW_MACRO_END

#define IPW_FAIL(_cond_, _args_)                             \
    IPW_MACRO_START                                          \
        if (!(_cond_))                                       \
        {                                                    \
            IPw_FailStart(__FUNCTION__, __FILE__, __LINE__); \
            IPw_Printf _args_;                               \
            IPw_FailEnd();                                   \
        }                                                    \
    IPW_MACRO_END

#else

#define IPW_NOTE(_args_)         IPW_EMPTY_STATEMENT
#define IPW_WARN(_cond_, _args_) IPW_EMPTY_STATEMENT  
#define IPW_FAIL(_cond_, _args_) IPW_EMPTY_STATEMENT    

#endif

/* -------------------------------------------------------------------------- */

#ifdef IPW_USE_ASSERTS

#define IPW_ASSERT(_cond_, _args_)                            \
    IPW_MACRO_START                                           \
        if (!(_cond_))                                        \
        {                                                     \
            IPw_AssertStart(__FUNCTION__, __FILE__, __LINE__);\
            IPw_Printf _args_;                                \
            IPw_AssertEnd();                                  \
        }                                                     \
    IPW_MACRO_END

#else /* IPW_USE_ASSERTS */

#define IPW_ASSERT(_cond_, _args_) IPW_EMPTY_STATEMENT

#endif /* not IPW_USE_ASSERTS */

#define IPW_CHECK_PTR(_ptr_) \
    IPW_ASSERT(PW_NULL != (_ptr_), (# _ptr_ " == NULL"))

#define IPW_CHECK_PTR2(_ptr1_, _ptr2_)                          \
    IPW_MACRO_START                                             \
        IPW_ASSERT(PW_NULL != (_ptr1_), (# _ptr1_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr2_), (# _ptr2_ " == NULL")); \
    IPW_MACRO_END           

#define IPW_CHECK_PTR3(_ptr1_, _ptr2_, _ptr3_)                  \
    IPW_MACRO_START                                             \
        IPW_ASSERT(PW_NULL != (_ptr1_), (# _ptr1_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr2_), (# _ptr2_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr3_), (# _ptr3_ " == NULL")); \
    IPW_MACRO_END           

#define IPW_CHECK_PTR4(_ptr1_, _ptr2_, _ptr3_, _ptr4_)          \
    IPW_MACRO_START                                             \
        IPW_ASSERT(PW_NULL != (_ptr1_), (# _ptr1_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr2_), (# _ptr2_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr3_), (# _ptr3_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr4_), (# _ptr4_ " == NULL")); \
    IPW_MACRO_END           

#define IPW_CHECK_PTR5(_ptr1_, _ptr2_, _ptr3_, _ptr4_, _ptr5_)  \
    IPW_MACRO_START                                             \
        IPW_ASSERT(PW_NULL != (_ptr1_), (# _ptr1_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr2_), (# _ptr2_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr3_), (# _ptr3_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr4_), (# _ptr4_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr5_), (# _ptr5_ " == NULL")); \
    IPW_MACRO_END           

#define IPW_CHECK_PTR6(_ptr1_, _ptr2_, _ptr3_,                  \
                       _ptr4_, _ptr5_, _ptr6_)                  \
    IPW_MACRO_START                                             \
        IPW_ASSERT(PW_NULL != (_ptr1_), (# _ptr1_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr2_), (# _ptr2_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr3_), (# _ptr3_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr4_), (# _ptr4_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr5_), (# _ptr5_ " == NULL")); \
        IPW_ASSERT(PW_NULL != (_ptr6_), (# _ptr6_ " == NULL")); \
    IPW_MACRO_END           

#define IPW_CHECK_COND(_cond_) \
    IPW_ASSERT(_cond_, (# _cond_ " is FALSE!"))

/* -------------------------------------------------------------------------- */

#ifdef IPW_USE_TRACE

#define IPW_TRACE_F(_args_)                               \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf _args_;                                \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE1_F(_t_, _a_)                            \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _t_ " ", _a_);            \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE2_F(_t_, _a_, _b_)                       \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _t_ " "                   \
                   # _b_ "=%" # _t_ " ",                  \
                   _a_, _b_);                             \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE3_F(_t_, _a_, _b_, _c_)                  \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _t_ " "                   \
                   # _b_ "=%" # _t_ " "                   \
                   # _c_ "=%" # _t_ " ",                  \
                    _a_, _b_, _c_);                       \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE4_F(_t_, _a_, _b_, _c_, _d_)             \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _t_ " "                   \
                   # _b_ "=%" # _t_ " "                   \
                   # _c_ "=%" # _t_ " "                   \
                   # _d_ "=%" # _t_ " ",                  \
                   _a_, _b_, _c_, _d_);                   \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE5_F(_t_, _a_, _b_, _c_, _d_, _e_)        \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _t_ " "                   \
                   # _b_ "=%" # _t_ " "                   \
                   # _c_ "=%" # _t_ " "                   \
                   # _d_ "=%" # _t_ " "                   \
                   # _e_ "=%" # _t_ " ",                  \
                   _a_, _b_, _c_, _d_, _e_);              \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE6_F(_t_, _a_, _b_, _c_, _d_, _e_, _f_)   \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _t_ " "                   \
                   # _b_ "=%" # _t_ " "                   \
                   # _c_ "=%" # _t_ " "                   \
                   # _d_ "=%" # _t_ " "                   \
                   # _e_ "=%" # _t_ " "                   \
                   # _f_ "=%" # _t_ " ",                  \
                   _a_, _b_, _c_, _d_, _e_, _f_);         \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE1M_F(_ta_, _a_)                          \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _ta_ " ", _a_);           \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE2M_F(_ta_, _a_, _tb_, _b_)               \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _ta_ " "                  \
                   # _b_ "=%" # _tb_ " ",                 \
                   _a_, _b_);                             \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE3M_F(_ta_, _a_, _tb_, _b_, _tc_, _c_)    \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _ta_ " "                  \
                   # _b_ "=%" # _tb_ " "                  \
                   # _c_ "=%" # _tc_ " ",                 \
                    _a_, _b_, _c_);                       \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE4M_F(_ta_, _a_, _tb_, _b_,               \
                      _tc_, _c_, _td_, _d_)               \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _ta_ " "                  \
                   # _b_ "=%" # _tb_ " "                  \
                   # _c_ "=%" # _tc_ " "                  \
                   # _d_ "=%" # _td_ " ",                 \
                   _a_, _b_, _c_, _d_);                   \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE5M_F(_ta_, _a_, _tb_, _b_,               \
                      _tc_, _c_, _td_, _d_, _te_, _e_)    \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _ta_ " "                  \
                   # _b_ "=%" # _tb_ " "                  \
                   # _c_ "=%" # _tc_ " "                  \
                   # _d_ "=%" # _td_ " "                  \
                   # _e_ "=%" # _te_ " ",                 \
                   _a_, _b_, _c_, _d_, _e_);              \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END

#define IPW_TRACE6M_F(_ta_, _a_, _tb_, _b_, _tc_, _c_,    \
                      _td_, _d_, _te_, _e_, _tf_, _f_)    \
    IPW_MACRO_START                                       \
        IPw_TraceStart(__FUNCTION__, __FILE__, __LINE__); \
        IPw_Printf(# _a_ "=%" # _ta_ " "                  \
                   # _b_ "=%" # _tb_ " "                  \
                   # _c_ "=%" # _tc_ " "                  \
                   # _d_ "=%" # _td_ " "                  \
                   # _e_ "=%" # _te_ " "                  \
                   # _f_ "=%" # _tf_ " ",                 \
                   _a_, _b_, _c_, _d_, _e_, _f_);         \
        IPw_TraceEnd();                                   \
    IPW_MACRO_END


#define IPW_TRACE(_DTYPE_, _args_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE_F(_args_))   

#define IPW_TRACE1(_DTYPE_, _t_, _a_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE1_F(_t_, _a_))   

#define IPW_TRACE2(_DTYPE_, _t_, _a_, _b_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE2_F(_t_, _a_, _b_))   

#define IPW_TRACE3(_DTYPE_, _t_, _a_, _b_, _c_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE3_F(_t_, _a_, _b_, _c_))   

#define IPW_TRACE4(_DTYPE_, _t_, _a_, _b_, _c_, _d_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE4_F(_t_, _a_, _b_, _c_, _d_))   

#define IPW_TRACE5(_DTYPE_, _t_, _a_, _b_, _c_, _d_, _e_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE5_F(_t_, _a_, _b_, _c_, _d_, _e_))   

#define IPW_TRACE6(_DTYPE_, _t_, _a_, _b_, _c_, _d_, _e_, _f_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE6_F(_t_, _a_, _b_, _c_, _d_, _e_, _f_))   

#define IPW_TRACE1M(_DTYPE_, _ta_, _a_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE1M_F(_ta_, _a_))   

#define IPW_TRACE2M(_DTYPE_, _ta_, _a_, _tb_, _b_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE2M_F(_ta_, _a_, _tb_, _b_))   

#define IPW_TRACE3M(_DTYPE_, _ta_, _a_, _tb_, _b_, _tc_, _c_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE3M_F(_ta_, _a_, _tb_, _b_, _tc_, _c_))   

#define IPW_TRACE4M(_DTYPE_, _ta_, _a_, _tb_, _b_, _tc_, _c_, _td_, _d_) \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE4M_F(_ta_, _a_, _tb_, _b_,              \
                                      _tc_, _c_, _td_, _d_))             \

#define IPW_TRACE5M(_DTYPE_, _ta_, _a_, _tb_, _b_, _tc_, _c_,          \
                             _td_, _d_, _te_, _e_)                     \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE5M_F(_ta_, _a_, _tb_, _b_, _tc_, _c_, \
                                     _td_, _d_, _te_, _e_))

#define IPW_TRACE6M(_DTYPE_, _ta_, _a_, _tb_, _b_, _tc_, _c_,           \
                             _td_, _d_, _te_, _e_, _tf_, _f_)           \
    IPW_TRC_ ## _DTYPE_(IPW_TRACE6M_F(_ta_, _a_, _tb_, _b_, _tc_, _c_,  \
                                      _td_, _d_, _te_, _e_, _tf_, _f_))   

#define IPW_TRACE_ENTER(_DTYPE_) \
    IPW_TRACE(_DTYPE_, ("ENTER"))

#define IPW_TRACE_EXIT(_DTYPE_) \
    IPW_TRACE(_DTYPE_, ("EXIT"))

#define IPW_TRACE_EXIT_V(_DTYPE_, _t_, _v_) \
    IPW_TRACE(_DTYPE_, ("EXIT (%" # _t_ ")", _v_)); 

#define IPW_TRACE_RECT(_DTYPE_, _r_)                                    \
    IPW_MACRO_START                                                     \
        if (PW_NULL != _r_)                                             \
        {                                                               \
            IPW_TRACE(_DTYPE_, ("rect (%p) " # _r_ "=(%d, %d, %d, %d)", \
                                (_r_), (_r_)->x, (_r_)->y,              \
                                (_r_)->w, (_r_)->h));                   \
        }                                                               \
        else                                                            \
        {                                                               \
            IPW_TRACE(_DTYPE_, ("rect (%p) " # _r_ "=()", (_r_)));      \
        }                                                               \
    IPW_MACRO_END

#define IPW_TRACE_RECT2(_DTYPE_, _r1_, _r2_) \
    IPW_MACRO_START                          \
        IPW_TRACE_RECT(_DTYPE_, _r1_);       \
        IPW_TRACE_RECT(_DTYPE_, _r2_);       \
    IPW_MACRO_END

#define IPW_TRACE_RECT3(_DTYPE_, _r1_, _r2_, _r3_) \
    IPW_MACRO_START                                \
        IPW_TRACE_RECT(_DTYPE_, _r1_);             \
        IPW_TRACE_RECT(_DTYPE_, _r2_);             \
        IPW_TRACE_RECT(_DTYPE_, _r3_);             \
    IPW_MACRO_END

#define IPW_TRACE_RECT4(_DTYPE_, _r1_, _r2_, _r3_, _r4_) \
    IPW_MACRO_START                                      \
        IPW_TRACE_RECT(_DTYPE_, _r1_);                   \
        IPW_TRACE_RECT(_DTYPE_, _r2_);                   \
        IPW_TRACE_RECT(_DTYPE_, _r3_);                   \
        IPW_TRACE_RECT(_DTYPE_, _r4_);                   \
    IPW_MACRO_END

#define IPW_TRACE_REGION(_DTYPE_, _r_)                                  \
    IPW_MACRO_START                                                     \
        if (PW_NULL != _r_)                                             \
        {                                                               \
            IPW_TRACE(_DTYPE_, ("reg (%p) " # _r_ " "                   \
                                "next=%p area=(%d, %d, %d, %d)",        \
                                (_r_), (_r_)->next, (_r_)->area.x,      \
                                (_r_)->area.y, (_r_)->area.w));         \
        }                                                               \
        else                                                            \
        {                                                               \
            IPW_TRACE(_DTYPE_, ("rect (%p) " # _r_ "=()", (_r_)));      \
        }                                                               \
    IPW_MACRO_END

#define IPW_TRACE_REGION2(_DTYPE_, _r1_, _r2_) \
    IPW_MACRO_START                            \
        IPW_TRACE_REGION(_DTYPE_, _r1_);       \
        IPW_TRACE_REGION(_DTYPE_, _r2_);       \
    IPW_MACRO_END

#define IPW_TRACE_REGION3(_DTYPE_, _r1_, _r2_, _r3_) \
    IPW_MACRO_START                                  \
        IPW_TRACE_REGION(_DTYPE_, _r1_);             \
        IPW_TRACE_REGION(_DTYPE_, _r2_);             \
        IPW_TRACE_REGION(_DTYPE_, _r3_);             \
    IPW_MACRO_END

#define IPW_TRACE_REGION4(_DTYPE_, _r1_, _r2_, _r3_, _r4_) \
    IPW_MACRO_START                                        \
        IPW_TRACE_REGION(_DTYPE_, _r1_);                   \
        IPW_TRACE_REGION(_DTYPE_, _r2_);                   \
        IPW_TRACE_REGION(_DTYPE_, _r3_);                   \
        IPW_TRACE_REGION(_DTYPE_, _r4_);                   \
    IPW_MACRO_END

#define IPW_TRACE_RLIST(_DTYPE_, _rlist_)                                \
    IPW_MACRO_START                                                      \
        if (PW_NULL != _rlist_)                                          \
        {                                                                \
            IPW_TRACE(_DTYPE_, ("rlist (%p) " # _rlist_ " "              \
                                "regs=%p num=%d bnds=(%d, %d, %d, %d)",  \
                                (_rlist_), (_rlist_)->regs,              \
                                (_rlist_)->regs_num,                     \
                                (_rlist_)->bounds.x,                     \
                                (_rlist_)->bounds.y,                     \
                                (_rlist_)->bounds.w,                     \
                                (_rlist_)->bounds.h));                   \
        }                                                                \
        else                                                             \
        {                                                                \
            IPW_TRACE(_DTYPE_, ("rlist (%p) " # _rlist_ "=()",           \
                                (_rlist_)));                             \
        }                                                                \
    IPW_MACRO_END

#define IPW_TRACE_RLIST2(_DTYPE_, _r1_, _r2_) \
    IPW_MACRO_START                           \
        IPW_TRACE_RLIST(_DTYPE_, _r1_);       \
        IPW_TRACE_RLIST(_DTYPE_, _r2_);       \
    IPW_MACRO_END

#define IPW_TRACE_RLIST3(_DTYPE_, _r1_, _r2_, _r3_) \
    IPW_MACRO_START                                 \
        IPW_TRACE_RLIST(_DTYPE_, _r1_);             \
        IPW_TRACE_RLIST(_DTYPE_, _r2_);             \
        IPW_TRACE_RLIST(_DTYPE_, _r3_);             \
    IPW_MACRO_END

#define IPW_TRACE_RLIST4(_DTYPE_, _r1_, _r2_, _r3_, _r4_) \
    IPW_MACRO_START                                       \
        IPW_TRACE_RLIST(_DTYPE_, _r1_);                   \
        IPW_TRACE_RLIST(_DTYPE_, _r2_);                   \
        IPW_TRACE_RLIST(_DTYPE_, _r3_);                   \
        IPW_TRACE_RLIST(_DTYPE_, _r4_);                   \
    IPW_MACRO_END

#define IPW_TRACE_EN(_DTYPE_) \
    (0 IPW_TRC_ ## _DTYPE_(== 0))

#else /* IPW_USE_TRACE */

#define IPW_TRACE(_DTYPE_, _args_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE1(_DTYPE_, _t_, _a_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE2(_DTYPE_, _t_, _a_, _b_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE3(_DTYPE_, _t_, _a_, _b_, _c_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE4(_DTYPE_, _t_, _a_, _b_, _c_, _d_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE5(_DTYPE_, _t_, _a_, _b_, _c_, _d_, _e_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE6(_DTYPE_, _t_, _a_, _b_, _c_, _d_, _e_, _f_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE1M(_DTYPE_, _ta_, _a_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE2M(_DTYPE_, _ta_, _a_, _tb_, _b_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE3M(_DTYPE_, _ta_, _a_, _tb_, _b_, _tc_, _c_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE4M(_DTYPE_, _ta_, _a_, _tb_, _b_, _tc_, _c_, _td_, _d_) \
    IPW_EMPTY_STATEMENT
#define IPW_TRACE5M(_DTYPE_, _ta_, _a_, _tb_, _b_, _tc_, _c_, \
                             _td_, _d_, _te_, _e_) \
    IPW_EMPTY_STATEMENT          
#define IPW_TRACE6M(_DTYPE_, _ta_, _a_, _tb_, _b_, _tc_, _c_, \
                             _td_, _d_, _te_, _e_, _tf_, _f_) \
    IPW_EMPTY_STATEMENT                         
#define IPW_TRACE_ENTER(_DTYPE_)                           IPW_EMPTY_STATEMENT
#define IPW_TRACE_EXIT(_DTYPE_)                            IPW_EMPTY_STATEMENT
#define IPW_TRACE_EXIT_V(_DTYPE_, _t_, _v_)                IPW_EMPTY_STATEMENT
#define IPW_TRACE_RECT(_DTYPE_, _r_)                       IPW_EMPTY_STATEMENT
#define IPW_TRACE_RECT2(_DTYPE_, _r1_, _r2_)               IPW_EMPTY_STATEMENT  
#define IPW_TRACE_RECT3(_DTYPE_, _r1_, _r2_, _r3_)         IPW_EMPTY_STATEMENT
#define IPW_TRACE_RECT4(_DTYPE_, _r1_, _r2_, _r3_, _r4_)   IPW_EMPTY_STATEMENT  
#define IPW_TRACE_REGION(_DTYPE_, _r_)                     IPW_EMPTY_STATEMENT
#define IPW_TRACE_REGION2(_DTYPE_, _r1_, _r2_)             IPW_EMPTY_STATEMENT
#define IPW_TRACE_REGION3(_DTYPE_, _r1_, _r2_, _r3_)       IPW_EMPTY_STATEMENT
#define IPW_TRACE_REGION4(_DTYPE_, _r1_, _r2_, _r3_, _r4_) IPW_EMPTY_STATEMENT  
#define IPW_TRACE_RLIST(_DTYPE_, _r_)                      IPW_EMPTY_STATEMENT
#define IPW_TRACE_RLIST2(_DTYPE_, _r1_, _r2_)              IPW_EMPTY_STATEMENT 
#define IPW_TRACE_RLIST3(_DTYPE_, _r1_, _r2_, _r3_)        IPW_EMPTY_STATEMENT
#define IPW_TRACE_RLIST4(_DTYPE_, _r1_, _r2_, _r3_, _r4_)  IPW_EMPTY_STATEMENT  

#define IPW_TRACE_EN(_DTYPE_) 0

#endif /* not IPW_USE_TRACE */

/* -------------------------------------------------------------------------- */

#ifdef IPW_USE_CHECKS

#define IPW_CHECK(_CTYPE_, _code_) \
    IPW_CHK_ ## _CTYPE_(_code_)

#define IPW_CHECK_EN(_CTYPE_) \
    (0 IPW_CHK_ ## _CTYPE_(== 0))

#else /* IPW_USE_CHECKS */

#define IPW_CHECK(_CTYPE_, _code_) IPW_EMPTY_STATEMENT

#define IPW_CHECK_EN(_CTYPE_) 0

#endif /* not IPW_USE_CHECKS */

/* -------------------------------------------------------------------------- */

#if IPW_CHECK_EN(BITMAP_STRUCT)

Pw_Bool IPw_BitmapCheck(Pw_Bitmap* bitmap);

#define IPW_CHECK_BITMAP(_bitmap_)                     \
    IPW_MACRO_START                                    \
        IPW_ASSERT(IPw_BitmapCheck(_bitmap_),          \
            ("Bitmap (%p) check failed!", _bitmap_));  \
    IPW_MACRO_END

#else /* IPW_CHECK_EN(BITMAP_STRUCT) */

#define IPW_CHECK_BITMAP(_bitmap_) IPW_EMPTY_STATEMENT

#endif  /* not IPW_CHECK_EN(BITMAP_STRUCT) */


#if IPW_CHECK_EN(GC_STRUCT)

Pw_Bool IPw_GCCheck(Pw_GC* bitmap);

#define IPW_CHECK_GC(_gc_)                             \
    IPW_MACRO_START                                    \
        IPW_ASSERT(IPw_GCCheck(_gc_),                  \
            ("GC (%p) check failed!", _gc_));          \
    IPW_MACRO_END

#else /* IPW_CHECK_EN(GC_STRUCT) */

#define IPW_CHECK_GC(_gc_) IPW_EMPTY_STATEMENT

#endif /* not IPW_CHECK_EN(GC_STRUCT) */



#if IPW_CHECK_EN(WINDOW_STRUCT)

Pw_Bool IPw_WindowCheck(Pw_Window* window);

#define IPW_CHECK_WINDOW(_win_)                        \
    IPW_MACRO_START                                    \
        IPW_ASSERT(IPw_WindowCheck(_win_),             \
            ("Window (%p) check failed!", _win_));     \
    IPW_MACRO_END

#else /* IPW_CHECK_EN(WINDOW_STRUCT) */

#define IPW_CHECK_WINDOW(_win_) IPW_EMPTY_STATEMENT

#endif  /* not IPW_CHECK_EN(WINDOW_STRUCT) */



#if IPW_CHECK_EN(REGION_LIST)

Pw_Bool IPw_RegionListCheck(Pw_RegionList* rlist, Pw_Bool hard);

#define IPW_CHECK_RLIST(_rlist_)                          \
    IPW_MACRO_START                                       \
        IPW_ASSERT(IPw_RegionListCheck(_rlist_, FALSE),   \
            ("Region list (%p) check failed!", _rlist_)); \
    IPW_MACRO_END

#define IPW_CHECK_RLIST_HARD(_rlist_)                          \
    IPW_MACRO_START                                            \
        IPW_ASSERT(IPw_RegionListCheck(_rlist_, TRUE),         \
            ("Region list (%p) hard check failed!", _rlist_)); \
    IPW_MACRO_END

#else /* IPW_CHECK_EN(REGION_LIST) */

#define IPW_CHECK_RLIST(_rlist_)      IPW_EMPTY_STATEMENT

#define IPW_CHECK_RLIST_HARD(_rlist_) IPW_EMPTY_STATEMENT

#endif /* not IPW_CHECK_EN(REGION_LIST) */

/* -------------------------------------------------------------------------- */

void IPw_Printf(Pw_Char* format, ...);

void IPw_TraceStart(Pw_Char* func, Pw_Char* file, Pw_Int line); 

void IPw_TraceEnd(void); 

void IPw_AssertStart(Pw_Char* func, Pw_Char* file, Pw_Int line); 

void IPw_AssertEnd(void); 

void IPw_NoteStart(Pw_Char* func, Pw_Char* file, Pw_Int line); 

void IPw_NoteEnd(void); 

void IPw_WarnStart(Pw_Char* func, Pw_Char* file, Pw_Int line); 

void IPw_WarnEnd(void); 

void IPw_FailStart(Pw_Char* func, Pw_Char* file, Pw_Int line); 

void IPw_FailEnd(void); 

/* -------------------------------------------------------------------------- */

#endif /* not PW_DIAG_H */

/*---------------------------------------------------------------------------
// end of pw_diag.h 
//--------------------------------------------------------------------------- */
