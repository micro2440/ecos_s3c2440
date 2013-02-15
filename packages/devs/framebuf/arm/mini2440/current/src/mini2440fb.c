//==========================================================================
//
//      mini2440fb_init.cxx
//
//      Instantiate a framebuffer device for stm32 TFT LCD.
//      Cloned from synth initialization.
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2008, 2009 Free Software Foundation, Inc.                        
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
// Author(s):     Ricky Wu
//
// Date:          2011-08-17
//
//
//###DESCRIPTIONEND####


#include <pkgconf/devs_framebuf_mini2440.h>
#include <pkgconf/io_framebuf.h>
#include <cyg/io/framebuf.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_io.h>
#include <cyg/io/io.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/devtab.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_trac.h>         // tracing macros
#include <cyg/infra/cyg_ass.h>          // assertion macros

#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/hal_intr.h>           // Interrupt names
#include <cyg/hal/hal_cache.h>
#include <cyg/hal/s3c2440x.h>           // Platform specifics

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <cyg/io/framebuf.h>

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(a) (sizeof (a) / sizeof (a)[0])
#endif

#include CYGHWR_MEMORY_LAYOUT_H
#define cyg_mini2440_fb0_base  ((volatile cyg_uint8*)CYGMEM_REGION_lcd)


#define LCD_TYPE_N35			1	//NEC  LCD
#define LCD_TYPE_A70			2	//7
#define LCD_TYPE_VGA1024x768	3

#define MVAL		(13)
#define MVAL_USED 	(0)				//0=each frame   1=rate by MVAL
#define INVVDEN		(1)				//0=normal       1=inverted
#define BSWP		(0)				//Byte swap control
#define HWSWP		(1)				//Half word swap control

#define M5D(n) ((n) & 0x1fffff)		//To get lower 21bits

#define LCD_TYPE LCD_TYPE_N35

#if LCD_TYPE == LCD_TYPE_N35
	//TFT 240320
	#define LCD_TFT_XSIZE 	(CYGNUM_DEVS_FRAMEBUF_MINI2440_FB0_WIDTH)	
	#define LCD_TFT_YSIZE 	(CYGNUM_DEVS_FRAMEBUF_MINI2440_FB0_HEIGHT)

	#define SCR_XSIZE_TFT 	(CYGNUM_DEVS_FRAMEBUF_MINI2440_FB0_WIDTH)
	#define SCR_YSIZE_TFT 	(CYGNUM_DEVS_FRAMEBUF_MINI2440_FB0_HEIGHT)

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




#define CYG_BF_FRAMEBUFFER_DATAWIDTH_8  0

#define EXPLICITLY_SET_CONTROL_PINS     0
#define TIME_SYNCH                      0


typedef struct CYG_BF_LCD {
    const char         *fb_id;
    int                 id;
} cyg_mini2440_lcd_t;

#ifdef CYGPKG_DEVS_FRAMEBUF_MINI2440_FB0


cyg_mini2440_lcd_t cyg_mini2440_fb0_lcd = {
    .fb_id = "fb0",
    .id = 0,
};

#endif


/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  volatile cyg_uint16 LCD_REG;
  volatile cyg_uint16 LCD_RAM;
} LCD_TypeDef;

/* Note: LCD /CS is CE4 - Bank 4 of NOR/SRAM Bank 1~4 */
#define LCD_BASE        ((cyg_uint32)(0x60000000 | 0x0C000000))
#define LCD             ((LCD_TypeDef *) LCD_BASE)

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/                              
/* Private variables ---------------------------------------------------------*/
  /* Global variables to set the written text color */
static volatile cyg_uint16 TextColor = 0x0000, BackColor = 0xFFFF;
  
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*initialize LCD hardware interface*/

// Initialize LCD hardware

void
MINI2440_LCD_Init(void)
{
    cyg_uint32 reg;

    //HAL_VIRT_TO_PHYS_ADDRESS(LCD_FRAMEBUFFER, cyg_mini2440_fb0_base);

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

    HAL_WRITE_UINT32(LCDSADDR1,((cyg_uint32)cyg_mini2440_fb0_base>>22)<<21|M5D(((cyg_uint32)cyg_mini2440_fb0_base>>1)));
    HAL_WRITE_UINT32(LCDSADDR2,M5D(((cyg_uint32)cyg_mini2440_fb0_base+(SCR_XSIZE_TFT*LCD_TFT_YSIZE*2))>>1 ));
    HAL_WRITE_UINT32(LCDSADDR3,(((SCR_XSIZE_TFT-LCD_TFT_XSIZE)/1)<<11)|(LCD_TFT_XSIZE/1));

    HAL_READ_UINT32(LCDINTMSK, reg);
    reg |= 3;
    HAL_WRITE_UINT32(LCDINTMSK, reg);

    HAL_READ_UINT32(LPCSEL, reg);
    reg &= ~7;
    HAL_WRITE_UINT32(LPCSEL, reg);

    HAL_WRITE_UINT32(TPAL, 0);
}


