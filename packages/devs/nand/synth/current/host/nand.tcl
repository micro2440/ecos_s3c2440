# {{{  Banner                                                   

# ============================================================================
# 
#      nand.tcl
# 
#      Nand flash support for the eCos synthetic target I/O auxiliary
# 
# ============================================================================
# ####ECOSHOSTGPLCOPYRIGHTBEGIN####                                         
# -------------------------------------------                               
# This file is part of the eCos host tools.                                 
# Copyright (C) 2009 eCosCentric Ltd.
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
# ============================================================================
# #####DESCRIPTIONBEGIN####
# 
#  Author(s):   bartv
#  Contact(s):  bartv
#  Date:        2009/07/01
#  Version:     0.01
#  Description:
#      Host-side support for the synthetic nand driver.
# 
# ####DESCRIPTIONEND####
# ============================================================================

# }}}

# Overview.
#
# The host-side support for the synth nand driver is aimed primarily
# at controlling various debug features. These are set up during
# initialization. Once the driver is up and running there is no
# further interaction with the host-side.
#
# All required parameters for the target-side are controlled by
# CDL, including:
#
#    image filename
#    page size
#    spare (OOB) size
#    pages per block
#    block count
#
# This means the target-side can run in the absence of the I/O
# auxiliary. This module only extends the functionality, and
# only when --nanddebug is added to the command line.
#
# The desired facilities are:
#
# 1) image selection. The image file itself remains as per the
#    CDL setting, to maintain consistent behaviour when running
#    with or without the I/O auxiliary. There are now buttons
#    for deleting the current image, restoring the image from
#    a previous run, or saving the image.
#
# 2) enabling/disabling logging. Specifically we want to be able
#    to log
#        read calls
#        read calls plus the data read
#        write calls
#        write calls with the data written
#        erase calls
#        errors
#
#    Obviously it should be possible to specify the logfile, and
#    default settings should come from default.tdf. It is also a good
#    idea to impose some limits on the logfile size, e.g. n MB, and to
#    control the behaviour when a logfile overflows. Maximum flexibility
#    is achieved by allowing n (>=1) logfiles, with the option of
#    creating checkpoint images.
#
# 3) error injection. We want to be able to set the initial factory-bad
#    blocks when creating a new nand image. We also want to be able to
#    inject new bad blocks at run-time, using an appropriately flexible
#    .tdf syntax and GUI.
#
# For now only one nand device is supported here, but the protocol and
# target-side do support multiple devices. Basically that would require
# turning the settings from global to array entries.

namespace eval nand {

    variable    debug_enabled           0

    # This information comes from the target-side, and helps with the UI.
    variable    target_protocol_version 0
    variable    image_filename          ""
    variable    pagesize                0
    variable    sparesize               0
    variable    pages_per_block         0
    variable    number_of_blocks        0
    variable    number_of_partitions    0       ;# 0 indicates that manual partitioning is disabled
    array set   partitions              [list]  ;# (id,base) and (id,size)

    # These are the logging settings. They can come from the config file,
    # and are editable in the dialog's logging tab
    variable    log_read                0
    variable    log_READ                0
    variable    log_write               0
    variable    log_WRITE               0
    variable    log_erase               0
    variable    log_error               0
    variable    logfile                 ""
    variable    max_logfile_size        1024
    variable    max_logfile_unit        "K"
    variable    number_of_logfiles      1
    variable    generate_checkpoints    0

    # These are the bad-block injection settings. They can also come
    # from the config file, and are editable in the dialog's errors
    # tab.
    variable    factory_bad_blocks      [list]
    variable    MAX_INJECTIONS          8   ;# each for erase and write, giving 16
    array set   erase_injections        [list]
    array set   write_injections        [list]
    for { set i 0 } { $i < $nand::MAX_INJECTIONS } { incr i } {
        set erase_injections($i,enabled)        0
        set erase_injections($i,str)            ""
        set erase_injections($i,affects)        "c" ;# "c" for current block, "n" for specific block or page
        set erase_injections($i,number)         "0" ;#
        set erase_injections($i,after_type)     "e" ;# "r" for rand() %, "e" for exactly
        set erase_injections($i,after_count)    "1" ;#
        set erase_injections($i,after_which)    "E" ;# "E" for any erase, "W" for any write, "C" for any call, "e" for erase block, "w" for write page
        set erase_injections($i,repeat)         "o" ;# "o" for once, "r" for repeat
        set write_injections($i,enabled)        0
        set write_injections($i,str)            ""
        set write_injections($i,affects)        "c" ;# "c" for current block, "p" for specific page
        set write_injections($i,number)         "0" ;#
        set write_injections($i,after_type)     "e" ;# "r" for rand() %, "e" for exactly
        set write_injections($i,after_count)    "1" ;#
        set write_injections($i,after_which)    "W" ;# "E" for any erase, "W" for any write, "C" for any call, "e" for erase block, "w" for write page
        set write_injections($i,repeat)         "o" ;# "o" for once, "r" for repeat
    }

    # A couple of utilities to help with parsing .tdf inject options
    proc inject_get_next_word { } {
        upvar 1 idx     idx_l
        upvar 1 option  option_l

        if { $idx_l >= [llength $option_l] } {
            synth::report_warning "nand device, incomplete \"inject\" entry in target definition file $synth::target_definition\n   \
                                           \"[join $option_l]\"\n"
            return -code continue
        }
        set result [string tolower [lindex $option_l $idx_l]]
        incr   idx_l
        return $result
    }
    
