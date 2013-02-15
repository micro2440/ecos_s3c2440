//==========================================================================
//
//      scr_synth_ecos.c
//
//
//
//==========================================================================
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.
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
// Author(s):    Ricky Wu <rickleaf.wu@gmail.com>
// Contributors:
// Date:         2010-04-16
// Purpose:
// Description:  Microwindows screen driver for synth server on eCos
//
//####DESCRIPTIONEND####
//
//========================================================================*/



#define _GNU_SOURCE 1

#include <pkgconf/system.h>

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <cyg/hal/drv_api.h>
#include <cyg/infra/diag.h>
#include <cyg/io/io.h>
#include <cyg/io/framebuf.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"


static cyg_ucount32 colours[16];
/* In genmem.c*/
MWBOOL  set_subdriver(PSD psd, PSUBDRIVER subdriver, MWBOOL init);

/* Prototypes for driver functions */
static int synth_init(PSD psd);
static PSD  synth_open(PSD psd);
static void synth_close(PSD psd);
static void synth_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void synth_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL synth_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void synth_drawhorizline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void synth_drawvertline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
static void synth_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c);
static void synth_blit(PSD , MWCOORD, MWCOORD, MWCOORD, MWCOORD, PSD, MWCOORD, MWCOORD, long);
static void synth_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
                            MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy,
                            MWCOORD srcw, MWCOORD srch, long op);
static void synth_drawarea(PSD psd, driver_gc_t *gc, int op);
static void synth_preselect(PSD psd);
MWBOOL synth_mapmemgc(PSD, MWCOORD, MWCOORD, int, int, int, int, void *);


#define FRAMEBUF cyg_synth_fb0

#define BLACK        cyg_fb_make_colour(&FRAMEBUF,   0,   0,   0)
#define WHITE        cyg_fb_make_colour(&FRAMEBUF, 255, 255, 255)
#define RED          cyg_fb_make_colour(&FRAMEBUF, 255,   0,   0)
#define GREEN        cyg_fb_make_colour(&FRAMEBUF,   0, 255,   0)
#define BLUE         cyg_fb_make_colour(&FRAMEBUF,   0,   0, 255)
#define YELLOW       cyg_fb_make_colour(&FRAMEBUF, 255, 255,  80)



SCREENDEVICE	scrdev = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
    synth_open,
    synth_close,
    synth_getscreeninfo,
    NULL,
    synth_drawpixel,     /* DrawPixel subdriver*/
    synth_readpixel,     /* ReadPixel subdriver*/
    synth_drawhorizline, /* DrawHorzLine subdriver*/
    synth_drawvertline,  /* DrawVertLine subdriver*/
    synth_fillrect,      /* FillRect subdriver*/
    gen_fonts,
    synth_blit,          /* Blit subdriver*/
    synth_preselect,	 /* PreSelect*/
    NULL,                /* DrawArea subdriver*/
    NULL,	         /* SetIOPermissions*/
    gen_allocatememgc,
    synth_mapmemgc,
    gen_freememgc,
    NULL,                /* StretchBlit subdriver*/
    NULL	         /* SetPortrait*/
};

SUBDRIVER synth_subdriver = {
    synth_init,
    synth_drawpixel,
    synth_readpixel,
    synth_drawhorizline,
    synth_drawvertline,
    synth_fillrect,
    synth_blit,
    synth_drawarea,
    synth_stretchblit,
    synth_preselect,	 /* PreSelect*/
};

/* Static variables*/
static int status;		/* 0=never inited, 1=once inited, 2=inited. */
//static synth_frame_format_t *frame_format;

/* Calc linelen and mmap size, return 0 on fail*/
static int synth_init(PSD psd)
{
    if (!psd->size)
    {
        psd->size = psd->yres * psd->xres * psd->bpp / 8;
        /* convert linelen from byte to pixel len for bpp 16, 24, 32*/
        psd->linelen = psd->xres;
    }

    return 1;
}

static cyg_fb_colour CMWtofb(cyg_fb* fb,MWPIXELVAL c)
{
	if (CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_8BPP_TRUE_332)
	{
		return cyg_fb_make_colour(fb, (c&0xe0)>>(3+2), (c&0x1c)>>2, (c&0x03));
    	}
    	else if (CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_16BPP_TRUE_555)
    	{
		return cyg_fb_make_colour(fb, (c&0x7c00)>>(5+5), (c&0x03e0)>>5, (c&0x001f));
    	}
    	else if(CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_16BPP_TRUE_565)
	{
		return cyg_fb_make_colour(fb, (c&0xf800)>>(6+5), (c&0x07e0)>>5, (c&0x001f));
	}
	else if(CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_32BPP_TRUE_0888)
	{
		return cyg_fb_make_colour(fb, (c&0xFF0000)>>(8+8), (c&0x00ff00)>>8, (c&0x0000ff));
	}
}

