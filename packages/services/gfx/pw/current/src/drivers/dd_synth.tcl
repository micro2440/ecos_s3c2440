# ============================================================================
#
#      dd_synth.tcl
#
#      PW display support for the eCos synthetic target I/O auxiliary 
#
# ============================================================================
# ============================================================================
#
#  Copyright (C) 2004 Savin Zlobec.
#
#  This file is part of PW graphic library.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# ============================================================================
# ============================================================================
#
#  Author(s):     Savin Zlobec
#
# ============================================================================
    
namespace eval dd_synth {

    variable P_GETPARAMS   0x01
    variable P_GETKEY      0x02
    variable P_SETPIXELS   0x03
    variable P_RESETPIXELS 0x04
    variable P_SETMISC     0x05

    variable KC_UP    1 
    variable KC_DOWN  2 
    variable KC_LEFT  3 
    variable KC_RIGHT 4 
    variable KC_NEXT  5 
    variable KC_PREV  6 
    variable KC_YES   7 

    variable canvas

    variable key_intvec  -1
    variable key_list    [list] 
   
    variable lcd_canvas   
    variable lcd_pixels     
 
    variable lcd_width  160
    variable lcd_height 64 
    variable lcd_zoom   2 

    variable lcd_back_light_on 0
    
    proc key_ev { pressed keynum } {

        set llen [llength $dd_synth::key_list]
        lappend dd_synth::key_list $keynum $pressed
        synth::interrupt_raise $dd_synth::key_intvec
    }

    proc lcd_set_pixels { sdata slen } {

        set dlen [expr $slen >> 1]
        binary scan $sdata "s$dlen" data
        for {set i 0} {$i < $dlen} {incr i} {
            set addr  [lindex $data $i]
            set pixel [lindex $dd_synth::lcd_pixels $addr]
            $dd_synth::lcd_canvas itemconfigure $pixel \
                               -fill black -outline black 
        }
    }

    proc lcd_reset_pixels { sdata slen } {

        set dlen [expr $slen >> 1]
        binary scan $sdata "s$dlen" data
        for {set i 0} {$i < $dlen} {incr i} {
            set addr  [lindex $data $i]
            set pixel [lindex $dd_synth::lcd_pixels $addr]

            if { $dd_synth::lcd_back_light_on } {
                $dd_synth::lcd_canvas itemconfigure $pixel \
                    -fill SeaGreen1 -outline SeaGreen1 
            } else {
                $dd_synth::lcd_canvas itemconfigure $pixel \
                    -fill SeaGreen3 -outline SeaGreen3 
            }
        }
    }

    proc lcd_set_back_light { back_light_on } {
        
        if { $dd_synth::lcd_back_light_on != $back_light_on } {
            set dd_synth::lcd_back_light_on $back_light_on

            foreach pixel $dd_synth::lcd_pixels {
                set fill [$dd_synth::lcd_canvas itemcget $pixel -fill]
                if { $fill != "black" } {
                    if { $back_light_on } {
                        $dd_synth::lcd_canvas itemconfigure $pixel \
                            -fill SeaGreen1 -outline SeaGreen1 
                    } else {
                        $dd_synth::lcd_canvas itemconfigure $pixel \
                            -fill SeaGreen3 -outline SeaGreen3 
                    }
                }
            }
        }            
    }