    proc report_bogus_inject { text } {
        upvar 1 option option_l
        synth::report_warning "nand device, invalid \"inject\" entry in target definition file $synth::target_definition\n   \
                                           \"inject [join $option_l]\"\n   \
                                           $text"
        return -code continue
    }
    
    proc instantiate { id name data } {
        # NOTE: the argument will remain unconsumed until the target-side
        # switches to initialization via a static constructor.
        if { [synth::argv_defined "-nanddebug"] } {
            set nand::debug_enabled 1
        }
        # Pick up logging configuration options from default.tdf
        set tmp [synth::tdf_get_option "nand" "logfile"]
        if { 0 == [llength $tmp] } {
            # Option not specified, the default will be based on the image filename
        } elseif { 1 == [llength $tmp] } {
            set nand::logfile $tmp
        } else {
            synth::report_error "nand device, invalid \"logfile\" entry in target definition file $synth::target_definition\n   \
                                 This entry should be a single string, a filename.\n"
        }

        foreach entry [synth::tdf_get_option "nand" "log"] {
            switch -exact -- $entry {
                "read" -
                "reads" {
                    set nand::log_read 1
                }
                "READ" -
                "READS" {
                    set nand::log_READ 1
                }
                "write" -
                "writes" {
                    set nand::log_write 1
                }
                "WRITE" -
                "WRITES" {
                    set nand::log_WRITE 1
                }
                "erase" -
                "erases" {
                    set nand::log_erase 1
                }
                "error" -
                "errors" {
                    set nand::log_error 1
                }
                default {
                    synth::report_warning "nand device, invalid \"log\" entry in target definition file $synth::target_definition\n   \
                                           Unknown value $entry.\n   \
                                           Valid values are read READ write WRITE erase error\n"
                }
            }
        }
        set tmp [synth::tdf_get_option "nand" "max_logfile_size"]
        if { 0 != [llength $tmp] } {
            set ok 1
            if { 1 != [llength $tmp] } {
                set ok 0
            } else {
                set tmp     [lindex $tmp 0]
                set value   ""
                set unit    ""
                if { ! [regexp -- {^(\d*)([kKgGmM])$} $tmp junk value unit] } {
                    set ok 0
                }
                # Only digits are allowed in the regexp, so we know we have a number.
                # Round up 0.
                if { 0 == $value } {
                    set value 1
                    synth::report_warning "nand device, invalid \"max_logfile_size\" entry in target definition file $synth::target_definition\n   \
                                           Rounding size up to 1.\n"
                }
                switch -- $unit {
                    "k" { set unit "K" }
                    "g" { set unit "G" }
                    "m" { set unit "M" }
                }
                set nand::max_logfile_size  $value
                set nand::max_logfile_unit  $unit
            }
            if { ! $ok } {
                synth::report_warning "nand device, invalid \"max_logfile_size\" entry in target definition file $synth::target_definition\n   \
                                       Value should be a single number followed by a unit K, M or G.\n"
            }
        }

        set tmp [synth::tdf_get_option "nand" "number_of_logfiles"]
        if { 0 != [llength $tmp] } {
            set ok 1
            if { 1 != [llength $tmp] } {
                set ok 0
            } else {
                set count [lindex $tmp 0]
                if { ! [string is integer -strict $count] } {
                    set ok 0
                } else {
                    if { $count < 1 } {
                        synth::report_warning "nand device, invalid \"number_of_logfiles\" entry in target definition file $synth::target_definition\n   \
                                               Value should be at least 1.\n"
                        set count 1
                    }
                    set nand::number_of_logfiles $count
                }
            }
            if { ! $ok } {
                synth::report_warning "nand device, invalid \"number_of_logfiles\" entry in target definition file $synth::target_definition\n   \
                                       Value should be a single number.\n"
            }
        }

        if { [synth::tdf_has_option "nand" "generate_checkpoint_images"] } {
            set nand::generate_checkpoints 1
        }

        set tmp [synth::tdf_get_option "nand" "factory_bad"]
        if { 0 != [llength $tmp] } {
            foreach entry $tmp {
                if { ![string is integer -strict $entry] } {
                    synth::report_warning "nand device, invalid \"factory_bad\" entry in target definition file $synth::target_definition\n   \
                                           All entries should be simple integers.\n"
                } elseif { $entry < 0 } {
                    synth::report_warning "nand device, invalid \"factory_bad\" entry in target definition file $synth::target_definition\n   \
                                           All entries should be positive integers.\n"
                } else {
                    lappend nand::factory_bad_blocks $entry
                }
            }
        }

        # Parse the inject filters. Some of this could be done by a regexp, but
        # diagnostics are easier if the parsing is done a word at a time.
        set erase_idx   0
        set write_idx   0
        foreach option [synth::tdf_get_options "nand" "inject"] {
            set erase           0
            set affects         ""
            set number          0
            set after_type      ""
            set after_count     0
            set after_which     ""
            set repeat          "o"
            set disabled        0
            
            set idx     0
            set word [inject_get_next_word]
            switch -- $word {
                "erase" {
                    set erase   1
                }
                "write" {
                    set erase   0
                }
                default {
                    report_bogus_inject "inject should be followed by erase or write, not \"$word\"\n"
                }
            }
            set word [inject_get_next_word]
            switch -- $word {
                "current" {
                    set affects "c"
                }
                "page" {
                    if { $erase } {
                        report_bogus_inject "page can only be specified after write.\n"
                    }
                    set affects "n"
                }
                "block" {
                    if { ! $erase } {
                        report_bogus_inject "block can only be specified after erase.\n"
                        continue
                    }
                    set affects "n"
                }
                default {
                    report_bogus_inject "write/erase should be followed by current/page/block, not \"$word\"\n"
                }
            }
            if { $affects == "n" } {
                set number [inject_get_next_word]
                if { ! [string is integer -strict $number] } {
                    report_bogus_inject "page/block should be followed by a number.\n"
                }
                if { $number < 0 } {
                    report_bogus_inject "page and block numbers should be positive integers.\n"
                }
            }
            set word [inject_get_next_word]
            switch -- $word {
                "after"  -
                "during" {
                    # Syntactic sugar, just ignore these.
                    set word [inject_get_next_word]
                }
            }

            if { [string equal $word "rand%"] } {
                set after_type "r"
                set word [inject_get_next_word]
            } else {
                set after_type "e"
            }
            if { ! [string is integer -strict $word] } {
                report_bogus_inject "event count \"$word\" should be an integer or rand% integer.\n"
            }
            if { $word < 0 } {
                report_bogus_inject "event count \"$word\" should be >= 0.\n"
            }
            set after_count $word

            set word [inject_get_next_word]
            switch -regexp -- $word {
                "^erases$" {
                    set after_which "E"
                }
                "^writes$" {
                    set after_which "W"
                }
                "^calls$" {
                    set after_which "C"
                }
                "^block_?erases$" {
                    set after_which "e"
                }
                "^page_?writes$" {
                    set after_which "w"
                }
                default {
                    report_bogus_inject "Invalid event type \"$word\"\n    Should be erases, writes, calls, block_erases or page_writes\n"
                }
            }
            if { (($after_which == "e") || ($after_which == "w")) && ($affects != "n") } {
                report_bogus_inject "Event types block_erases and page_writes cannot be combined with \"current\"\n"
            }

            if { $idx != [llength  $option] } {
                set word [inject_get_next_word]
                if { [string equal $word "repeat"] } {
                    if { $affects != "c" } {
                        report_bogus_inject "repeat can only be used with current, not with a specific block or page.\n"
                    } else {
                        set repeat "r"
                    }
                    if { $idx != [llength $option] } {
                        set word [inject_get_next_word]
                    } else {
                        set word ""
                    }
                }
                if { [string equal $word "disabled"] } {
                    set disabled 1
                    if { $idx != [llength $option] } {
                        set word [inject_get_next_word]
                    } else {
                        set word ""
                    }
                }
                if { ! [string equal $word ""] } {
                    report_bogus_inject "unexpected data \"$word\" at end of inject pattern.\n"
                }
            }

            # puts "Parsed filter $option"
            # puts " Erase $erase, affects $affects, number $number"
            # puts " after_type $after_type, after_count $after_count after_which $after_which"
            # puts " repeat $repeat disabled $disabled"

            if { $erase } {
                if { $erase_idx >= $nand::MAX_INJECTIONS } {
                    report_bogus_inject "Too many erase patters, the limit is $nand::MAX_INJECTIONS.\n"
                }
                set nand::erase_injections($erase_idx,enabled)      [expr !$disabled]
                set nand::erase_injections($erase_idx,affects)      $affects
                set nand::erase_injections($erase_idx,number)       $number
                set nand::erase_injections($erase_idx,after_type)   $after_type
                set nand::erase_injections($erase_idx,after_count)  $after_count
                set nand::erase_injections($erase_idx,after_which)  $after_which
                set nand::erase_injections($erase_idx,repeat)       $repeat
                set nand::erase_injections($erase_idx,str)          [join $option]
                incr erase_idx
            } else {
                if { $write_idx >= $nand::MAX_INJECTIONS } {
                    report_bogus_inject "Too many write patters, the limit is $nand::MAX_INJECTIONS.\n"
                }
                set nand::write_injections($write_idx,enabled) [expr !$disabled]
                set nand::write_injections($write_idx,affects)      $affects
                set nand::write_injections($write_idx,number)       $number
                set nand::write_injections($write_idx,after_type)   $after_type
                set nand::write_injections($write_idx,after_count)  $after_count
                set nand::write_injections($write_idx,after_which)  $after_which
                set nand::write_injections($write_idx,repeat)       $repeat
                set nand::write_injections($write_idx,str)          [join $option]
                incr write_idx
            }
        }
        return nand::handle_request
    }