static MWPIXELVAL CFBtoMW(cyg_fb* fb,cyg_fb_colour fb_colour)
{
	cyg_ucount8 r,g,b;
	MWPIXELVAL mMWPIXELVAL=0;
	cyg_fb_break_colour(fb,fb_colour,&r, &g, &b);
	if (CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_8BPP_TRUE_332)
	{
		mMWPIXELVAL|=r;
		mMWPIXELVAL<<=3;
		mMWPIXELVAL|=g;
		mMWPIXELVAL<<=2;
		mMWPIXELVAL|=b;
		return mMWPIXELVAL;
    	}
    	else if (CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_16BPP_TRUE_555)
    	{
		mMWPIXELVAL|=r;
		mMWPIXELVAL<<=5;
		mMWPIXELVAL|=g;
		mMWPIXELVAL<<=5;
		mMWPIXELVAL|=b;
		return mMWPIXELVAL;
    	}
    	else if(CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_16BPP_TRUE_565)
	{
		mMWPIXELVAL|=r;
		mMWPIXELVAL<<=6;
		mMWPIXELVAL|=g;
		mMWPIXELVAL<<=5;
		mMWPIXELVAL|=b;
		return mMWPIXELVAL;
	}
	else if(CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_32BPP_TRUE_0888)
	{
		mMWPIXELVAL|=r;
		mMWPIXELVAL<<=8;
		mMWPIXELVAL|=g;
		mMWPIXELVAL<<=8;
		mMWPIXELVAL|=b;
 		return mMWPIXELVAL;
	}
	
}

/* Initialise the synth framebuffer */
static PSD synth_open(PSD psd)
{
    /* Get frame format details */
    cyg_fb_on(&FRAMEBUF);

    psd->xres = psd->xvirtres = CYG_FB_fb0_WIDTH;
    psd->yres = psd->yvirtres = CYG_FB_fb0_HEIGHT;
    psd->portrait = MWPORTRAIT_NONE;
    psd->planes = 1;  /* Should probably find out what this means */


    if (CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_8BPP_TRUE_332)
    {
        psd->bpp = 8;
        psd->ncolors = 0xFF + 1;
        psd->pixtype = MWPF_TRUECOLOR332;
    }
    else if (CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_16BPP_TRUE_555)
    {
        psd->bpp = 16;
        psd->ncolors = 0x7FFF + 1;
        psd->pixtype = MWPF_TRUECOLOR555;
    }
    else if (CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_16BPP_TRUE_565)
    {
        psd->bpp = 16;
        psd->ncolors = 0xFFFF + 1;
        psd->pixtype = MWPF_TRUECOLOR565;
    }
    else if (CYG_FB_fb0_FORMAT==CYG_FB_FORMAT_32BPP_TRUE_0888)
    {
        psd->bpp = 32;
        psd->ncolors = 0xFFFFFF + 1;
        psd->pixtype = MWPF_TRUECOLOR0888;
    }
    else
    {
        EPRINTF("Unsupported display type\n");
        goto fail;
    }

    psd->linelen = CYG_FB_fb0_WIDTH * psd->bpp / 8;;  /* What is linelen?  - linelen in bytes for now...*/
    psd->size = psd->xres * psd->yres * psd->bpp / 8;
    psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
    psd->addr = CYG_FB_fb0_BASE;  /* Test */
//    psd->addr = NULL;  /* We do not want MW to access the frame buffer directly */


    /* We always use our own subdriver */
    psd->orgsubdriver = &synth_subdriver;


    status = 2;
    return psd;	/* success*/

 fail:
    return NULL;
}


/* Close framebuffer*/
static void synth_close(PSD psd)
{
    EPRINTF("%s - NOT IMPLEMENTED\n", __FUNCTION__);
}


static void synth_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
    cyg_fb_write_pixel(&FRAMEBUF,x, y, CMWtofb(&FRAMEBUF,c));
}


static MWPIXELVAL synth_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
    return CFBtoMW(&FRAMEBUF,cyg_fb_read_pixel(&FRAMEBUF,x,y));
}