/*display on and off function*/
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

/*lcd power on*/
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

static int
fb_lcd_init(cyg_mini2440_lcd_t *lcd)
{
    //diag_printf("%s()\n", __func__);
	MINI2440_LCD_Init();
    return 0;
}

typedef struct CYG_FB_MINI2440 {
    volatile cyg_uint16 data_mapped[CYG_FB_fb0_WIDTH * CYG_FB_fb0_HEIGHT];
} cyg_fb_mini2440_t;


#ifdef CYGPKG_DEVS_FRAMEBUF_MINI2440_FB0
static cyg_fb_mini2440_t fb0_mini2440;
#endif



static
void mini2440_lcd_initial(cyg_mini2440_lcd_t *lcd)
{
    //diag_printf("%s()\n", __func__);
	Lcd_PowerEnable(0, 1);
}


#ifdef CYGPKG_DEVS_FRAMEBUF_MINI2440_FB0

static int mini2440_fb_on(struct cyg_fb *fb)
{
    //cyg_mini2440_lcd_t *lcd = (cyg_mini2440_lcd_t *)fb->fb_driver0;
	//diag_printf("%s()\n", __func__);
	Lcd_EnvidOnOff(1);
    return 0;
}


static int mini2440_fb_off(struct cyg_fb *fb)
{
    //cyg_mini2440_lcd_t *lcd = (cyg_mini2440_lcd_t *)fb->fb_driver0;
	//diag_printf("%s()\n", __func__);
	Lcd_EnvidOnOff(0);
    return 0;
}


/* @@@@ ToDo: might this be a candidate for DMA? */
static void mini2440_fb_synch(struct cyg_fb *fb, cyg_ucount16 when)
{
    //cyg_mini2440_lcd_t *lcd = (cyg_mini2440_lcd_t *)fb->fb_driver0;

	diag_printf("%s(): enter\n", __func__);
}


static int
mini2440_fb_ioctl(struct cyg_fb *fb, cyg_ucount16 a, void *b, size_t *s)
{
    return -1;
}


CYG_FB_FRAMEBUFFER(CYG_FB_fb0_STRUCT,
                   CYG_FB_fb0_DEPTH,
                   CYG_FB_fb0_FORMAT,
                   CYG_FB_fb0_WIDTH,
                   CYG_FB_fb0_HEIGHT,
                   CYG_FB_fb0_WIDTH,
                   CYG_FB_fb0_HEIGHT,
                   CYG_FB_fb0_BASE,
                   CYG_FB_fb0_STRIDE,
                   CYG_FB_fb0_FLAGS0,
                   CYG_FB_fb0_FLAGS1,
                   CYG_FB_fb0_FLAGS2,
                   CYG_FB_fb0_FLAGS3,
                   (CYG_ADDRWORD) &cyg_mini2440_fb0_lcd,
                   (CYG_ADDRWORD) 1,
                   (CYG_ADDRWORD) &fb0_mini2440,
                   (CYG_ADDRWORD) CYGMEM_REGION_lcd,
                   &mini2440_fb_on,
                   &mini2440_fb_off,
                   &mini2440_fb_ioctl,
                   &mini2440_fb_synch,
                   &cyg_fb_nop_read_palette,
                   &cyg_fb_nop_write_palette,
                   &cyg_fb_dev_make_colour_16bpp_true_565,
                   &cyg_fb_dev_break_colour_16bpp_true_565,
                   &cyg_fb_linear_write_pixel_16,
                   &cyg_fb_linear_read_pixel_16,
                   &cyg_fb_linear_write_hline_16,
                   &cyg_fb_linear_write_vline_16,
                   &cyg_fb_linear_fill_block_16,
                   &cyg_fb_linear_write_block_16,
                   &cyg_fb_linear_read_block_16,
                   &cyg_fb_linear_move_block_16,
                   0, 0, 0, 0
                   );

#endif  /* def CYGPKG_DEVS_FRAMEBUF_MINI2440_FB0 */



// Create a framebuffer device. This gets called from a C++ static constructor.
void
cyg_mini2440_fb0_instantiate(struct cyg_fb *fb)
{
    cyg_mini2440_lcd_t *lcd = (cyg_mini2440_lcd_t *)fb->fb_driver0;
    //cyg_fb_mini2440_t *mini2440 = (cyg_fb_mini2440_t *)fb->fb_driver2;

    fb_lcd_init(lcd);

    mini2440_lcd_initial(lcd);
}