    proc handle_request { id reqcode arg1 arg2 reqdata reqlen reply_len } {

        switch -- $reqcode {
            1 {
                # SET_PARAMS
                if { ! [regexp -- {^(\d*),.*} $reqdata junk protocol] } {
                    synth::report_warning "nand: invalid SET_PARAMS request from target.\nHost-side nand support disabled.\n"
                } else {
                    if { 1 != $protocol } {
                        synth::report_warning "nand: target uses unrecognised protocol $protocol\nHost-side nand support disabled.\n"
                    } else {
                        if { ! [regexp -- {^(\d*),(\d*),(\d*),(\d*),(\d*),(.*)$} $reqdata junk \
                                nand::target_protocol_version nand::pagesize \
                                nand::sparesize nand::pages_per_block \
                                nand::number_of_blocks nand::image_filename]
                         } {
                            synth::report_warning "nand: invalid SET_PARAMS request from target.\nHost-side nand support disabled.\n"
                            set nand::target_protocol_version 0
                        } else {
                            if { "" == $nand::logfile } {
                                set nand::logfile "[set nand::image_filename].log"
                            }
                        }
                    }
                }
                # We can now do some more sanity checks of the target definition file entries.
                foreach block $nand::factory_bad_blocks {
                    if { $block >= $nand::number_of_blocks } {
                        synth::report_warning "nand device, invalid \"factory_bad\" entry in target definition file $synth::target_definition\n   \
                                               Block $block has been listed, but valid values are from 0 to [expr $nand::number_of_blocks]\n"
                    }
                }
                for { set i 0 } { $i < $nand::MAX_INJECTIONS } { incr i } {
                    if { $nand::erase_injections($i,enabled) && ($nand::erase_injections($i,affects) == "n") } {
                        if { $nand::erase_injections($i,number) >= $nand::number_of_blocks } {
                            synth::report_warning "nand device, invalid \"inject\" entry in target definition file $synth::target_definition\n   \
                                                   \"$nand::erase_injections($i,str)\"\n   \
                                                   Blocks should be between 0 and [expr $nand::number_of_blocks - 1]\n"
                            set nand::erase_injections($i,enabled) 0
                        }
                    }
                    if { $nand::write_injections($i,enabled) && ($nand::write_injections($i,affects) == "n") } {
                        if { $nand::write_injections($i,number) >= ($nand::number_of_blocks * $nand::pages_per_block) } {
                            synth::report_warning "nand device, invalid \"inject\" entry in target definition file $synth::target_definition\n   \
                                                   \"$nand::write_injections($i,str)\"\n   \
                                                   Pages should be between 0 and [expr ($nand::number_of_blocks * $nand::pages_per_block) - 1]\n"
                            set nand::write_injections($i,enabled) 0
                        }
                    }
                }
            }
            2 {
                # SET_PARTITIONS
                if { 1 == $nand::target_protocol_version } {
                    if { ! [regexp -- {^(\d*),(\d*)(?:,(\d*),(\d*))?(?:,(\d*),(\d*))?(?:,(\d*),(\d*))?} \
                                $reqdata junk \
                                nand::partitions(0,base) nand::partitions(0,size) \
                                nand::partitions(1,base) nand::partitions(1,size) \
                                nand::partitions(2,base) nand::partitions(2,size) \
                                nand::partitions(3,base) nand::partitions(3,size)] } {
                        synth::report_warning "nand: invalid SET_PARTITIONS request from target.\n"
                    } else {
                        if { [string is integer -strict $nand::partitions(3,size)] } {
                            set nand::number_of_partitions  4
                        } elseif { [string is integer -strict $nand::partitions(2,size)] } {
                            set nand::number_of_partitions  3
                        } elseif { [string is integer -strict $nand::partitions(1,size)] } {
                            set nand::number_of_partitions  2
                        } else {
                            set nand::number_of_partitions   1
                        }
                    }
                }
            }
            3 {
                # DIALOG
                if { (1 == $nand::target_protocol_version) && $nand::debug_enabled && $synth::flag_gui } {
                    nand::dialog
                }
                synth::send_reply 0
            }
            4 {
                # GET_LOGFILE_SETTINGS
                if { 1 != $nand::target_protocol_version } {
                    synth::send_reply
                    return
                }
                set size [expr $nand::max_logfile_size * 1024]
                switch -- $nand::max_logfile_unit {
                    "M" {
                        set size [expr $size * 1024]
                    }
                    "G" {
                        set size [expr $size * 1024 * 1024]
                    }
                }
                set reply [format "%d%d%d%d%d%d%d%d,%d,%s" \
                               $nand::log_read $nand::log_READ $nand::log_write $nand::log_WRITE \
                               $nand::log_erase $nand::log_error \
                               $nand::generate_checkpoints \
                               $size \
                               $nand::number_of_logfiles \
                               $nand::logfile]
                if { [string length $reply] > $arg1 } {
                    synth::send_reply 0
                } else {
                    synth::send_reply 1 [string length $reply] $reply
                }
            }
            5 {
                # GET_FACTORY_BADS
                if { 1 != $nand::target_protocol_version } {
                    synth::send_reply
                    return
                }
                set reply ""
                foreach block $nand::factory_bad_blocks {
                    if { ($block >= 0) && ($block < $nand::number_of_blocks) } {
                        append reply [format "%d" $block] ","
                    }
                }
                set len [string length $reply]
                if {[string length $reply] > $arg1 } {
                    synth::send_reply 0
                } elseif {[string length $reply] > 0} {
                    synth::send_reply 1 [expr $len - 1] [string range $reply 0 end-1]
                } else {
                    synth::send_reply 1
                }
            }
            6 {
                # GET_BAD_BLOCK_INJECTIONS
                set reply ""
                for { set i 0 } { $i < $nand::MAX_INJECTIONS } { incr i } {
                    if { $nand::erase_injections($i,enabled) } {
                        append reply [format "E%s%d%s%d%s%s" \
                                          $nand::erase_injections($i,affects)     \
                                          $nand::erase_injections($i,number)      \
                                          $nand::erase_injections($i,after_type)  \
                                          $nand::erase_injections($i,after_count) \
                                          $nand::erase_injections($i,after_which) \
                                          $nand::erase_injections($i,repeat) ]
                    }
                    if { $nand::write_injections($i,enabled) } {
                        append reply [format "W%s%d%s%d%s%s" \
                                          $nand::write_injections($i,affects)     \
                                          $nand::write_injections($i,number)      \
                                          $nand::write_injections($i,after_type)  \
                                          $nand::write_injections($i,after_count) \
                                          $nand::write_injections($i,after_which) \
                                          $nand::write_injections($i,repeat) ]
                    }
                }
                if { [string length $reply] > $arg1 } {
                    synth::send_reply 0
                } else {
                    synth::send_reply 1 [string length $reply] $reply
                }
            }
        }
    }

