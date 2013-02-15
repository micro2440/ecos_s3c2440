#include <cyg/hal/s3c2440x.h> 
 
#define __REGb(x) (*(volatile unsigned char *)(x))
#define __REGi(x) (*(volatile unsigned int *)(x))

//-----------------------------------------------------------------
// Platform specific access to control-lines 
//-----------------------------------------------------------------
#define CYGHWR_FLASH_NAND_PLF_RESET nand_plf_reset
#define CYGHWR_FLASH_NAND_PLF_INIT nand_plf_init 
#define CYGHWR_FLASH_NAND_PLF_CE nand_plf_ce 
#define CYGHWR_FLASH_NAND_PLF_WP nand_plf_wp 
#define CYGHWR_FLASH_NAND_PLF_CMD nand_plf_cmd 
#define CYGHWR_FLASH_NAND_PLF_ADDR nand_plf_addr 
#define CYGHWR_FLASH_NAND_PLF_WAIT nand_plf_wait 
 
//----------------------------------------------------------------
// Global variables 
//-----------------------------------------------------------------
// The device-specific data 

static cyg_nand_dev nand_device = 
{ 
    .flash_base = (void*) (0x80000000), 
    .addr_r = (void*) NFDATA,
    .addr_w = (void*) NFDATA, 
     
    .delay_cmd = 1, 
    .delay_rst = 500, 
}; 

//-----------------------------------------------------------------
// Reset platform nand 
//-----------------------------------------------------------------
static inline void nand_plf_reset(void)
{
    int i;
        
    __REGi(NFCONF) &= ~(1<<11);
    __REGi(NFCMD) = 0xFF;               /* reset command */        
    for(i = 0; i < 10; i++);            /* tWB = 100ns. */
    while (!(__REGi(NFSTAT) & (1<<0))); /* wait 200~500us; */
    __REGi(NFCONF) |= (1<<11);
}
 
//-----------------------------------------------------------------
// Init platform nand 
//-----------------------------------------------------------------
static inline void nand_plf_init(void) 
{ 
#if 0
    // Init NAND Flash platform stuff 
#if 1
    #define TACLS 0
    #define TWRPH0 1
    #define TWRPH1 0
#else
    #define TACLS 0
    #define TWRPH0 4
    #define TWRPH1 2
#endif

    __REGi(NFCONF) = ((7<<12)|(7<<8)|(7<<4)|(0<<0) );
    /* 1 1 1 1, 1 xxx, r xxx, r xxx */
    /* En 512B 4step ECCR nFCE=H tACLS tWRPH0 tWRPH1 */

    nand_plf_reset();
#endif
} 
//-----------------------------------------------------------------
// Enable/disable nand chip 
//-----------------------------------------------------------------
static inline void nand_plf_ce(int state) 
{ 
    if(state) 
    { 
        // Enable CE line 
        __REGi(NFCONT) &= ~(1<<1);
    } 
    else 
    { 
        // Disable CE line 
        __REGi(NFCONT) |= (1<<1);
    } 
} 
 
//-----------------------------------------------------------------
// Enable/disable write protect 
//-----------------------------------------------------------------
static inline void nand_plf_wp(int nowrite) 
{ 
    if(nowrite) 
    { 
        // Enable WP 
    } 
    else 
    { 
        // Disable WP 
    } 
} 
//-----------------------------------------------------------------
// Write nand command 
//-----------------------------------------------------------------
static inline void nand_plf_cmd(int cmd) 
{ 
    // Enable CLE line 
    // Write command 
    // Disable CLE line 
    __REGi(NFCMD) = cmd;
} 
 
//-----------------------------------------------------------------
// Write nand address 
//-----------------------------------------------------------------
static inline void nand_plf_addr(int addr) 
{ 
    // Enable ALE line 
    // Write address 
    // Disable ALE line 
    __REGi(NFADDR)    = addr;
} 
//-----------------------------------------------------------------
// Wait device ready pin 
//-----------------------------------------------------------------
static inline void nand_plf_wait(void) 
{ 
    // Wait while device is not ready 
    while (!(__REGi(NFSTAT) & (1<<0)));
} 