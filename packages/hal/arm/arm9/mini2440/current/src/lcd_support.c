//==========================================================================
//
//        Lcd_support.c
//
//        Agilent MINI2440 - LCD support routines
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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
//#####DESCRIPTIONBEGIN####
//
// Author(s):     Ricky Wu <rickleaf.wu@gmail.com>
// Contributors:  Ricky Wu <rickleaf.wu@gmail.com>
// Date:          2011-03-21
// Description:   Simple LCD support
//####DESCRIPTIONEND####

#include <pkgconf/hal.h>

#include <cyg/infra/diag.h>
#include <cyg/hal/hal_io.h>       // IO macros
#include <cyg/hal/hal_if.h>       // Virtual vector support
#include <cyg/hal/hal_arch.h>     // Register state info
#include <cyg/hal/hal_intr.h>     // HAL interrupt macros

#include <cyg/hal/s3c2440x.h>  // Board definitions
#include <cyg/hal/lcd_support.h>
#include <cyg/hal/hal_cache.h>

#include <string.h>

#define USE_RGB565 1

#ifdef CYGPKG_ISOINFRA
# include <pkgconf/isoinfra.h>
# ifdef CYGINT_ISO_STDIO_FORMATTED_IO
#  include <stdio.h>  // sscanf
# endif
#endif

#include CYGHWR_MEMORY_LAYOUT_H
#define LCD_FRAMEBUFFER CYGMEM_REGION_lcd
static cyg_uint32 lcd_framebuffer;

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#define LCD_TYPE_N35			1	//NEC3.5¥ÁLCD
#define LCD_TYPE_A70	2	//7
#define LCD_TYPE_VGA1024x768 3

#define LCD_TYPE LCD_TYPE_N35

#define MVAL		(13)
#define MVAL_USED 	(0)		//0=each frame   1=rate by MVAL
#define INVVDEN		(1)		//0=normal       1=inverted
#define BSWP		(0)		//Byte swap control
#define HWSWP		(1)		//Half word swap control

#define M5D(n) ((n) & 0x1fffff)	// To get lower 21bits

#if LCD_TYPE == LCD_TYPE_N35
//TFT 240320
#define LCD_TFT_XSIZE 	(640)	
#define LCD_TFT_YSIZE 	(480)

#define SCR_XSIZE_TFT 	(640)
#define SCR_YSIZE_TFT 	(480)

//TFT240320
#define HOZVAL_TFT	(LCD_TFT_XSIZE-1)
#define LINEVAL_TFT	(LCD_TFT_YSIZE-1)


//Timing parameter for LCD LQ035Q7DB02
#define VBPD		(1)		
#define VFPD		(5)		
#define VSPW		(1)

#define HBPD		(35)
#define HFPD		(19)
#define HSPW		(5)

#define CLKVAL_TFT	(4) 	
//FCLK=180MHz,HCLK=90MHz,VCLK=6.5MHz

#elif LCD_TYPE == LCD_TYPE_A70

#define LCD_TFT_XSIZE 	(800)	
#define LCD_TFT_YSIZE 	(480)

#define SCR_XSIZE_TFT 	(800)
#define SCR_YSIZE_TFT 	(480)

//TFT240320
#define HOZVAL_TFT	(LCD_TFT_XSIZE-1)
#define LINEVAL_TFT	(LCD_TFT_YSIZE-1)

//Timing parameter for LCD LQ035Q7DB02
#define VBPD		(32)
#define VFPD		(9)
#define VSPW		(1)

#define HBPD		(47)
#define HFPD		(15)
#define HSPW		(95)

#define CLKVAL_TFT	(2) 	
//FCLK=180MHz,HCLK=90MHz,VCLK=6.5MHz

#elif LCD_TYPE == LCD_TYPE_VGA1024x768

#define LCD_TFT_XSIZE 	(1024)	
#define LCD_TFT_YSIZE 	(768)

#define SCR_XSIZE_TFT 	(1024)
#define SCR_YSIZE_TFT 	(768)

//TFT240320
#define HOZVAL_TFT	(LCD_TFT_XSIZE-1)
#define LINEVAL_TFT	(LCD_TFT_YSIZE-1)

//Timing parameter for LCD LQ035Q7DB02
#define VBPD		(1)
#define VFPD		(1)
#define VSPW		(1)