    # ----------------------------------------------------------------------------
    # ----------------------------------------------------------------------------
    # The GUI dialog on startup.
    
    variable    dialog_done         0
    variable    restore_filename    ""
    variable    save_filename       ""

    proc dialog_exit { } {
        set nand::dialog_done   1
    }
    
    proc dialog_select_tab { widget new } {
        set current [lindex [pack slaves .nand_dialog.notebook] 0]
        if { $new != $current } {
            pack forget $current
            pack $new -side top -fill x
            foreach child [pack slaves .nand_dialog.buttons] {
                $child configure -relief sunken
            }
            $widget configure -relief raised
        }
    }

    proc log { message } {
        if { ! $nand::dialog_done } {
            .nand_dialog.log.text insert end $message
            .nand_dialog.log.text see end
        }
    }

    # ----------------------------------------------------------------------------
    # Enable/disable the various buttons in the files tab
    proc dialog_check_files { first check } {
        set frm .nand_dialog.notebook.files.buttons
        # Only enable the delete button if the image file exists.
        if { [file exists $nand::image_filename] } {
            if { $first } {
                nand::log "Checking for existing image file... yes\n"
            }
            if { ! [file isfile $nand::image_filename] } {
                nand::log "Error: $nand::image_filename is not a file."
            }
            $frm.delete  configure -state normal
            $frm.save    configure -state normal
            $frm.save_as configure -state normal
        } else {
            if { $first } {
                nand::log "Checking for existing image file... no\n"
            }
            $frm.delete  configure -state disabled
            $frm.save    configure -state disabled
            $frm.save_as configure -state disabled
        }
        # The save button can never be enabled/disabled, but we do want
        # to check that it is a file.
        if { [file exists $nand::save_filename] && ![file isfile $nand::save_filename] } {
            nand::log "Error: $nand::save_filename is not a file.\n"
            return
        }
       # The restore button should depend on the existence of an archive file.
        if { [file exists $nand::restore_filename] } {
            if { $first } {
                nand::log "Checking for existing archive file... yes\n"
            }
            if { ! [file isfile $nand::restore_filename] } {
                nand::log "Error: $nand::restore_filename is not a file."
            }
            $frm.restore configure -state normal
        } else {
            if { $first } {
                nand::log "Checking for existing archive file... no\n"
            }
            $frm.restore configure -state disabled
        }

        # FIXME: when a file is specified, we should also check the header
        # to ensure it is compatible with the current settings.
        if { $check } {
        }
    }