    proc init_gui { } { 

        set w .hwsimulator
        catch {destroy $w} 
        toplevel $w
        wm title $w "PW synth display"
        wm protocol $w "WM_DELETE_WINDOW" synth::_handle_exit_request

        canvas $w.c -width 324 -height 132 
      
        set dd_synth::canvas $w.c

        canvas $w.c.dpy -bg black -relief sunken -borderwidth 2  \
            -width  [expr $dd_synth::lcd_width  * $dd_synth::lcd_zoom] \
            -height [expr $dd_synth::lcd_height * $dd_synth::lcd_zoom] 

        $w.c create window 0 0 -window $w.c.dpy -anchor nw
        
        for {set y 0} {$y < $dd_synth::lcd_height} {incr y} {
            for {set x 0} {$x < $dd_synth::lcd_width} {incr x} {
                set xc [expr $x * $dd_synth::lcd_zoom + 3]
                set yc [expr $y * $dd_synth::lcd_zoom + 3]
                lappend dd_synth::lcd_pixels                           \
                        [$w.c.dpy create rectangle $xc $yc             \
                                     [expr $xc + $dd_synth::lcd_zoom]  \
                                     [expr $yc + $dd_synth::lcd_zoom]  \
                                     -fill SeaGreen3 -outline SeaGreen3]        
            }
        }
        set dd_synth::lcd_canvas $w.c.dpy
       
        bind $w <KeyPress-Up>         { dd_synth::key_ev 1 $dd_synth::KC_UP }
        bind $w <KeyRelease-Up>       { dd_synth::key_ev 0 $dd_synth::KC_UP } 
        bind $w <KeyPress-Down>       { dd_synth::key_ev 1 $dd_synth::KC_DOWN }
        bind $w <KeyRelease-Down>     { dd_synth::key_ev 0 $dd_synth::KC_DOWN }
        bind $w <KeyPress-Left>       { dd_synth::key_ev 1 $dd_synth::KC_LEFT }
        bind $w <KeyRelease-Left>     { dd_synth::key_ev 0 $dd_synth::KC_LEFT }
        bind $w <KeyPress-Right>      { dd_synth::key_ev 1 $dd_synth::KC_RIGHT }
        bind $w <KeyRelease-Right>    { dd_synth::key_ev 0 $dd_synth::KC_RIGHT }
        bind $w <KeyPress-Return>     { dd_synth::key_ev 1 $dd_synth::KC_YES }
        bind $w <KeyRelease-Return>   { dd_synth::key_ev 0 $dd_synth::KC_YES }
        bind $w <KeyPress-Page_Up>    { dd_synth::key_ev 1 $dd_synth::KC_PREV }
        bind $w <KeyRelease-Page_Up>  { dd_synth::key_ev 0 $dd_synth::KC_PREV }
        bind $w <KeyPress-Page_Down>  { dd_synth::key_ev 1 $dd_synth::KC_NEXT }
        bind $w <KeyRelease-Page_Down> { dd_synth::key_ev 0 $dd_synth::KC_NEXT }

        pack $w.c
    }
 
    proc instantiate { id name data } {

        if { ! $synth::flag_gui } {
            synth::report_error "Cannot run dd_synth in non gui mode.\n"
            return ""
        }

        dd_synth::init_gui
  
        set dd_synth::key_intvec [synth::interrupt_allocate dd_synth]
        if { -1 == $dd_synth::key_intvec } { return "" }

        return dd_synth::handle_request
    }

    proc handle_request { id reqcode arg1 arg2 reqdata reqlen reply_len } {

        if { $reqcode == $dd_synth::P_GETPARAMS } {
            
            set replay [binary format "c" $dd_synth::key_intvec]
            synth::send_reply 0 1 $replay

        } elseif { $reqcode == $dd_synth::P_GETKEY } {

            set len [llength $dd_synth::key_list]
            if { $len != 0 } {
                set replay [binary format "cc" \
                    [lindex $dd_synth::key_list 0] \
                    [lindex $dd_synth::key_list 1]]
                set dd_synth::key_list [lrange $dd_synth::key_list 2 \
                                       [llength $dd_synth::key_list]]
                set len [llength $dd_synth::key_list]
                synth::send_reply $len 2 $replay 
            } else {
                set replay [binary format "cc" 0 0]
                synth::send_reply -1 2 $replay 
            }
            
        } elseif { $reqcode == $dd_synth::P_SETPIXELS} {
            
            dd_synth::lcd_set_pixels $reqdata $reqlen 
            
        } elseif { $reqcode == $dd_synth::P_RESETPIXELS} {
            
            dd_synth::lcd_reset_pixels $reqdata $reqlen
            
        } elseif { $reqcode == $dd_synth::P_SETMISC} {
            
            binary scan $reqdata "c" back_light_on
            dd_synth::lcd_set_back_light $back_light_on
            
        } else {
            synth::report_error "Received unexpected request $reqcode for dd_synth device"
        }
    }
   
    proc ecos_exited { arg_list } {
    }
    
    synth::hook_add "ecos_exit" dd_synth::ecos_exited
}

return dd_synth::instantiate
