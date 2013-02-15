## Process this file with automake to produce Makefile.in
## =====================================================================
##
##     Makefile.am
##
##     Build support for the host-side synthetic target support,
##     the nand driver package
##
## =====================================================================
# ####ECOSHOSTGPLCOPYRIGHTBEGIN####                                         
# -------------------------------------------                               
# This file is part of the eCos host tools.                                 
# Copyright (C) 2009 Free Software Foundation, Inc.                         
#
# This program is free software; you can redistribute it and/or modify      
# it under the terms of the GNU General Public License as published by      
# the Free Software Foundation; either version 2 or (at your option) any    
# later version.                                                            
#
# This program is distributed in the hope that it will be useful, but       
# WITHOUT ANY WARRANTY; without even the implied warranty of                
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         
# General Public License for more details.                                  
#
# You should have received a copy of the GNU General Public License         
# along with this program; if not, write to the                             
# Free Software Foundation, Inc., 51 Franklin Street,                       
# Fifth Floor, Boston, MA  02110-1301, USA.                                 
# -------------------------------------------                               
# ####ECOSHOSTGPLCOPYRIGHTEND####                                           
## =====================================================================
#######DESCRIPTIONBEGIN####
##
## Author(s):	bartv
## Contact(s):	bartv
## Date:	2009/07/01
## Version:	0.01
##
######DESCRIPTIONEND####
## =====================================================================

AUTOMAKE_OPTIONS = 1.10 foreign

## Only some platforms are supported. Having the configure script throw
## an error when attempting to configure on an unsupported platform
## would be a mistake, since that would prevent any configury from
## the toplevel on unsupported platforms. Instead an automake conditional
## is used, leading to null makefiles on unsupported platforms.

if SUPPORTED

## The only thing that needs to be installed is a Tcl script and some
## data files.
nanddir 		= $(libexecdir)/ecos/@PACKAGE_INSTALL@
nand_DATA		= nand.tcl \
                          nand.tdf
endif
