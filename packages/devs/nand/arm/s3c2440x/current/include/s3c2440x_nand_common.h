#ifndef CYGONCE_S3C2440X_NAND_COMMON_H
#define CYGONCE_S3C2440X_NAND_COMMON_H
/*=============================================================================
//
//      s3c2440x26x_nand_common.h
//
//      NAND support common to the s3c2440x26x family
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
//#####DESCRIPTIONBEGIN####
//
// Author(s):   Ricky Wu (rickleaf.wu@gmail.com)
// Date:        2011-04-18
//
//####DESCRIPTIONEND####
//
//===========================================================================*/

// The OOB layout used on the AT91 by Linux (inter alia)
extern const cyg_nand_oob_layout s3c2440x_oob_largepage;
extern const cyg_nand_oob_layout s3c2440x_oob_smallpage;

// On-board hardware ECC:
extern cyg_nand_ecc_t s3c2440x_ecc;


#define S3C2440_NFCONT_INITECC		(1<<4)
#define S3C2440_NFCONT_MAINECCLOCK	(1<<5)


#endif // CYGONCE_S3C2440X_NAND_COMMON_H
