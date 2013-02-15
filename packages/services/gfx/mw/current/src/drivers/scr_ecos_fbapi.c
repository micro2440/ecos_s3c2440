/*
 * Copyright (c) 2008 Gabor Toeroek <tgabor84@gmail.com>
 *
 * Microwindows Screen Driver for eCos framebuffer API
 *
 * Function headers copied from scr_bios.c and modified
 *
 */

#include <pkgconf/system.h>
#include <pkgconf/microwindows.h>

#include <cyg/infra/diag.h>
#include <cyg/io/framebuf.h>
#include <cyg/io/framebufs/framebufs.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"



/* In genmem.c*/
MWBOOL  set_subdriver(PSD psd, PSUBDRIVER subdriver, MWBOOL init);
static int ecosfb_init(PSD psd);
static PSD  ecosfb_open(PSD psd);
static void ecosfb_close(PSD psd);
static void ecosfb_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void ecosfb_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void ecosfb_drawpixel(PSD psd,MWCOORD x,MWCOORD y,MWPIXELVAL c);
static MWPIXELVAL ecosfb_readpixel(PSD psd,MWCOORD x,MWCOORD y);
static void ecosfb_drawhorzline(PSD psd,MWCOORD x1,MWCOORD x2,MWCOORD y,
    MWPIXELVAL c);
static void ecosfb_drawvertline(PSD psd,MWCOORD x,MWCOORD y1,MWCOORD y2,
    MWPIXELVAL c);
static void ecosfb_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
    MWPIXELVAL c);

#ifdef CYGIMP_MICROWINDOWS_ECOSFB_HAVEBLIT
static MWBOOL ecosfb_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
    int linelen,int size,void *addr);
static void ecosfb_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
                     PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op);
static void ecosfb_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
                            MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy,
                            MWCOORD srcw, MWCOORD srch, long op);
static void ecosfb_drawarea(PSD psd, driver_gc_t *gc, int op);
static void ecosfb_preselect(PSD psd);
#else
static void NULL_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w,
    MWCOORD h, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op) {}
static PSD NULL_allocatememgc(PSD psd) { return NULL; }
static MWBOOL NULL_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
    int linelen,int size,void *addr) { return 0; }
static void NULL_freememgc(PSD mempsd) {}
#endif // defined(CYGIMP_MICROWINDOWS_ECOSFB_HAVEBLIT)


SCREENDEVICE    scrdev = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
    ecosfb_open,
    ecosfb_close,
    ecosfb_getscreeninfo,
    ecosfb_setpalette,
    ecosfb_drawpixel,
    ecosfb_readpixel,
    ecosfb_drawhorzline,
    ecosfb_drawvertline,
    ecosfb_fillrect,
    gen_fonts,
#if CYGIMP_MICROWINDOWS_ECOSFB_HAVEBLIT
    ecosfb_blit,    /* Blit*/
#else
    NULL_blit,        /* Blit*/
#endif
#if CYGIMP_MICROWINDOWS_ECOSFB_HAVEBLIT
    ecosfb_preselect, /* PreSelect*/
#else
    NULL,			  /* PreSelect*/
#endif
    NULL,            /* DrawArea*/
    NULL,            /* SetIOPermissions*/
#if CYGIMP_MICROWINDOWS_ECOSFB_HAVEBLIT
    gen_allocatememgc,
    ecosfb_mapmemgc,
    gen_freememgc
#else
    NULL_allocatememgc,
    NULL_mapmemgc,
    NULL_freememgc
#endif
};

SUBDRIVER ecosfb_subdriver = {
    ecosfb_init,
    ecosfb_drawpixel,
    ecosfb_readpixel,
	ecosfb_drawhorzline,
    ecosfb_drawvertline,
    ecosfb_fillrect,
#ifdef CYGIMP_MICROWINDOWS_ECOSFB_HAVEBLIT
    ecosfb_blit,
    ecosfb_drawarea,
    ecosfb_stretchblit,
    ecosfb_preselect,	 /* PreSelect*/
#endif
};