#define HBPD		(15)
#define HFPD		(19)
#define HSPW		(15)
#define CLKVAL_TFT	(2) 	
//FCLK=180MHz,HCLK=90MHz,VCLK=6.5MHz
#endif

// Physical dimensions of LCD display
#define DISPLAY_WIDTH  640
#define DISPLAY_HEIGHT 480

// Logical layout
#ifdef CYGSEM_MINI2440_LCD_PORTRAIT_MODE
#define LCD_WIDTH  DISPLAY_HEIGHT
#define LCD_HEIGHT DISPLAY_WIDTH
#else
#define LCD_WIDTH  DISPLAY_WIDTH
#define LCD_HEIGHT DISPLAY_HEIGHT
#endif
#define LCD_DEPTH   16

#define RGB_RED(x)   (((x)&0x1F)<<11)
#define RGB_GREEN(x) (((x)&0x3F)<<5)
#define RGB_BLUE(x)  (((x)&0x1F)<<0)

// Physical screen info
//static int lcd_depth  = LCD_DEPTH;  // Should be 1, 2, or 4
static int lcd_bpp;
#ifdef CYGSEM_MINI2440_LCD_PORTRAIT_MODE
static int lcd_width  = LCD_WIDTH;
#endif
static int lcd_height = LCD_HEIGHT;

// Black on light blue
static int bg = RGB_RED(0x0) | RGB_GREEN(0x0) | RGB_BLUE(0x0);
#ifdef CYGSEM_MINI2440_LCD_COMM
static int fg = RGB_RED(0x17) | RGB_GREEN(0x37) | RGB_BLUE(0x1f);
#endif

// Compute the location for a pixel within the framebuffer
static cyg_uint32 *
lcd_fb(int row, int col)
{
    return (cyg_uint32 *)(lcd_framebuffer+(((row*DISPLAY_WIDTH)+col)*2));
}

static void Lcd_PowerEnable(int invpwren,int pwren)
{
	cyg_uint32 reg;

	/*GPG4 is setted as LCD_PWREN*/
	/*Pull-up disable*/
	HAL_READ_UINT32(GPGUP, reg);
	reg &= (~(1<<4))|(1<<4);
	HAL_WRITE_UINT32(GPGUP, reg);

	/*GPG4=LCD_PWREN*/
	HAL_READ_UINT32(GPGCON, reg);
	reg &= (~(3<<8))|(3<<8);
	HAL_WRITE_UINT32(GPGCON, reg);

	HAL_READ_UINT32(GPGDAT, reg);
	reg |= (1<<4);
	HAL_WRITE_UINT32(GPGDAT, reg);

	/*invpwren=pwren;*/
	/*Enable LCD POWER ENABLE Function*/

	/*PWREN*/
	HAL_READ_UINT32(LCDCON5, reg);
	reg &= ~(1<<3)|(pwren<<3);
	HAL_WRITE_UINT32(LCDCON5, reg);

	/*INVPWREN*/
	HAL_READ_UINT32(LCDCON5, reg);
	reg &= ~(1<<5)|(invpwren<<5);
	HAL_WRITE_UINT32(LCDCON5, reg);
}

void Lcd_EnvidOnOff(int onoff)
{
	cyg_uint32 reg;
    if(onoff==1){
		/*ENVID=ON*/
		HAL_READ_UINT32(LCDCON1, reg);
    	reg |= 1;
    	HAL_WRITE_UINT32(LCDCON1, reg);
	}
	else{
		/*ENVID Off*/
		HAL_READ_UINT32(LCDCON1, reg);
    	reg &= 0x3fffe;
    	HAL_WRITE_UINT32(LCDCON1, reg);
		
	}
}

void
lcd_on(bool enable)
{
    cyg_uint32 ctl;

    if (!enable) return;  // FIXME - need a way to [safely] blank screen

    Lcd_PowerEnable(0, 1);
    Lcd_EnvidOnOff(1);
}

// Initialize LCD hardware