    proc dialog_delete { } {
        if { [catch { file delete -- $nand::image_filename } msg] } {
            nand::log "Error: failed to delete $nand::image_filename\n  $msg\n"
        } else {
            nand::log "File $nand::image_filename deleted.\n"
        }
        dialog_check_files 0 0
    }

    proc dialog_save { } {
        if { [catch { file copy -force -- $nand::image_filename $nand::save_filename } msg] } {
            nand::log "Error: failed to save $nand::image_filename as $nand::save_filename\n  $msg\n"
        } else {
            nand::log "File $nand::image_filename copied to $nand::save_filename\n"
        }
        dialog_check_files 0 0
    }

    proc dialog_save_as { } {
        set filename [tk_getSaveFile -parent .nand_dialog -title "Select new archive file"]
        if { "" != $filename } {
            # This version changes the current save image
            # set nand::save_filename $filename
            # nand::dialog_save

            # This version preserves the current save image
            if { [catch { file copy -force -- $nand::image_filename $filename } msg] } {
                nand::log "Error: failed to save $nand::image_filename as $filename\n  $msg\n"
            } else {
                nand::log "File $nand::image_filename copied to $filename\n"
            }
            dialog_check_files 0 0
        }
    }

    proc dialog_restore { } {
        if { [catch { file copy -force -- $nand::restore_filename $nand::image_filename } msg] } {
            nand::log "Error: failed to save $nand::restore_filename as $nand::image_filename\n  $msg\n"
        } else {
            nand::log "File $nand::restore_filename copied to $nand::image_filename\n"
        }
        dialog_check_files 0 1
    }

    proc dialog_restore_from { } {
        set filename [tk_getOpenFile -parent .nand_dialog -title "Select existing archive file"]
        if { "" != $filename } {
            # This version changes the current restore image
            # set nand::restore_filename $filename
            # nand::dialog_restore
            
            # This version preserves the current restore image
            if { [catch { file copy -force -- $filename $nand::image_filename } msg] } {
                nand::log "Error: failed to restore from $filename to $nand::image_filename\n  $msg\n"
            } else {
                nand::log "File $filename copied to $nand::image_filename\n"
            }
            dialog_check_files 0 1
        }
    }