/* Calc linelen and mmap size, return 0 on fail*/
static int ecosfb_init(PSD psd)
{
    diag_printf("ecosfb_init(): Error unsupported operation\n");
    if (!psd->size)
    {
        psd->size = psd->yres * psd->xres * psd->bpp / 8;
        /* convert linelen from byte to pixel len for bpp 16, 24, 32*/
        psd->linelen = psd->xres * psd->bpp / 8;
    }

    return 1;
}

PSD  ecosfb_open(PSD psd) {
    CYG_FB_ON(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME);
    
    psd->xres = psd->xvirtres = CYG_FB_WIDTH(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME);
    psd->yres = psd->yvirtres = CYG_FB_HEIGHT(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME);
    
    psd->planes = 1;
    psd->bpp = CYG_FB_DEPTH(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME);
        /* line length in bytes for bpp 1,2,4,8*/
        /* line length in pixels for bpp 16, 24, 32*/
    psd->linelen = CYG_FB_STRIDE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME);
    if (psd->linelen >= 16) psd->linelen /= psd->bpp / 8;
    
    psd->size = CYG_FB_STRIDE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME)*CYG_FB_HEIGHT(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME);
#warning "Some paletted modes doesn't set the correct palette"
    switch(CYG_FB_FORMAT(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME)) {
        case CYG_FB_FORMAT_1BPP_MONO_0_BLACK:
        case CYG_FB_FORMAT_1BPP_MONO_0_WHITE:
        case CYG_FB_FORMAT_1BPP_PAL888:
                psd->ncolors = 2;
                psd->pixtype = MWPF_PALETTE;
                break;

        case CYG_FB_FORMAT_2BPP_GREYSCALE_0_BLACK:
        case CYG_FB_FORMAT_2BPP_GREYSCALE_0_WHITE:
        case CYG_FB_FORMAT_2BPP_PAL888:
                psd->ncolors = 4;
                psd->pixtype = MWPF_PALETTE;
                break;

        case CYG_FB_FORMAT_4BPP_GREYSCALE_0_BLACK:
        case CYG_FB_FORMAT_4BPP_GREYSCALE_0_WHITE:
#warning "Palette is set for VGA colors for 4bit greyscale"
        case CYG_FB_FORMAT_4BPP_PAL888:
                psd->ncolors = 16;
                psd->pixtype = MWPF_PALETTE;
                break;

        case CYG_FB_FORMAT_8BPP_PAL888:
                psd->ncolors = 256;
                psd->pixtype = MWPF_PALETTE;
                break;

        case CYG_FB_FORMAT_8BPP_TRUE_332:
                psd->ncolors = 256;
                psd->pixtype = MWPF_TRUECOLOR332;
                break;
    
        case CYG_FB_FORMAT_16BPP_TRUE_565:
                psd->ncolors = 65536;
                psd->pixtype = MWPF_TRUECOLOR565;
                break;

        case CYG_FB_FORMAT_16BPP_TRUE_555:
                psd->ncolors = 32768;
                psd->pixtype = MWPF_TRUECOLOR555;
                break;

        case CYG_FB_FORMAT_32BPP_TRUE_0888:
                psd->ncolors = 2 << 24;
                psd->pixtype = MWPF_TRUECOLOR0888;
                break;
    }
#if CYGIMP_MICROWINDOWS_ECOSFB_HAVEBLIT
    psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
#else
    psd->flags = PSF_SCREEN;
#endif
    psd->addr = CYG_FB_BASE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME);
	/* We always use our own subdriver */
    psd->orgsubdriver = &ecosfb_subdriver;


    
    return psd;
}

void ecosfb_close(PSD psd) {
    CYG_FB_OFF(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME);
}

void ecosfb_getscreeninfo(PSD psd, PMWSCREENINFO psi) {
    psi->rows = psd->yvirtres;
    psi->cols = psd->xvirtres;
    psi->planes = psd->planes;
    psi->bpp = psd->bpp;
    psi->ncolors = psd->ncolors;
    psi->pixtype = psd->pixtype;
    psi->fonts = NUMBER_FONTS;

    psi->xdpcm = psd->xres * 10 / CYGNUM_MICROWINDOWS_ECOSFB_XSIZE;    // Screen X size is provided in mm-s
    psi->ydpcm = psd->yres * 10 / CYGNUM_MICROWINDOWS_ECOSFB_YSIZE;    // Screen Y size is provided in mm-s

    psi->portrait = psd->portrait;
    psi->fbdriver = CYG_FB_FLAGS0(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME) & CYG_FB_FLAGS0_LINEAR_FRAMEBUFFER;
    
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
        break;
    }
}