void
lcd_init(int depth)
{
    cyg_uint32 reg;

    HAL_VIRT_TO_PHYS_ADDRESS(LCD_FRAMEBUFFER, lcd_framebuffer);

    HAL_READ_UINT32(GPBUP, reg);
	reg &= (~(1<<1));
	HAL_WRITE_UINT32(GPBUP, reg);

	HAL_READ_UINT32(GPBCON, reg);
	reg &= (~(3<<1))|(3<<1);
	HAL_WRITE_UINT32(GPBCON, reg);

	HAL_READ_UINT32(GPBDAT, reg);
	reg |= (1<<1);
	HAL_WRITE_UINT32(GPBDAT, reg);


    HAL_WRITE_UINT32(GPCUP,0x00000000);
	HAL_WRITE_UINT32(GPCCON,0xaaaa02a9);

    HAL_WRITE_UINT32(GPDUP,0x00000000);

	/*Initialize VD[15:8]*/
    HAL_WRITE_UINT32(GPDCON,0xaaaaaaaa);

    HAL_WRITE_UINT32(LCDCON1,((CLKVAL_TFT<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0));

    /* TFT LCD panel,16bpp TFT,ENVID=off*/
    HAL_WRITE_UINT32(LCDCON2,((VBPD<<24)|(LINEVAL_TFT<<14)|(VFPD<<6)|(VSPW)));
    HAL_WRITE_UINT32(LCDCON3,((HBPD<<19)|(HOZVAL_TFT<<8)|(HFPD)));
    HAL_WRITE_UINT32(LCDCON4,((MVAL<<8)|(HSPW)));

#if LCD_TYPE==LCD_TYPE_VGA1024x768
    HAL_WRITE_UINT32(LCDCON5,(1<<11)|(0<<9)|(0<<8)|(0<<3)|(1<<0));
#else
    HAL_WRITE_UINT32(LCDCON5,(1<<11) | (1<<10) | (1<<9) | (1<<8) | (0<<7) | (0<<6)
             | (1<<3)  |(BSWP<<1) | (HWSWP));
    /* 5:6:5 VCLK posedge BSWP=0,HWSWP=1;*/
#endif

    HAL_WRITE_UINT32(LCDSADDR1,((cyg_uint32)lcd_framebuffer>>22)<<21|M5D(((cyg_uint32)lcd_framebuffer>>1)));
    HAL_WRITE_UINT32(LCDSADDR2,M5D(((cyg_uint32)lcd_framebuffer+(SCR_XSIZE_TFT*LCD_TFT_YSIZE*2))>>1 ));
    HAL_WRITE_UINT32(LCDSADDR3,(((SCR_XSIZE_TFT-LCD_TFT_XSIZE)/1)<<11)|(LCD_TFT_XSIZE/1));

    HAL_READ_UINT32(LCDINTMSK, reg);
    reg |= 3;
    HAL_WRITE_UINT32(LCDINTMSK, reg);

    HAL_READ_UINT32(LPCSEL, reg);
    reg &= ~7;
    HAL_WRITE_UINT32(LPCSEL, reg);

    HAL_WRITE_UINT32(TPAL, 0);

    lcd_on(true);
    lcd_bpp = 16;
}

// Get information about the frame buffer
int
lcd_getinfo(struct lcd_info *info)
{
    if (lcd_bpp == 0) {
        return 0;  // LCD not initialized
    }
    info->width = DISPLAY_WIDTH;
    info->height = DISPLAY_HEIGHT;
    info->bpp = lcd_bpp;
    info->fb = (void*)LCD_FRAMEBUFFER;
    info->rlen = DISPLAY_WIDTH * 2;
    info->type = FB_TRUE_RGB565;
    return 1; // Information valid
}

// Clear screen
void
lcd_clear(void)
{
#ifndef CYGSEM_MINI2440_LCD_PORTRAIT_MODE
    cyg_uint32 *fb_row0, *fb_rown;
    cyg_uint32 _bg = (bg<<16)|bg;

    fb_row0 = lcd_fb(0, 0);
    fb_rown = lcd_fb(lcd_height, 0);
    while (fb_row0 != fb_rown) {
        *fb_row0++ = _bg;
    }
#else
    int row, col;
    for (row = 0;  row < lcd_height;  row++) {
        for (col = 0;  col < lcd_width;  col++) {
            set_pixel(row, col, bg);
        }
    }
#endif
}

#ifdef CYGSEM_MINI2440_LCD_COMM

//
// Additional support for LCD/Keyboard as 'console' device
//

#ifdef CYGOPT_MINI2440_LCD_COMM_LOGO
#include "banner.xpm"
#endif
#include "font.h"

// Virtual screen info
static int curX = 0;  // Last used position
static int curY = 0;
//static int width = LCD_WIDTH / (FONT_WIDTH*NIBBLES_PER_PIXEL);
//static int height = LCD_HEIGHT / (FONT_HEIGHT*SCREEN_SCALE);

#define SCREEN_PAN            20
#define SCREEN_WIDTH          80
#define SCREEN_HEIGHT         (LCD_HEIGHT/FONT_HEIGHT)
#define VISIBLE_SCREEN_WIDTH  (LCD_WIDTH/FONT_WIDTH)
#define VISIBLE_SCREEN_HEIGHT (LCD_HEIGHT/FONT_HEIGHT)
static char screen[SCREEN_HEIGHT][SCREEN_WIDTH];
static int screen_height = SCREEN_HEIGHT;
static int screen_width = SCREEN_WIDTH;
static int screen_pan = 0;

// Usable area on screen [logical pixel rows]
static int screen_start = 0;                       
static int screen_end = LCD_HEIGHT/FONT_HEIGHT;

static bool cursor_enable = true;

// Functions
static void lcd_drawc(cyg_int8 c, int x, int y);

// Note: val is a 16 bit, RGB555 value which must be mapped
// onto a 12 bit value.

#define RED(v)  ((v>>1) & 0x0F)
#define GREEN(v) ((v>>7) & 0x0F)
#define BLUE(v)   ((v>>10) & 0x0F)

#ifdef CYGSEM_MINI2440_LCD_PORTRAIT_MODE
#error PORTRAIT MODE NOT IMPLEMENTED
// Translate coordinates, rotating clockwise 90 degrees
static void
set_pixel(int row, int col, unsigned short val)
{
	if ( (row < SCR_XSIZE_TFT) && (col < SCR_YSIZE_TFT) )
        *(lcd_fb(row,col))= val;
}
#else

static void
set_pixel(int row, int col, unsigned short val)
{
    if ( (row < SCR_XSIZE_TFT) && (col < SCR_YSIZE_TFT) )
		*(lcd_fb(row,col)) = val;
}
#endif

static int
_hexdigit(char c)
{
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    } else
    if ((c >= 'A') && (c <= 'F')) {
        return (c - 'A') + 0x0A;
    } else
    if ((c >= 'a') && (c <= 'f')) {
        return (c - 'a') + 0x0a;
    }

    return 0;
}