    # ----------------------------------------------------------------------------
    # Support for the logging tab.
    #
    # This is used to enable/disable logfile-related widgets depending on
    # whether or not any of the logging options are enabled.
    variable dialog_logfile_widgets     [list]
    # These are used for the relevant spinboxes. There are problems with
    # validation and users clicking on OK without moving focus from a
    # spinbox, so copies of the relevant variables are needed here.
    variable dialog_max_logfile_size    ""
    variable dialog_max_logfile_unit    ""
    variable dialog_number_of_logfiles  ""

    proc dialog_check_logging_enabled { } {
        set enabled [expr $nand::log_read || $nand::log_READ || $nand::log_write || $nand::log_WRITE || $nand::log_erase || $nand::log_error]
        foreach widget $nand::dialog_logfile_widgets {
            if { $enabled } {
                $widget configure -state normal
            } else {
                $widget configure -state disabled
            }
        }
    }
    
    proc dialog_new_logfile { widget } {
        set filename [tk_getSaveFile -parent .nand_dialog -title "Select new logfile"]
        if { "" != $filename } {
            set nand::logfile   $filename
            $widget configure -text "Logfile: $nand::logfile"
            nand::log "New logfile is $nand::logfile"
        }
    }

    proc dialog_check_logfile_size { } {
        if { [string is integer -strict $nand::dialog_max_logfile_size] && ($nand::dialog_max_logfile_size > 0) } {
            set nand::max_logfile_size  $nand::dialog_max_logfile_size
            return 1
        } else {
            nand::log "Invalid value for logfile size, resetting to $nand::max_logfile_size\n"
            set nand::dialog_max_logfile_size $nand::max_logfile_size
            return 0
        }
    }
    proc dialog_check_logfile_unit { } {
        set current $nand::dialog_max_logfile_unit
        switch -- $current {
            "k" -
            "K" -
            "m" -
            "M" -
            "g" -
            "G" {
                set nand::max_logfile_unit [string toupper $current]
                return 1
            }
            default {
                nand::log "Invalid setting for logfile units, resetting to $nand::max_logfile_unit\n"
                set nand::dialog_max_logfile_unit $nand::max_logfile_unit
                return 0
            }
        }
    }

    proc dialog_check_logfile_count { } {
        if { [string is integer -strict $nand::dialog_number_of_logfiles] && ($nand::dialog_number_of_logfiles > 0) } {
            set nand::number_of_logfiles  $nand::dialog_number_of_logfiles
            return 1
        } else {
            nand::log "Invalid value for number of logfiles, resetting to $nand::number_of_logfiles\n"
            set nand::dialog_number_of_logfiles $nand::number_of_logfiles
            return 0
        }
    }
    
