//==========================================================================
//
//      example.c
//
//      Demonstration of the synthetic target framebuffer capabilities
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2008 Free Software Foundation, Inc.                        
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
//==========================================================================
//###DESCRIPTIONBEGIN####
//
// Author(s):     bartv
// Date:          2008-10-06
//
//###DESCRIPTIONEND####
//========================================================================

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/io/framebuf.h>
#include <cyg/kernel/kapi.h>
#include <string.h>
#include <stdio.h>

#define BLACK        colours[0x00]
#define BLUE         colours[0x01]
#define GREEN        colours[0x02]
#define CYAN         colours[0x03]
#define RED          colours[0x04]
#define MAGENTA      colours[0x05]
#define BROWN        colours[0x06]
#define LIGHTGREY    colours[0x07]
#define DARKGREY     colours[0x08]
#define LIGHTBLUE    colours[0x09]
#define LIGHTGREEN   colours[0x0A]
#define LIGHTCYAN    colours[0x0B]
#define LIGHTRED     colours[0x0C]
#define LIGHTMAGENTA colours[0x0D]
#define YELLOW       colours[0x0E]
#define WHITE        colours[0x0F]

// ----------------------------------------------------------------------------
static void
fb2_thread(cyg_addrword_t arg)
{
#define FRAMEBUF fb0
#define WIDTH    CYG_FB_WIDTH(FRAMEBUF)
#define HEIGHT   CYG_FB_HEIGHT(FRAMEBUF)
    
    static cyg_ucount32 colours[16];
    cyg_ucount16 block_width;
    int i, j;
    int x = 0, y = 0;
    //CYG_FB_PIXEL0_VAR(FRAMEBUF);
    //CYG_FB_PIXEL1_VAR(FRAMEBUF);
    //cyg_fb_ioctl_viewport viewport;
    size_t len;

    CYG_FB_ON(FRAMEBUF);
	
    for (i = 0; i < 16; i++) {
        colours[i]  = CYG_FB_MAKE_COLOUR(FRAMEBUF,
                                         cyg_fb_palette_vga[i + i + i], cyg_fb_palette_vga[i + i + i + 1],cyg_fb_palette_vga[i + i + i + 2]);
    }
    // A white background
    CYG_FB_FILL_BLOCK(FRAMEBUF, 0, 0, WIDTH, HEIGHT, WHITE);
    // A black block in the middle, 25 pixels in.
    CYG_FB_FILL_BLOCK(FRAMEBUF, 32, 32, WIDTH - 64, HEIGHT - 64, BLACK);


    // Four diagonal lines in the corners. Red in the top left, blue in the top right,
    // green in the bottom left, and yellow in the bottom right.
    for (i = 0; i < 32; i++) {
        CYG_FB_WRITE_PIXEL(FRAMEBUF, i,               i,                RED);
        CYG_FB_WRITE_PIXEL(FRAMEBUF, (WIDTH - 1) - i, i,                BLUE);
        CYG_FB_WRITE_PIXEL(FRAMEBUF, i,               (HEIGHT - 1) - i, GREEN);
        CYG_FB_WRITE_PIXEL(FRAMEBUF, (WIDTH - 1) - i, (HEIGHT - 1) - i, YELLOW);
    }

    // Horizontal and vertical lines. Cyan at the top, magenta on the bottom,
    // brown on the left, lightgrey on the right.
    CYG_FB_WRITE_HLINE(FRAMEBUF, 32,         16,            WIDTH - 64, CYAN);
    CYG_FB_WRITE_HLINE(FRAMEBUF, 32,         HEIGHT - 16,   WIDTH - 64, MAGENTA);
    CYG_FB_WRITE_VLINE(FRAMEBUF, 16,         32,            HEIGHT - 64, BROWN);
    CYG_FB_WRITE_VLINE(FRAMEBUF, WIDTH - 16, 32,            HEIGHT - 64, LIGHTGREY);
	
	// And 14 vertical stripes, from blue to yellow, in the centre of the box.
    block_width     = (WIDTH - 100) / 14;
    for (i = 1; i <= 14; i++) {
        CYG_FB_FILL_BLOCK(FRAMEBUF, 50 + ((i - 1) * block_width), 50, block_width, HEIGHT - 100, colours[i]);
    }


#undef FRAMEBUF
#undef WIDTH
#undef HEIGHT
}

// ----------------------------------------------------------------------------
// main(). Start up separate threads for FB1 and FB2, run the FB0 code since
// it just does some drawing and finishes, then run the FB3 code.
//static cyg_thread       fb1_thread_data;
//static cyg_handle_t     fb1_thread_handle;
//static unsigned char    fb1_thread_stack[CYGNUM_HAL_STACK_SIZE_TYPICAL];
static cyg_thread       fb2_thread_data;
static cyg_handle_t     fb2_thread_handle;
static unsigned char    fb2_thread_stack[CYGNUM_HAL_STACK_SIZE_TYPICAL];

void cyg_user_start(void)
{
    //cyg_thread_create(10, &fb1_thread, 0, "fb1", fb1_thread_stack, CYGNUM_HAL_STACK_SIZE_TYPICAL, &fb1_thread_handle, &fb1_thread_data);
    cyg_thread_create(10, &fb2_thread, 0, "fb2", fb2_thread_stack, CYGNUM_HAL_STACK_SIZE_TYPICAL, &fb2_thread_handle, &fb2_thread_data);
    //cyg_thread_resume(fb1_thread_handle);
    cyg_thread_resume(fb2_thread_handle);
    //fb0_thread(0);
    //fb3_thread(0);
    return 0;
}