void ecosfb_setpalette(PSD psd, int first, int count, MWPALENTRY *pal) {
#if (CYG_FB_FLAGS0(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME) & CYG_FB_FLAGS0_WRITEABLE_PALETTE)
    for(  ; count ; count--,first++,pal++ )
    {
        // Cant't call function with the whole array,
        // must call it entry-by-entry, because eCos palette is not padded to 4 bytes, but MW palette is padded
        // FIXME: there may be problems with endianness (eCos: byte 0 is red, byte 1 is green and byte 2 is blue)
        void CYG_FB_WRITE_PALETTE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME, cyg_ucount32 first, cyg_ucount32 1, const void* pal, CYG_FB_UPDATE_NOW);
    }
#endif
}



void ecosfb_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c) {
    CYG_FB_WRITE_PIXEL(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME, x, y, c);
}

MWPIXELVAL ecosfb_readpixel(PSD psd, MWCOORD x, MWCOORD y) {
    return CYG_FB_READ_PIXEL(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME, x, y);
}

void ecosfb_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c) {
    CYG_FB_WRITE_HLINE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME, x1, y, x2-x1+1, c);
}

void ecosfb_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c) {
    CYG_FB_WRITE_VLINE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME, x, y1, y2-y1+1, c);
}

void ecosfb_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c) {
    CYG_FB_FILL_BLOCK(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME, x1, y1, x2-x1+1, y2-y1+1, c);
}

#ifdef CYGIMP_MICROWINDOWS_ECOSFB_HAVEBLIT
static void ecosfb_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
                     PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
    if (op != 0)
    {
        diag_printf("ecosfb_blit(): op = 0x%x not supported\n", op);
    }

    if (srcpsd->addr == CYG_FB_BASE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME) && dstpsd->addr != CYG_FB_BASE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME))
    {
        CYG_FB_READ_BLOCK(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME, srcx, srcy, w, h, dstpsd->addr, 0,(w+1)*2);

    }
    else if (srcpsd->addr != CYG_FB_BASE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME) && dstpsd->addr == CYG_FB_BASE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME))
    {
        CYG_FB_WRITE_BLOCK(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME, dstx, dsty, w, h, srcpsd->addr, 0,(w+1)*2);
    }
    else if (srcpsd->addr == CYG_FB_BASE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME) && dstpsd->addr == CYG_FB_BASE(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME))
    {
        CYG_FB_MOVE_BLOCK(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME, srcx, srcy, w, h, dstx, dsty);
    }
    else
    {
        diag_printf("ecosfb_blit(): Error unsupported operation\n");
    }
}

static void ecosfb_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
                            MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy,
                            MWCOORD srcw, MWCOORD srch, long op)
{
    diag_printf("ecosfb_stretch_blit() not implemented\n");
}

/* perform pre-select() duties*/
static void ecosfb_preselect(PSD psd)
{
    cyg_fb_synch(CYGDAT_MICROWINDOWS_ECOSFB_DEVNAME,0);
}

static void ecosfb_drawarea(PSD psd, driver_gc_t *gc, int op)
{
    diag_printf("ecosfb_drawarea() not implemented\n");
}


/*
 * Initialize memory device with passed parms,
 * select suitable framebuffer subdriver,
 * and set subdriver in memory device.
 */
MWBOOL ecosfb_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
    int size,void *addr)
{
    PSUBDRIVER subdriver;

    /* initialize mem screen driver*/
    initmemgc(mempsd, w, h, planes, bpp, linelen, size, addr);

    subdriver = &ecosfb_subdriver;

    /* set and initialize subdriver into mem screen driver*/
    if(!set_subdriver(mempsd, subdriver, TRUE))
    {
        diag_printf("set_subdriver() failed\n");
        return 0;
    }

    return 1;
}
#endif