static int
_hex(char *cp)
{
    return (_hexdigit(*cp)<<4) | _hexdigit(*(cp+1));
}

static unsigned short
parse_color(char *cp)
{
    int red, green, blue;

    while (*cp && (*cp != 'c')) cp++;
    if (cp) {
        cp += 2;
        if (*cp == '#') {
            red = _hex(cp+1);
            green = _hex(cp+3);
            blue = _hex(cp+5);
#ifdef USE_RGB565
            return RGB_RED(red>>3) | RGB_GREEN(green>>2) | RGB_BLUE(blue>>3);
#else
            return RGB_RED(red>>3) | RGB_GREEN(green>>3) | RGB_BLUE(blue>>3);
#endif
        } else {
            // Should be "None"
            return 0xFFFF;
        }
    } else {
        return 0xFFFF;
    }
}

#ifndef CYGINT_ISO_STDIO_FORMATTED_IO
static int
get_int(char **_cp)
{
    char *cp = *_cp;
    char c;
    int val = 0;
    
    while ((c = *cp++) && (c != ' ')) {
        if ((c >= '0') && (c <= '9')) {
            val = val * 10 + (c - '0');
        } else {
            return -1;
        }
    }
    *_cp = cp;
    return val;
}
#endif

#ifdef CYGOPT_MINI2440_LCD_COMM_LOGO
int
show_xpm(char *xpm[], int screen_pos)
{
    int i, row, col, offset;
    char *cp;
    int nrows, ncols, nclrs;
    unsigned short colors[256];  // Mapped by character index

    cp = xpm[0];
#ifdef CYGINT_ISO_STDIO_FORMATTED_IO
    if (sscanf(cp, "%d %d %d", &ncols, &nrows, &nclrs) != 3) {
#else
    if (((ncols = get_int(&cp)) < 0) ||
        ((nrows = get_int(&cp)) < 0) ||
        ((nclrs = get_int(&cp)) < 0)) {

#endif
        diag_printf("Can't parse XPM data, sorry\n");
        return 0;
    }
    // printf("%d rows, %d cols, %d colors\n", nrows, ncols, nclrs);

    for (i = 0;  i < 256;  i++) {
        colors[i] = 0x0000;
    }
    for (i = 0;  i < nclrs;  i++) {
        cp = xpm[i+1];
        colors[(unsigned int)*cp] = parse_color(&cp[1]);
        // printf("Color[%c] = %x\n", *cp, colors[(unsigned int)*cp]);
    }

#ifdef CYGOPT_MINI2440_LCD_COMM_LOGO_TOP
    offset = screen_pos;
#else
    offset = screen_pos-nrows;
#endif
    for (row = 0;  row < nrows;  row++) {            
        cp = xpm[nclrs+1+row];        
        for (col = 0;  col < ncols;  col++) {
            set_pixel(row+offset, col, colors[(unsigned int)*cp++]);
        }
    }
#ifdef CYGOPT_MINI2440_LCD_COMM_LOGO_TOP
    screen_start = (nrows + (FONT_HEIGHT-1))/FONT_HEIGHT;
    screen_end = LCD_HEIGHT/FONT_HEIGHT;
    return offset+nrows;
#else    
    screen_start = 0;
    screen_height = offset / FONT_HEIGHT;
    screen_end = screen_height;
    return offset;
#endif
}
#endif

void
lcd_screen_clear(void)
{
    int row, col, pos;
    for (row = 0;  row < screen_height;  row++) {
        for (col = 0;  col < screen_width;  col++) {
            screen[row][col] = ' ';
        }
    }
#ifdef CYGOPT_MINI2440_LCD_COMM_LOGO
    // Note: Row 0 seems to wrap incorrectly
#ifdef CYGOPT_MINI2440_LCD_COMM_LOGO_TOP
    pos = 0;
#else
    pos = (LCD_HEIGHT-1);
#endif
    show_xpm(banner_xpm, pos);
#endif // CYGOPT_MINI2440_LCD_COMM_LOGO
    curX = 0;  curY = screen_start;
    if (cursor_enable) {
        lcd_drawc(CURSOR_ON, curX-screen_pan, curY);
    }
}

// Position cursor
void
lcd_moveto(int X, int Y)
{
    if (cursor_enable) {
        lcd_drawc(screen[curY][curX], curX-screen_pan, curY);
    }
    if (X < 0) X = 0;
    if (X >= screen_width) X = screen_width-1;
    curX = X;
    if (Y < screen_start) Y = screen_start;
    if (Y >= screen_height) Y = screen_height-1;
    curY = Y;
    if (cursor_enable) {
        lcd_drawc(CURSOR_ON, curX-screen_pan, curY);
    }
}

// Render a character at position (X,Y) with current background/foreground
static void
lcd_drawc(cyg_int8 c, int x, int y)
{
    cyg_uint8 bits;
    int l, p;
    int xoff, yoff;
#ifndef CYGSEM_MINI2440_LCD_PORTRAIT_MODE
    cyg_uint32 *fb;
#endif


    if ((x < 0) || (x >= VISIBLE_SCREEN_WIDTH) || 
        (y < 0) || (y >= screen_height)) return;  
    for (l = 0;  l < FONT_HEIGHT;  l++) {
        bits = font_table[c-FIRST_CHAR][l]; 
        yoff = y*FONT_HEIGHT + l;
        xoff = x*FONT_HEIGHT;
#ifndef CYGSEM_MINI2440_LCD_PORTRAIT_MODE
        // Caution - only works for little-endian & font sizes multiple of 2
        fb = lcd_fb(yoff, xoff);
        for (p = 0;  p < FONT_WIDTH;  p += 2) {
            switch (bits & 0x03) {
            case 0:
                *fb++ = (bg << 16) | bg;
                break;
            case 1:
                *fb++ = (bg << 16) | fg;
                break;
            case 2:
                *fb++ = (fg << 16) | bg;
                break;
            case 3:
                *fb++ = (fg << 16) | fg;
                break;
            }
            bits >>= 2;
        }
#else
        for (p = 0;  p < FONT_WIDTH;  p++) {
            set_pixel(yoff, xoff + p, (bits & 0x01) ? fg : bg);
            bits >>= 1;
        }
#endif
    }
}

static void
lcd_refresh(void)
{
    int row, col;

    for (row = screen_start;  row < screen_height;  row++) {
        for (col = 0;  col < VISIBLE_SCREEN_WIDTH;  col++) {
            if ((col+screen_pan) < screen_width) {
                lcd_drawc(screen[row][col+screen_pan], col, row);
            } else {
                lcd_drawc(' ', col, row);
            }
        }
    }
    if (cursor_enable) {
        lcd_drawc(CURSOR_ON, curX-screen_pan, curY);
    }
}

static void
lcd_scroll(void)
{
    int col;
    cyg_uint8 *c1;
    cyg_uint32 *lc0, *lc1, *lcn;
#ifndef CYGSEM_MINI2440_LCD_PORTRAIT_MODE
    cyg_uint32 *fb_row0, *fb_row1, *fb_rown;
#endif

    // First scroll up the virtual screen
#if ((SCREEN_WIDTH%4) != 0)
#error Scroll code optimized for screen with multiple of 4 columns
#endif
    lc0 = (cyg_uint32 *)&screen[0][0];
    lc1 = (cyg_uint32 *)&screen[1][0];
    lcn = (cyg_uint32 *)&screen[screen_height][0];
    while (lc1 != lcn) {
        *lc0++ = *lc1++;
    }
    c1 = &screen[screen_height-1][0];
    for (col = 0;  col < screen_width;  col++) {
        *c1++ = 0x20;
    }
#ifdef CYGSEM_MINI2440_LCD_PORTRAIT_MODE
    // Redrawing the screen in this mode is hard :-)
    lcd_refresh();
#else
    fb_row0 = lcd_fb(screen_start*FONT_HEIGHT, 0);
    fb_row1 = lcd_fb((screen_start+1)*FONT_HEIGHT, 0);
    fb_rown = lcd_fb(screen_end*FONT_HEIGHT, 0);
#if 1
    while (fb_row1 != fb_rown) {
        *fb_row0++ = *fb_row1++;
    }
#else
    // Optimized ARM assembly "move" code
    asm __volatile(
        "mov r0,%0;"
        "mov r1,%1;"
        "mov r2,%2;"
        "10: ldmia r1!,{r3-r10};"
        "stmia r0!,{r3-r10};"
        "cmp r1,r2;"
        "bne 10b"
        :
        : "r"(fb_row0), "r"(fb_row1), "r"(fb_rown)
        : "r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10"
        );
#endif
    // Erase bottom line
    for (col = 0;  col < screen_width;  col++) {
        lcd_drawc(' ', col, screen_end-1);
    }
#endif
}

// Draw one character at the current position
void
lcd_putc(cyg_int8 c)
{
    if (cursor_enable) {
        lcd_drawc(screen[curY][curX], curX-screen_pan, curY);
    }
    switch (c) {
    case '\r':
        curX = 0;
        break;
    case '\n':
        curY++;
        break;
    case '\b':
        curX--;
        if (curX < 0) {
            curY--;
            if (curY < 0) curY = 0;
            curX = screen_width-1;
        }
        break;
    default:
        if (((cyg_uint8)c < FIRST_CHAR) || ((cyg_uint8)c > LAST_CHAR)) c = '.';
        screen[curY][curX] = c;
        lcd_drawc(c, curX-screen_pan, curY);
        curX++;
        if (curX == screen_width) {
            curY++;
            curX = 0;
        }
    } 
    if (curY >= screen_height) {
        lcd_scroll();
        curY = (screen_height-1);
    }
    if (cursor_enable) {
        lcd_drawc(CURSOR_ON, curX-screen_pan, curY);
    }
}

// Basic LCD 'printf()' support

#include <stdarg.h>

#define is_digit(c) ((c >= '0') && (c <= '9'))

static int
_cvt(unsigned long val, char *buf, long radix, char *digits)
{
    char temp[80];
    char *cp = temp;
    int length = 0;
    if (val == 0) {
        /* Special case */
        *cp++ = '0';
    } else {
        while (val) {
            *cp++ = digits[val % radix];
            val /= radix;
        }
    }
    while (cp != temp) {
        *buf++ = *--cp;
        length++;
    }
    *buf = '\0';
    return (length);
}

static int
lcd_vprintf(void (*putc)(cyg_int8), const char *fmt0, va_list ap)
{
    char c, sign, *cp;
    int left_prec, right_prec, zero_fill, length, pad, pad_on_right;
    char buf[32];
    long val;
    while ((c = *fmt0++)) {
        cp = buf;
        length = 0;
        if (c == '%') {
            c = *fmt0++;
            left_prec = right_prec = pad_on_right = 0;
            if (c == '-') {
                c = *fmt0++;
                pad_on_right++;
            }
            if (c == '0') {
                zero_fill = TRUE;
                c = *fmt0++;
            } else {
                zero_fill = FALSE;
            }
            while (is_digit(c)) {
                left_prec = (left_prec * 10) + (c - '0');
                c = *fmt0++;
            }
            if (c == '.') {
                c = *fmt0++;
                zero_fill++;
                while (is_digit(c)) {
                    right_prec = (right_prec * 10) + (c - '0');
                    c = *fmt0++;
                }
            } else {
                right_prec = left_prec;
            }
            sign = '\0';
            switch (c) {
            case 'd':
            case 'x':
            case 'X':
                val = va_arg(ap, long);
                switch (c) {
                case 'd':
                    if (val < 0) {
                        sign = '-';
                        val = -val;
                    }
                    length = _cvt(val, buf, 10, "0123456789");
                    break;
                case 'x':
                    length = _cvt(val, buf, 16, "0123456789abcdef");
                    break;
                case 'X':
                    length = _cvt(val, buf, 16, "0123456789ABCDEF");
                    break;
                }
                break;
            case 's':
                cp = va_arg(ap, char *);
                length = strlen(cp);
                break;
            case 'c':
                c = va_arg(ap, long /*char*/);
                (*putc)(c);
                continue;
            default:
                (*putc)('?');
            }
            pad = left_prec - length;
            if (sign != '\0') {
                pad--;
            }
            if (zero_fill) {
                c = '0';
                if (sign != '\0') {
                    (*putc)(sign);
                    sign = '\0';
                }
            } else {
                c = ' ';
            }
            if (!pad_on_right) {
                while (pad-- > 0) {
                    (*putc)(c);
                }
            }
            if (sign != '\0') {
                (*putc)(sign);
            }
            while (length-- > 0) {
                (*putc)(c = *cp++);
                if (c == '\n') {
                    (*putc)('\r');
                }
            }
            if (pad_on_right) {
                while (pad-- > 0) {
                    (*putc)(' ');
                }
            }
        } else {
            (*putc)(c);
            if (c == '\n') {
                (*putc)('\r');
            }
        }
    }

    // FIXME
    return 0;
}

int
_lcd_printf(char const *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = lcd_vprintf(lcd_putc, fmt, ap);
    va_end(ap);
    return (ret);
}

void
lcd_setbg(int red, int green, int blue)
{
    bg = RGB_RED(red) | RGB_GREEN(green) | RGB_BLUE(blue);
}

void
lcd_setfg(int red, int green, int blue)
{
    fg = RGB_RED(red) | RGB_GREEN(green) | RGB_BLUE(blue);
}

//
// Support LCD/keyboard (PS2) as a virtual I/O channel
//   Adapted from i386/pcmb_screen.c
//

static int  _timeout = 500;

static cyg_bool
lcd_comm_getc_nonblock(void* __ch_data, cyg_uint8* ch)
{
    if( !mini2440_KeyboardTest() )
        return false;
    *ch = mini2440_KeyboardGetc();
    return true;
}

static cyg_uint8
lcd_comm_getc(void* __ch_data)
{
    cyg_uint8 ch;

    while (!lcd_comm_getc_nonblock(__ch_data, &ch)) ;
    return ch;
}

static void
lcd_comm_putc(void* __ch_data, cyg_uint8 c)
{
    lcd_putc(c);
}

static void
lcd_comm_write(void* __ch_data, const cyg_uint8* __buf, cyg_uint32 __len)
{
#if 0
    CYGARC_HAL_SAVE_GP();

    while(__len-- > 0)
        lcd_comm_putc(__ch_data, *__buf++);

    CYGARC_HAL_RESTORE_GP();
#endif
}

static void
lcd_comm_read(void* __ch_data, cyg_uint8* __buf, cyg_uint32 __len)
{
#if 0
    CYGARC_HAL_SAVE_GP();

    while(__len-- > 0)
        *__buf++ = lcd_comm_getc(__ch_data);

    CYGARC_HAL_RESTORE_GP();
#endif
}

static cyg_bool
lcd_comm_getc_timeout(void* __ch_data, cyg_uint8* ch)
{
    int delay_count;
    cyg_bool res;

    delay_count = _timeout * 2; // delay in .5 ms steps
    for(;;) {
        res = lcd_comm_getc_nonblock(__ch_data, ch);
        if (res || 0 == delay_count--)
            break;
        CYGACC_CALL_IF_DELAY_US(500);
    }
    return res;
}

static int
lcd_comm_control(void *__ch_data, __comm_control_cmd_t __func, ...)
{
    static int vector = 0;
    int ret = -1;
    static int irq_state = 0;

    CYGARC_HAL_SAVE_GP();

    switch (__func) {
    case __COMMCTL_IRQ_ENABLE:
        ret = irq_state;
        irq_state = 1;
        break;
    case __COMMCTL_IRQ_DISABLE:
        ret = irq_state;
        irq_state = 0;
        break;
    case __COMMCTL_DBG_ISR_VECTOR:
        ret = vector;
        break;
    case __COMMCTL_SET_TIMEOUT:
    {
        va_list ap;

        va_start(ap, __func);

        ret = _timeout;
        _timeout = va_arg(ap, cyg_uint32);

        va_end(ap);
	break;
    }
    case __COMMCTL_FLUSH_OUTPUT:
        ret = 0;
	break;
    default:
        break;
    }
    CYGARC_HAL_RESTORE_GP();
    return ret;
}

static int
lcd_comm_isr(void *__ch_data, int* __ctrlc, 
           CYG_ADDRWORD __vector, CYG_ADDRWORD __data)
{
#if 0
    char ch;

    cyg_drv_interrupt_acknowledge(__vector);
    *__ctrlc = 0;
    if (lcd_comm_getc_nonblock(__ch_data, &ch)) {
        if (ch == 0x03) {
            *__ctrlc = 1;
        }
    }
    return CYG_ISR_HANDLED;
#else
    return 0;
#endif
}

void
lcd_comm_init(void)
{
    static int init = 0;

    if (!init) {
        hal_virtual_comm_table_t* comm;
        int cur = CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);

        init = 1;
        lcd_on(false);
        if (!mini2440_KeyboardInit()) {
            // No keyboard - no LCD display
            return;
        }
        // Initialize screen
        cursor_enable = true;
        lcd_init(16);
        lcd_screen_clear();

        // Setup procs in the vector table
        CYGACC_CALL_IF_SET_CONSOLE_COMM(1);  // FIXME - should be controlled by CDL
        comm = CYGACC_CALL_IF_CONSOLE_PROCS();
        //CYGACC_COMM_IF_CH_DATA_SET(*comm, chan);
        CYGACC_COMM_IF_WRITE_SET(*comm, lcd_comm_write);
        CYGACC_COMM_IF_READ_SET(*comm, lcd_comm_read);
        CYGACC_COMM_IF_PUTC_SET(*comm, lcd_comm_putc);
        CYGACC_COMM_IF_GETC_SET(*comm, lcd_comm_getc);
        CYGACC_COMM_IF_CONTROL_SET(*comm, lcd_comm_control);
        CYGACC_COMM_IF_DBG_ISR_SET(*comm, lcd_comm_isr);
        CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, lcd_comm_getc_timeout);

        // Restore original console
        CYGACC_CALL_IF_SET_CONSOLE_COMM(cur);
    }
}

#ifdef CYGPKG_REDBOOT
#include <redboot.h>

// Get here when RedBoot is idle.  If it's been long enough, then
// dim the LCD.  The problem is - how to determine other activities
// so at this doesn't get in the way.  In the default case, this will
// be called from RedBoot every 10ms (CYGNUM_REDBOOT_CLI_IDLE_TIMEOUT)

#define MAX_IDLE_TIME (30*100)

static void
idle(bool is_idle)
{
    static int idle_time = 0;
    static bool was_idled = false;

    if (is_idle) {
        if (!was_idled) {
            if (++idle_time == MAX_IDLE_TIME) {
                was_idled = true;
                lcd_on(false);
            }
        }
    } else {        
        idle_time = 0;
        if (was_idled) {
            was_idled = false;
                lcd_on(true);
        }
    }
}

RedBoot_idle(idle, RedBoot_AFTER_NETIO);
#endif
#endif