    # ----------------------------------------------------------------------------
    proc dialog { } {
        toplevel    .nand_dialog
        # Do not show anything until the sizes have been sorted out.
        wm withdraw .nand_dialog
        wm title    .nand_dialog "NAND driver settings"
        wm protocol .nand_dialog WM_DELETE_WINDOW nand::dialog_exit

        # A close button at the bottom
        frame       .nand_dialog.done
        button      .nand_dialog.done.ok -text "OK" -command nand::dialog_exit
        synth::register_balloon_help .nand_dialog.done.ok "Close this dialog and let the eCos application continue."
        pack .nand_dialog.done.ok -side bottom
        pack .nand_dialog.done    -side bottom -fill x

        # We also want a logging tab with scrollbar.
        frame       .nand_dialog.log
        text        .nand_dialog.log.text -height 5 -yscrollcommand { .nand_dialog.log.scroll set}
        scrollbar   .nand_dialog.log.scroll -command { .nand_dialog.log.text yview }
        pack        .nand_dialog.log.scroll -side right -fill y
        pack        .nand_dialog.log.text   -fill both -expand 1
        pack        .nand_dialog.log -side bottom -fill x
        
        # This should be a notebook widget, but for portability I
        # cannot assume the presence of ttk or bwidgets. So for now
        # just use a couple of buttons.
        frame       .nand_dialog.buttons
        button      .nand_dialog.buttons.files    -text "Files"   -relief raised \
            -command [list nand::dialog_select_tab .nand_dialog.buttons.files .nand_dialog.notebook.files]
        button      .nand_dialog.buttons.logging  -text "Logging" -relief sunken \
            -command [list nand::dialog_select_tab .nand_dialog.buttons.logging .nand_dialog.notebook.logging]
        button      .nand_dialog.buttons.errors   -text "Errors"  -relief sunken \
            -command [list nand::dialog_select_tab .nand_dialog.buttons.errors .nand_dialog.notebook.errors]
        pack        .nand_dialog.buttons.files .nand_dialog.buttons.logging .nand_dialog.buttons.errors -fill x -expand 1 -side left
        pack        .nand_dialog.buttons -side top -fill x
        synth::register_balloon_help .nand_dialog.buttons.files   "Select Files tab"
        synth::register_balloon_help .nand_dialog.buttons.logging "Select Logging tab"
        synth::register_balloon_help .nand_dialog.buttons.errors  "Select Errors tab"

        frame       .nand_dialog.notebook
        pack        .nand_dialog.notebook -side top -fill both -expand 1

        set nand::restore_filename  "[set nand::image_filename].bak"
        set nand::save_filename     $nand::restore_filename
        set frm [frame .nand_dialog.notebook.files]
        set txt [text $frm.text -height 5]
        $txt insert end "NAND image file: $nand::image_filename\n"
        $txt insert end "   Page size: $nand::pagesize\n"
        $txt insert end "   Spare (OOB) size: $nand::sparesize\n"
        $txt insert end "   Pages per erase block: $nand::pages_per_block\n"
        $txt insert end "   Number of erase blocks: $nand::number_of_blocks\n"
        pack $txt -side top -fill x
        frame $frm.separator -height 1 -background black
        pack  $frm.separator -side top -fill x
        set frm [frame $frm.buttons]
        button $frm.delete  -text "Delete"  -command [list nand::dialog_delete]
        synth::register_balloon_help $frm.delete "Delete the current NAND image file.\nA new, blank, image file will be created by the target-side driver."
        button $frm.save    -text "Save"    -command [list nand::dialog_save]
        synth::register_balloon_help $frm.save "Save the current NAND image file to the archive."
        button $frm.save_as -text "Save As ..." -command [list nand::dialog_save_as]
        synth::register_balloon_help $frm.save_as "Save the current NAND image file to a new archive."
        button $frm.restore -text "Restore" -command [list nand::dialog_restore]
        synth::register_balloon_help $frm.restore "Restore the NAND image from the archive."
        button $frm.restore_from  -text "Restore from ..." -command [list nand::dialog_restore_from]
        synth::register_balloon_help $frm.restore_from "Restore the NAND image from another archive."
        button $frm.refresh       -text "Refresh" -command [list nand::dialog_check_files 1 1]
        synth::register_balloon_help $frm.refresh "Check the filesystem again."
        label  $frm.image_file   -textvariable nand::image_filename -anchor w
        label  $frm.save_file    -textvariable nand::save_filename -anchor w
        label  $frm.restore_file -textvariable nand::restore_filename -anchor w
        grid configure $frm.delete       -row 0 -column 0 -sticky we
        grid configure $frm.save         -row 1 -column 0 -sticky we
        grid configure $frm.save_as      -row 2 -column 0 -sticky we
        grid configure $frm.restore      -row 3 -column 0 -sticky we
        grid configure $frm.restore_from -row 4 -column 0 -sticky we
        grid configure $frm.refresh      -row 5 -column 0 -sticky we
        grid configure $frm.image_file   -row 0 -column 1 -sticky we
        grid configure $frm.save_file    -row 1 -column 1 -sticky we
        grid configure $frm.restore_file -row 3 -column 1 -sticky we
        grid columnconfigure $frm 0 -weight 0
        grid columnconfigure $frm 1 -weight 1
        pack $frm -side top -fill x
        nand::dialog_check_files 1 1
 
        frame .nand_dialog.notebook.logging
        
        set frm [frame .nand_dialog.notebook.logging.events -borderwidth 2 -relief groove]
        pack $frm -fill both -expand 1 -pady 10 -padx 2
        label .nand_dialog.notebook.logging.events_title -text "Events to Log"
        place .nand_dialog.notebook.logging.events_title -in .nand_dialog.notebook.logging.events -relx 0 -x +10 -y -10 -bordermode outside
        set frm [frame $frm.contents -padx 10 -pady 10]
        checkbutton $frm.read  -variable nand::log_read  -command nand::dialog_check_logging_enabled -text "read"
        synth::register_balloon_help $frm.read "Enable/disable logging of read calls, without the data"
        checkbutton $frm.rEAD  -variable nand::log_READ  -command nand::dialog_check_logging_enabled -text "READ"
        synth::register_balloon_help $frm.rEAD "Enable/disable logging of read calls, with all the data"
        checkbutton $frm.write -variable nand::log_write -command nand::dialog_check_logging_enabled -text "write"
        synth::register_balloon_help $frm.write "Enable/disable logging of write calls, without the data"
        checkbutton $frm.wRITE -variable nand::log_WRITE -command nand::dialog_check_logging_enabled -text "WRITE"
        synth::register_balloon_help $frm.wRITE "Enable/disable logging of write calls, with all the data"
        checkbutton $frm.erase -variable nand::log_erase -command nand::dialog_check_logging_enabled -text "erase"
        synth::register_balloon_help $frm.erase "Enable/disable logging of erase calls"
        checkbutton $frm.error -variable nand::log_error -command nand::dialog_check_logging_enabled -text "error"
        synth::register_balloon_help $frm.error "Enable/disable logging of bad block errors"
        grid configure $frm.read  -row 0 -column 0 -sticky w -pady 2
        grid configure $frm.rEAD  -row 0 -column 1 -sticky w -pady 2
        grid configure $frm.write -row 1 -column 0 -sticky w -pady 2
        grid configure $frm.wRITE -row 1 -column 1 -sticky w -pady 2
        grid configure $frm.erase -row 2 -column 0 -sticky w -pady 2
        grid configure $frm.error -row 2 -column 1 -sticky w -pady 2
        pack $frm -side top -fill x

        set frm [frame .nand_dialog.notebook.logging.file -borderwidth 2 -relief groove]
        pack $frm -fill both -expand 1 -pady 10 -padx 2
        label .nand_dialog.notebook.logging.file_title -text "Logfile settings"
        place .nand_dialog.notebook.logging.file_title -in .nand_dialog.notebook.logging.file -relx 0 -x +10 -y -10 -bordermode outside
        set frm [frame $frm.contents -padx 10 -pady 10]
        label   $frm.name -anchor w -text "Logfile: $nand::logfile"
        button  $frm.new  -text "..." -command [list nand::dialog_new_logfile $frm.name]
        synth::register_balloon_help $frm.new "Change the initial logfile"
        label $frm.size_label         -text "Maximum logfile size" -anchor w
        label $frm.number_label       -text "Number of logfiles" -anchor w
        label $frm.checkpoint_label   -text "Generate checkpoint files" -anchor w
        set nand::dialog_max_logfile_size   $nand::max_logfile_size
        set nand::dialog_max_logfile_unit   $nand::max_logfile_unit
        set nand::dialog_number_of_logfiles $nand::number_of_logfiles
        spinbox $frm.size_entry -textvariable nand::dialog_max_logfile_size -from 1 -to 0x7fffffff \
            -width 4 -validate focus -invcmd bell \
            -validatecommand nand::dialog_check_logfile_size
        $frm.size_entry set $nand::max_logfile_size
        synth::register_balloon_help $frm.size_entry \
            "Set the maximum size of a logfile.\n\
             The action taken when this size is exceeded depends on the \"Number of logfiles\" setting."
        spinbox $frm.size_unit      -values { K M G } -textvariable nand::dialog_max_logfile_unit \
            -width 1 -validate focus -invcmd bell \
            -validatecommand nand::dialog_check_logfile_unit
        $frm.size_unit set $nand::max_logfile_unit
        synth::register_balloon_help $frm.size_unit "Specify the unit for logfile size. Valid values are K, M and G."
        spinbox $frm.number_entry -textvariable nand::dialog_number_of_logfiles -from 1 -to 0x7fffffff \
            -width 4 -validate focus -invcmd bell \
            -validatecommand nand::dialog_check_logfile_count
        $frm.number_entry set $nand::number_of_logfiles
        synth::register_balloon_help $frm.number_entry "Limit the number of logfiles that will be generated during a single testrun."
        # Without these extra bindings, there is a problem when the user edits
        # an entry but hits ok without moving the focus. If the changes are
        # valid then they will be lost.
        bind $frm.size_entry   <Destroy> +nand::dialog_check_logfile_size
        bind $frm.size_unit    <Destroy> +nand::dialog_check_logfile_unit
        bind $frm.number_entry <Destroy> +nand::dialog_check_logfile_count
        checkbutton $frm.checkpoint -variable nand::generate_checkpoints -anchor w
        set nand::dialog_logfile_widgets [list $frm.new $frm.size_entry $frm.size_unit $frm.number_entry $frm.checkpoint]
        nand::dialog_check_logging_enabled
        
        synth::register_balloon_help $frm.checkpoint \
            "Generate a checkpoint file at the start of a run and whenever a new logfile is created."
        grid configure $frm.name                -row 0 -column 0 -sticky w -pady 2
        grid configure $frm.size_label          -row 1 -column 0 -sticky w -pady 2
        grid configure $frm.number_label        -row 2 -column 0 -sticky w -pady 2
        grid configure $frm.checkpoint_label    -row 3 -column 0 -sticky w -pady 2
        grid configure $frm.new                 -row 0 -column 1 -sticky w -pady 2 -padx 5
        grid configure $frm.size_entry          -row 1 -column 1 -sticky w -pady 2 -padx 5
        grid configure $frm.size_unit           -row 1 -column 2 -sticky w -pady 2 -padx 5
        grid configure $frm.number_entry        -row 2 -column 1 -sticky w -pady 2 -padx 5
        grid configure $frm.checkpoint          -row 3 -column 1 -sticky w -pady 2 -padx 5
        pack $frm -side top -fill x
        
        frame       .nand_dialog.notebook.errors
        label       .nand_dialog.notebook.errors.message -anchor w -text "Error tab not yet implemented"
        pack        .nand_dialog.notebook.errors.message -side top -fill x

        # We now want to set the notebook width to the reqwidth of the larger 
        # of the .files, .logging or .errors tabs. Ditto for reqheight but
        # ignoring the errors tab because that is scrolled.
        update
        set width [winfo reqwidth .nand_dialog.notebook.files]
        if { [winfo reqwidth .nand_dialog.notebook.logging] >  $width } {
            set width [winfo reqwidth .nand_dialog.notebook.logging]
        }
        if { [winfo reqwidth .nand_dialog.notebook.errors] >  $width } {
            set width [winfo reqwidth .nand_dialog.notebook.errors]
        }
        set height [winfo reqheight .nand_dialog.notebook.files]
        if { [winfo reqheight .nand_dialog.notebook.logging] > $height } {
            set height [winfo reqheight .nand_dialog.notebook.logging]
        }
        .nand_dialog.notebook configure -width $width -height $height

        # Default to the files tab
        pack .nand_dialog.notebook.files -side top -fill both -expand 1

        # Size the dialog to prevent resizing when switching tabs.
        # Also centre the dialog.
        update
        set width  [winfo reqwidth  .nand_dialog]
        set height [winfo reqheight .nand_dialog]
        .nand_dialog configure -width $width
        .nand_dialog configure -height $height
        set x [expr ([winfo screenwidth  .] - $width) / 2]
        set y [expr ([winfo screenheight .] - $height) / 2]
        wm geometry .nand_dialog [set width]x[set height]+[set x]+[set y]
        wm transient .nand_dialog .
        wm deiconify .nand_dialog
        bind         .nand_dialog <KeyPress-Escape> nand::dialog_exit
        
        vwait nand::dialog_done
        destroy .nand_dialog
    }
}

return nand::instantiate