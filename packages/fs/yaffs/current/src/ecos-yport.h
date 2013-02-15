#ifndef CYGONCE_FS_ECOS_YPORT_H
#define CYGONCE_FS_ECOS_YPORT_H
//=============================================================================
//
//      ecos-yport.h
//
//      eCos porting layer for YAFFS
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
// Date:        2009-05-05
//
//####DESCRIPTIONEND####
//=============================================================================

#include "ecos-yaffs.h"

#include "devextras.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/cyg_type.h>

#define YCHAR char
#define YUCHAR unsigned char
#define _Y(x) x
#define yaffs_strcat(a,b)    strcat(a,b)
#define yaffs_strcpy(a,b)    strcpy(a,b)
#define yaffs_strncpy(a,b,c) strncpy(a,b,c)
#define yaffs_strncmp(a,b,c) strncmp(a,b,c)
#define yaffs_strlen(s)      strlen(s)
#define yaffs_sprintf        sprintf
#define yaffs_toupper(a)     toupper(a)

#define YAFFS_PATH_DIVIDERS  "/"

#define Y_INLINE inline

#define YMALLOC(x) malloc(x)
#define YFREE(x)   free(x)
#define YMALLOC_ALT(x) malloc(x)
#define YFREE_ALT(x)   free(x)

#define YMALLOC_DMA(x) YMALLOC(x)

#ifdef CYGFUN_KERNEL_API_C
#include <cyg/kernel/kapi.h>
#define YYIELD()  do { cyg_thread_yield(); } while(0)
#else
#define YYIELD()  do {} while(0)
#endif


//#define YINFO(s) YPRINTF(( __FILE__ " %d %s\n",__LINE__,s))
//#define YALERT(s) YINFO(s)
#define YBUG() do { cyg_assert_fail(__PRETTY_FUNCTION__, __FILE__, __LINE__, "==>> yaffs bug detected"); } while(0)

#define TENDSTR "\n"
#define TSTR(x) x
#define TCONT(x) x
#ifdef CYGSEM_FS_YAFFS_TRACEFN
externC int CYGSEM_FS_YAFFS_TRACEFN (const char *fmt, ...) CYGBLD_ATTRIB_PRINTF_FORMAT(1,2);
#define TOUT(p) CYGSEM_FS_YAFFS_TRACEFN p
#else
static inline int _yaffs_trace_ignore (const char *fmt, ...) CYGBLD_ATTRIB_PRINTF_FORMAT(1,2);
static inline int _yaffs_trace_ignore (const char *fmt, ...) { return 0; }
#define TOUT(p) _yaffs_trace_ignore p
#endif


#define YAFFS_LOSTNFOUND_NAME       "lost+found"
#define YAFFS_LOSTNFOUND_PREFIX     "obj"
//#define YPRINTF(x) printf x

#ifdef CYGPKG_LIBC_TIME
#define Y_CURRENT_TIME time(NULL)
#else
#define Y_CURRENT_TIME ((time_t)-1)
#endif
#define Y_TIME_CONVERT(x) (x)

// eCos defines mode bits differently to POSIX.
#define YAFFS_ROOT_MODE             ( S_IRUSR | S_IWUSR | S_IXUSR |  \
                                      S_IRGRP |           S_IXGRP |  \
                                      S_IROTH |           S_IXOTH   )
#define YAFFS_LOSTNFOUND_MODE       YAFFS_ROOT_MODE

#define yaffs_SumCompare(x,y) ((x) == (y))
#define yaffs_strcmp(a,b) strcmp(a,b)

typedef off_t loff_t; /* This will break and should probably be removed 
                         if/when we add 64-bit file support generally. */

#define yaffs_qsort qsort /* provided by eCos stdlib */

#endif /* CYGONCE_FS_ECOS_YPORT_H */