static void synth_drawhorizline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
    cyg_fb_write_hline(&FRAMEBUF,x1, y,x2-x1+1, CMWtofb(&FRAMEBUF,c));
}


static void synth_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
    cyg_fb_write_vline(&FRAMEBUF,x, y1, y2-y1+1, CMWtofb(&FRAMEBUF,c));

}


static void synth_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
    cyg_fb_fill_block(&FRAMEBUF,x1, y1, x2-x1+1, y2-y1+1, CMWtofb(&FRAMEBUF,c));
}


static void synth_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
                     PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
    if (op != 0)
    {
        diag_printf("synth_blit(): op = 0x%x not supported\n", op);
    }

    if (srcpsd->addr == CYG_FB_fb0_BASE && dstpsd->addr != CYG_FB_fb0_BASE)
    {
        cyg_fb_read_block(&FRAMEBUF, srcx, srcy, w, h, dstpsd->addr, 0,(w+1)*(srcpsd->bpp/8));

    }
    else if (srcpsd->addr != CYG_FB_fb0_BASE && dstpsd->addr == CYG_FB_fb0_BASE)
    {
        cyg_fb_write_block(&FRAMEBUF, dstx, dsty, w, h, srcpsd->addr, 0,(w+1)*(dstpsd->bpp/8));
    }
    else if (srcpsd->addr == CYG_FB_fb0_BASE && dstpsd->addr == CYG_FB_fb0_BASE)
    {
        cyg_fb_move_block(&FRAMEBUF, srcx, srcy, w, h, dstx, dsty);
    }
    else
    {
        diag_printf("synth_blit(): Error unsupported operation\n");
    }
}

static void synth_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
                            MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy,
                            MWCOORD srcw, MWCOORD srch, long op)
{
    diag_printf("synth_stretch_blit() not implemented\n");
}

/* perform pre-select() duties*/
static void synth_preselect(PSD psd)
{
    cyg_fb_synch(&FRAMEBUF,0);
}
static void synth_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
    psi->rows = psd->yvirtres;
    psi->cols = psd->xvirtres;
    psi->planes = psd->planes;
    psi->bpp = psd->bpp;
    psi->ncolors = psd->ncolors;
    psi->pixtype = psd->pixtype;
    psi->fonts = NUMBER_FONTS;
    psi->portrait = psd->portrait;
    psi->fbdriver = true;

    switch (psd->pixtype) {
    case MWPF_TRUECOLOR332:
        psi->rmask = 0xE0;
        psi->gmask = 0x1C;
        psi->bmask = 0x03;
        break;
    case MWPF_TRUECOLOR233:
        psi->rmask = 0x07;
        psi->gmask = 0x38;
        psi->bmask = 0xC0;
        break;
    case MWPF_TRUECOLOR555:
        psi->rmask = 0x7c00;
        psi->gmask = 0x03e0;
        psi->bmask = 0x001f;
        break;
    case MWPF_TRUECOLOR565:
        psi->rmask = 0xf800;
        psi->gmask = 0x07e0;
        psi->bmask = 0x001f;
        break;
    case MWPF_TRUECOLOR0888:
        psi->rmask = 0xFF0000;
        psi->gmask = 0x00FF00;
        psi->bmask = 0x0000FF;
        break;
    default:
        printf("%s - unsupported pixtype\n", __FUNCTION__);
        psi->rmask = 0xff;
        psi->gmask = 0xff;
        psi->bmask = 0xff;
        break;
    }

    /* Need to figure out better values possibly */
    psi->xdpcm = 24;    /* assumes screen width of 24 cm */
    psi->ydpcm = 18;    /* assumes screen height of 18 cm */
}

static void synth_drawarea(PSD psd, driver_gc_t *gc, int op)
{
    diag_printf("synth_drawarea() not implemented\n");
}


/*
 * Initialize memory device with passed parms,
 * select suitable framebuffer subdriver,
 * and set subdriver in memory device.
 */
MWBOOL synth_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
    int size,void *addr)
{
    PSUBDRIVER subdriver;

    /* initialize mem screen driver*/
    initmemgc(mempsd, w, h, planes, bpp, linelen, size, addr);

    subdriver = &synth_subdriver;

    /* set and initialize subdriver into mem screen driver*/
    if(!set_subdriver(mempsd, subdriver, TRUE))
    {
        diag_printf("set_subdriver() failed\n");
        return 0;
    }

    return 1;
}
