# K42: (C) Copyright IBM Corp. 2000.
# All Rights Reserved
#
# This file is distributed under the GNU LGPL. You should have
# received a copy of the license along with K42; see the file LICENSE.html
# in the top-level directory for more details.
#
# $Id: khmon.tcl,v 1.5 2005/12/23 22:06:58 jappavoo Exp $

proc khmonSupport {} {
    global env

    if [info exists env(KHMON_LIBDIR)] {
        global khmonPath
        set khmonPath "$env(KHMON_LIBDIR)/tcl"
    	lappend khmonPath "$env(KHMON_LIBDIR)/tcl/extras"
    }

    if ![info exists khmonPath] {
        global khmonPath
        set khmonPath "."
    }
    lappend khmonPath ~/.khmon

#    puts "KHMON: khmonPath Initialized to : $khmonPath"

    proc khmonSource {file} {
        global khmonPath
        if {[file readable $file]} {
#            puts "KHMON: *** sourcing $file\n"
            source $file
        } else {
            # puts "KHMON: searching $khmonPath for $file\n"
            foreach p $khmonPath {
                # puts "KHMON: checking $p for $file"
                if [file readable "$p/$file"] {
 #                   puts "KHMON: found $file in $p"
                    return [source "$p/$file"]
                }
            }
        }
        # puts "KHMON: *** $file not found (khmonPath = $khmonPath)"
        return -1
    }
    proc khmonStartCmdLine {} {
	    if {[catch {package require tclreadline}]} {
			puts "WARNING: tclreadline not found in TCLLIBPATH."
			# Default very primitive shell w/ no readline support
			uplevel #0 {
				while {1} {
					puts -nonewline "khmon> "
					flush stdout
					gets stdin LINE
					eval $LINE
				}
			}
		} else {
			::tclreadline::Loop
		}
    }
}

proc help {procname} {
	global helpDB
	if [info exists helpDB($procname)] {
		puts $helpDB($procname)
	} else {
		puts "Help on $procname not found"
	}
}


proc processArgs {} {
    global objServer objServerUser objServerCmd stateDir objDir argc argv argv0
    if { ($argc != 2) && ($argc != 3) } {
        puts "usage: $argv0 <object server> <khmon state directory> \[object directory\]"
        exit -1
    }
    set objServer [lindex $argv 0]
    set stateDir [lindex $argv 1]
    if { $objServer == "LOCAL" } {
	set objDir [lindex $argv 2]
    } else {
       set objServerUser "khmon"
       set objServerCmd "/root/scripts/khmon/bin/khmonserver"
    }
} 

global env 
processArgs

khmonSupport

khmonSource obj.tcl
khmonSource gui.tcl

if { [openObjectStream] == 0 } {
    exit -1;
}

startGraphicsInterface
#khmonStartCmdLine
