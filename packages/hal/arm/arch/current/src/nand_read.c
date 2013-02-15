/* 
 * vivi/s3c2410/nand_read.c: Simple NAND read functions for booting from NAND
 *
 * Copyright (C) 2002 MIZI Research, Inc.
 *
 * Author: Hwang, Chideok <hwang@mizi.com>
 * Date  : $Date: 2004/02/04 06:22:24 $
 *
 * $Revision: 1.1.1.1 $
 * $Id: param.c,v 1.9 2002/07/11 06:17:20 nandy Exp 
 *
 * History
 *
 * 2002-05-xx: Hwang, Chideok <hwang@mizi.com>
 *
 * 2002-05-xx: Chan Gyun Jeong <cgjeong@mizi.com>
 *
 * 2002-08-10: Yong-iL Joh <tolkien@mizi.com>
 *
 */

//#include <config.h>

#define NF_BASE        0x4E000000

#define NFCONF        (*(volatile unsigned int *)(NF_BASE + 0x00))
#define NFCONT        (*(volatile unsigned int *)(NF_BASE + 0x04))
#define NFCMD         (*(volatile unsigned char *)(NF_BASE + 0x08))
#define NFADDR        (*(volatile unsigned char *)(NF_BASE + 0x0C))
#define NFDATA        (*(volatile unsigned char *)(NF_BASE + 0x10))
#define NFSTAT        (*(volatile unsigned char *)(NF_BASE + 0x20))

//#define GPDAT        __REGi(GPIO_CTL_BASE+oGPIO_F+oGPIO_DAT)

#define NAND_CHIP_ENABLE  (NFCONT &= ~(1<<1))
#define NAND_CHIP_DISABLE (NFCONT |=  (1<<1))
#define NAND_CLEAR_RB      (NFSTAT |=  (1<<2))
#define NAND_DETECT_RB      { while(! (NFSTAT&(1<<2)) );}
/*
#define BUSY 4
inline void wait_idle(void) {
    while(!(NFSTAT & BUSY));
    NFSTAT |= BUSY;
}
*/
#define NAND_SECTOR_SIZE    2048
#define NAND_BLOCK_MASK        (NAND_SECTOR_SIZE - 1)

/* low level nand read function */
int
nand_read_ll(unsigned char *buf, unsigned long start_addr, int size)
{
    int i, j;

    if ((start_addr & NAND_BLOCK_MASK) || (size & NAND_BLOCK_MASK)) {
        return -1;    /* invalid alignment */
    }

    NAND_CHIP_ENABLE;

    for(i=start_addr; i < (start_addr + size);) {
        /* READ0 */
        NAND_CLEAR_RB;        
        NFCMD = 0;

        /* Write Address */
        //NFADDR = i & 0xff;
        //NFADDR = (i >> 9) & 0xff;
        //NFADDR = (i >> 17) & 0xff;
        //NFADDR = (i >> 25) & 0xff;
		NFADDR = 0;
		NFADDR = 0;
		NFADDR = (i >> 11) & 0xff;
		NFADDR = (i >> 19) & 0xff;
		NFADDR = (i >> 27) & 0xff;
		NFCMD = 0x30;
		NAND_DETECT_RB;

        for(j=0; j < NAND_SECTOR_SIZE; j++, i++) {
            *buf = (NFDATA & 0xff);
            buf++;
        }
    }
    NAND_CHIP_DISABLE;
    return 0;
}
