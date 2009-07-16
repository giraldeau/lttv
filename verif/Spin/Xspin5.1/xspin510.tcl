#!/bin/sh
# the next line restarts using wish \
exec wish c:/cygwin/bin/xspin -- $*

# cd	;# enable to cd to home directory by default

# on PCs:
# adjust the first argument to wish above with the name and
# location of this tcl/tk file on your system, if different.
#
# Cygwin:
# if you use cygwin, do not refer to the file as /usr/bin/xspin
# /usr/bin is a symbolic link to /bin, which really
# lives in c:/cygwin, hence the contortions above
#
# on Unix/Linux/Solaris systems
# replace the first line with something like
#	#!/usr/bin/wish -f
# using the pathname for the wish executable on your system

#======================================================================#
# Tcl/Tk Spin Controller, written by Gerard J. Holzmann, 1995-2005.    #
# See the README.html file for full installation notes.                #
#        http://spinroot.com/spin/whatispin.html                       #
#======================================================================#
set xversion "5.1.0 -- 24 April 2008"

# -- Xspin Installation Notes (see also README.html):

# 1. On Unix systems: change the first line of this file to point to the wish
#    executable you want to use (e.g., wish4.2 or /usr/local/bin/wish8.0)
#    ==> be careful, the pathname should be 30 characters or less
#
# 2. If you use another C compiler than gcc, change the set CC line below
#
# 3. Browse the configurable options below if you would like to
#    change or adjust the visual appearance of the GUI
#
# 4. If you run on a PC, and have an ancient version of tcl/tk,
#    you must set the values fo Unix, CMD, and Go32 by hand below
#    => with Tcl/Tk 7.5/4.1 or later, this happens automatically

# set CC   "cc -w -Wl -woff,84"	;# ANSI-C compiler, suppress warnings
# set CC   "cl -w -nologo"	;# Visual Studio C/C++ compiler, prefered on PCs
  set CC   "gcc -w"		;# standard gcc compiler - no warnings
  set CC0  "gcc"

# set CPP  "cpp"		;# the normal default C preprocessor
  set CPP  "gcc -E -x c"	;# c preprocessor, assuming we have gcc

  set SPIN "spin"	;# use a full path-name if necessary, e.g. c:/cygwin/bin/spin.exe
  set DOT  "dot"	;# optional, graph layout interface
  			;# no prob if dot is not available
  set BG   "white"	;# default background color for text
  set FG   "black"	;# default foreground color for text
  set CMD  ""		;# default is empty, and adjusted below
  set Unix 1		;# default is Unix, but this is adjusted below
  set Ptype "color"	;# printer-type: mono, color, or gray
  set NT 0		;# scratch variable, ignore

  set debug_on 0
  if {$debug_on} {
	toplevel .debug ;   #debugging window
	text .debug.txt -width 80 -height 60 -relief raised -bd 2 \
		-yscrollcommand ".debug.scroll set"
	scrollbar .debug.scroll -command ".debug.txt yview"
	pack .debug.scroll -side right -fill y
	pack .debug.txt -side left
  }
  proc debug {txt} {
	global debug_on
	if {$debug_on} {
	catch { .debug.txt insert end "\n$txt" }
  }	}

  if [info exists tcl_platform] {
	set sys $tcl_platform(platform)
#	if {$sys == "macintosh"} {
#		... no adjustments needed? ...
#	}
        if {[string match windows $sys]} {
		set Unix 0	;# means Windows95/98/2000/NT/XP

#		if {[auto_execok cl] != ""} {
#			set CC   "cl -w -nologo"	;# Visual Studio compiler, PCs
#			set CC0  "cl"
#		}

		if {$tcl_platform(os) == "Windows 95"} {
			set CMD  "command.com"	;# Windows95
		} else {
			set CMD  "cmd"
			set NT 1
  }	}	}

#-- GUI configuration options - by Leszek Holenderski <lhol@win.tue.nl>
#-- basic text size:
	set HelvBig	-Adobe-Helvetica-Medium-R-Normal--*-120-*-*-*-*-*-*
# mscs:
	if {$NT} {	;# on a windows nt machine
	set SmallFont	"-*-Courier-Bold-R-Normal--*-110-*"
	set BigFont	"-*-Courier-Bold-R-Normal--*-110-*"
	} else {
	set SmallFont	"-*-Courier-Bold-R-Normal--*-80-*"
	set BigFont	"-*-Courier-Bold-R-Normal--*-80-*"
	}

# Some visual aspects of Xspin GUI can be configured by the user.
# On PCs, the values of configuration options that are hard-coded
# into this script can be modified (see below). On Unix, in addition to
# (or rather, instead of) the manual modification, the values can be set in
# an Xspin resource file ($HOME/.xspinrc) and/or in the X11 default resource
# file (usually, $HOME/.Xdefaults).

# Since the same option can be specified in several places, options can have
# several, possibly inconsistent, values. The value which takes effect is
# determined by the order in which options are scanned. The values specified
# later in the order have higher priority. First, hard-coded options are
# scanned, then options specified in .Xdefaults, and finally options
# specified in .xspinrc.

# When setting configuration options in an .xspinrc file, convert the 
# settings below to the format of an X11 resource file. For example,
#
#	# width of scrollbars (any Tk dimension, default 15 pixels)
#	option add *Scrollbar.width	13	startupFile
#
# should be converted to
#
#	! width of scrollbars (any Tk dimension, default 15 pixels)
#	*Scrollbar.width	13
# In .Xdefaults file, configuration options should be preceded by the
# application's name, so the above option should be converted to
#
#	! width of scrollbars (any Tk dimension, default 15 pixels)
#	xspin*Scrollbar.width	13
# side on which side scrollbars are put (left or right, default=right)

option add *Scrollbar.side	left	startupFile

#--- sizes
	# width of scrollbars (any Tk dimension, default 15 pixels)
	option add *Scrollbar.width	13	startupFile
	# width of borders (in pixels, typically 1 or 2, default 2)
	option add *borderWidth		1	startupFile
	# initial width of the input/log area (in characters, default 80)
	option add *Input*Text.width	72	startupFile
	option add *Log*Text.width	72	startupFile
	# initial height of the input area (in lines, default 24)
	option add *Input*Text.height	20	startupFile
	# initial height of the log area (in lines, default 24)
	option add *Log*Text.height	5	startupFile
	# size of the handle used to adjust the height of the log area
	# (in pixels, default 0)
	option add *Handle.width	10	startupFile
	option add *Handle.height	10	startupFile
#--- colors
	# colors for the input/log area
	option add *Input*Text.background	white	startupFile
	option add *Input*Text.foreground	black	startupFile
	option add *Log*Text.background		gray95	startupFile
	option add *Log*Text.foreground		black	startupFile
	# colors for the editable/read-only area
	option add *Entry.background		white	startupFile
	option add *Edit*Text.background	white	startupFile
	option add *Edit*Text.foreground	black	startupFile
	# colors for the editable/read-only area
	option add *Read*Text.background	gray95	startupFile
	option add *Read*Text.foreground	black	startupFile
#--- fonts
	# fonts for the input/log area (default is system dependent,
	# usually -*-Courier-Medium-R-Normal--*-120-*)
option add *Input*Text.font	-*-Helvetica-Medium-R-Normal--*-120-*	startupFile
#option add *Input*Text.font	-schumacher-clean-medium-r-normal--*-120-*-60-*	startupFile
#option add *Log*Text.font	-schumacher-clean-medium-r-normal--*-120-*-60-*	startupFile

#--- widgets
	# simple texts (dialogs which present read-only texts, such as help)
	option add *SimpleText.Text.width 60
	option add *SimpleText.Text.height 30
	option add *SimpleText.Text.background white

	# sections (decorated frames for grouping related buttons)
	option add *Section*Title.font		-*-Helvetica-Bold-R-Normal--*-100-*	startupFile

#option add *Section*Checkbutton.font	-*-Helvetica-Medium-R-Normal--*-100-*	startupFile
#option add *Section*Radiobutton.font	-*-Helvetica-Medium-R-Normal--*-100-*	startupFile
#option add *Section*Label.font		-*-Helvetica-Medium-R-Normal--*-100-*	startupFile

################ end of configurable parameters #######################

wm title . "SPIN CONTROL $xversion"
wm iconname . "SPIN"
wm geometry . +41+50
wm minsize  . 400 200

set Fname ""
set firstime 1
set notignored 0

#### seed the advanced parameters settings

set e(0)	"Physical Memory Available (in Mbytes): "
set ival(0)	128
set expl(0)	"explain"

set e(1)	"Estimated State Space Size (states x 10^3): "
set ival(1)	500
set expl(1)	"explain"

set e(2)	"Maximum Search Depth (steps): "
set ival(2)	10000
set expl(2)	"explain"

set e(7)	"Nr of hash-functions in Bitstate mode: "
set ival(7)	2
set expl(7)	"explain"

set e(3)	"Extra Compile-Time Directives (Optional): "
set ival(3)	""
set expl(3)	"Choose"

set e(4)	"Extra Run-Time Options (Optional): "
set ival(4)	""
set expl(4)	"Choose"

set e(5)	"Extra Verifier Generation Options: "
set ival(5)	""
set expl(5)	"Choose"

set ival(6)	1	;# which error is reported, default is 1st


# allow no more than one entry per selection
catch { tk_listboxSingleSelect Listbox }

proc msg_file {f nowarn} {
	set msg ""
	set ef [open $f r]

	while {[gets $ef line] > -1} {
		if {$nowarn} {
			if {[string first "warning" $line] < 0} {
				set msg "$msg\n$line"
			}
		} else {
			set msg "$msg\n$line"
	}	}
	close $ef
	return $msg
}

	scan $tk_version "%d.%d" tk_major tk_minor

	set version ""

	if {[auto_execok $SPIN] == "" \
	||  [auto_execok $SPIN] == 0} {
		set version "Error: No executable $SPIN found..."
	} else {
		if {$Unix} {
			set version [exec $SPIN -V]
		} else {
			catch { exec $SPIN -V >&pan.tmp } err
			if {$err == ""} {
				set version [msg_file pan.tmp 1]
			} else {
				puts "error: $err"
				puts "is there a $SPIN and a go32.exe?"
				exit
		}	}
		if {[string first "Spin Version 5" $version] < 0 \
		&&  [string first "Spin Version 4" $version] < 0 \
		&&  [string first "Spin Version 3" $version] < 0 } {
			set version "Spin Version not recognized: $version"
		}
	}

frame .menu
	# top level menu bar
	menubutton .menu.file -text "File.." \
		-relief raised -menu .menu.file.m
	menubutton .menu.run -text "Run.." -fg red \
		-relief raised -menu .menu.run.m
	menubutton .menu.edit -text "Edit.." \
		-relief raised -menu .menu.edit.m
	menubutton .menu.view -text "View.." \
		-relief raised -menu .menu.view.m
	label .menu.title -text "SPIN  DESIGN  VERIFICATION" -fg blue

	set lno 1
	label .menu.lno -text "Line#:" -relief raised 
	entry .menu.ent -width 6 -textvariable lno \
		-relief sunken -background white -foreground black
	bind  .menu.ent <Return> {
		.inp.t tag remove hilite 0.0 end
		.inp.t tag add hilite $lno.0 $lno.1000
		.inp.t tag configure hilite -background $FG -foreground $BG
		.inp.t yview -pickplace [expr $lno-4]
	}

	label .menu.fnd1 -text "Find:" -relief raised 
	entry .menu.fnd2 -width 8 -textvariable pat \
		-relief sunken -background white -foreground black
	bind  .menu.fnd2 <Return> {
		.inp.t tag remove hilite 0.0 end
		forAllMatches .inp.t $pat
	}

	menubutton .menu.help -text "Help" -relief raised \
		-menu .menu.help.m

	pack append .menu \
		.menu.file {left frame w} \
		.menu.edit {left frame w} \
		.menu.view {left frame w} \
		.menu.run {left frame w} \
		.menu.help {left frame w} \
		.menu.title {left frame c expand} \
		.menu.fnd2 {right frame e} \
		.menu.fnd1 {right frame e} \
		.menu.ent {right frame e} \
		.menu.lno {right frame e}

set loglines 5
frame .log
	text .log.t -bd 2 -height $loglines -background $FG -foreground $BG
frame .log.b
	button .log.b.pl -text "+" \
		-command {incr loglines  1; .log.t configure -height $loglines}
	button .log.b.mn -text "-" \
		-command {incr loglines -1; .log.t configure -height $loglines}
	pack append .log.b .log.b.pl {top}
	pack append .log.b .log.b.mn {top}
	pack append .log .log.b {left filly}
	pack append .log .log.t {right expand fill}

proc dopaste {} {
	global FG BG
	set from [.inp.t index insert]
	tk_textPaste .inp.t
	set upto [.inp.t index insert]
	.inp.t tag add sel $from $upto
#	.inp.t tag configure hilite -background $FG -foreground $BG
}

#- fetch the value of user-defined configuration options

proc fetchOption {name default args} {

  set class Dummy
  set fullName $name

  # class encoded in name ?
  switch -glob -- $name *.* {
    set list [split $name .]
    switch [llength $list] 2 {} default { error "wrong option \"$name\" }
    set class [lindex $list 0]
    set name  [lindex $list 1]
  }

  # create a unique dummy frame of requested class and get the option's value
  set dummy .0
  while {[winfo exists $dummy]} { append dummy 0 }
  frame $dummy -class $class
  set value [option get $dummy $name $class]
  destroy $dummy

  # option not defined ?
  switch -- $value "" { return $default }

  # check a restriction on option's value
  switch [llength $args] {
    0 { # no restriction
      }
    1 { # restriction is given as a list of allowed values
        switch -- [lsearch -exact [lindex $args 0] $value] -1 {
          set msg "wrong value \"$value\" of option \"$fullName\"\
                   (should be one of $args)"
          return -code error -errorinfo $msg $msg
        }
      }
    2 { # restriction is given as a range (min and max)
        set min [lindex $args 0]
        set max [lindex $args 1]
        if {$value < $min} { set $value $min }
        if {$value > $max} { set $value $max }
      }
    default {
      error "internal error in fetchOption: wrong restriction \"$args\""
    }
  }

  return $value
}

# width of borders
set BD [fetchOption borderWidth 1 0 4]
#option add *Text.highlightThickness $BD startupFile

# scrollbar's side
set scrollbarSide [fetchOption Scrollbar.side right {left right}]

frame .inp
	# view of spin input
	scrollbar .inp.s  -command ".inp.t yview"
	text .inp.t -bd 2  -font $HelvBig -yscrollcommand ".inp.s set" -wrap word

	pack .inp.s -side $scrollbarSide -fill y
	pack append .inp \
		.inp.t {left expand fill}

	menu .inp.t.edit -tearoff 0
	.inp.t.edit add command -label "Cut" \
		-command {tk_textCopy .inp.t; tk_textCut .inp.t}
	.inp.t.edit add command -label "Copy" \
		-command {tk_textCopy .inp.t}
	.inp.t.edit add command -label "Paste" \
		-command {dopaste}

	bind .inp.t <ButtonPress-3> {
		tk_popup .inp.t.edit %X %Y
	}
	bind .inp.t <ButtonRelease-1> { setlno }


set l_typ 0;	# used by both simulator and validator
set lt_typ 0;	# used by ltl panel
set ol_typ -1;	# remembers setting last used in compilation
set m_typ 2;	# used by simulator

menu .menu.file.m
	.menu.file.m add command -label "New"  \
		-command ".inp.t delete 0.0 end"
#	.menu.file.m add command -label "UnSelect" \
#		-command ".inp.t tag remove hilite 0.0 end;\
#			  .inp.t tag remove Rev 0.0 end;\
#			  .inp.t tag remove sel 0.0 end"
	.menu.file.m add command -label "ReOpen" -command "open_spec"
	.menu.file.m add command -label "Open.." -command "open_spec 0"
	.menu.file.m add command -label "Save As.." -command "save_spec 0"
	.menu.file.m add command -label "Save" -command "save_spec"
	.menu.file.m add command -label "Quit" \
		-command "cleanup 1; destroy .; exit"

menu .menu.help.m
	.menu.help.m add command -label "About Spin" \
		-command "aboutspin"
	.menu.help.m add separator
	.menu.help.m add command -label "Promela Usage" \
		-command "promela"
	.menu.help.m add command -label "Xspin Usage" \
		-command "helper"
	.menu.help.m add command -label "Simulation" \
		-command "roadmap1"
	.menu.help.m add command -label "Verification" \
		-command "roadmap2"
	.menu.help.m add command -label "LTL Formulae" \
		-command "roadmap4"
	.menu.help.m add command -label "Spin Automata View" \
		-command "roadmap5"
	.menu.help.m add command -label "Reducing Complexity" \
		-command "roadmap3"

menu .menu.run.m
	.menu.run.m add command -label "Run Syntax Check" \
		-command {syntax_check "-a -v" "Syntax Check"}
	.menu.run.m add command -label "Run Slicing Algorithm" \
		-command {syntax_check "-A" "Property-Specific Slicing"}
	.menu.run.m add separator
	.menu.run.m add command -label "Set Simulation Parameters.." \
		-command simulation
	.menu.run.m add command -label "(Re)Run Simulation" \
		-command Rewind -state disabled
	.menu.run.m add separator
	.menu.run.m add command -label "Set Verification Parameters.." \
		-command "basicval"
	.menu.run.m add command -label "(Re)Run Verification" \
		-command {runval "0"} -state disabled
	.menu.run.m add separator
	.menu.run.m add command -label "LTL Property manager.." \
		-command call_tl
	.menu.run.m add separator
	.menu.run.m add command -label "View Spin Automaton for each Proctype.." \
		-command fsmview


menu .menu.edit.m
	.menu.edit.m add command -label "Cut" \
		-command {tk_textCopy .inp.t; tk_textCut .inp.t}
	.menu.edit.m add command -label "Copy" \
		-command {tk_textCopy .inp.t}
	.menu.edit.m add command -label "Paste" \
		-command {tk_textPaste .inp.t}

set FSz 110

menu .menu.view.m
	.menu.view.m add command -label "Larger" \
		-command {
			incr FSz 10
			set HelvBig "-Adobe-Helvetica-Medium-R-Normal--*-$FSz-*-*-*-*-*-*"
			.inp.t configure -font $HelvBig
		}
	.menu.view.m add command -label "Default text size" \
		-command {
			set FSz 110
			set HelvBig "-Adobe-Helvetica-Medium-R-Normal--*-$FSz-*-*-*-*-*-*"
			.inp.t configure -font $HelvBig
		}
	.menu.view.m add command -label "Smaller" \
		-command {
			incr FSz -10
			set HelvBig "-Adobe-Helvetica-Medium-R-Normal--*-$FSz-*-*-*-*-*-*"
			.inp.t configure -font $HelvBig
		}
	.menu.view.m add separator
	.menu.view.m add command -label "Clear Selections" \
		-command ".inp.t tag remove hilite 0.0 end;\
			  .inp.t tag remove Rev 0.0 end;\
			  .inp.t tag remove sel 0.0 end"

proc setlno {} {
	scan [.inp.t index insert] "%d.%d" lno cno
	.menu.ent delete 0 end
	.menu.ent insert end $lno
	.inp.t tag remove hilite $lno.0 $lno.end	;# or else cursor is invis
	update
}

proc add_log {{y ""}} {

	if {$y == "\n"} { return }
	.log.t insert end "\n$y"
	.log.t yview -pickplace end
}

proc cleanup {how} {
	global Unix
	if {$Unix == 0 && $how == 1} {
		add_log "removing temporary files, please wait.."; update
	}
	rmfile "pan.h pan.c pan.t pan.m pan.b pan.tmp pan.ltl"
	rmfile "pan.oin pan.pre pan.out pan.exe pan.otl"
	rmfile "pan_in pan_in.trail trail.out pan"
	rmfile "_tmp1_ _tmp2_ pan.o pan.obj pan.exe"
	if {$Unix == 0 && $how == 1} { add_log "done.." }
}


pack append . \
	.log  {bot frame w fillx} \
	.inp  {bot frame w expand fill} \
	.menu {top fillx}

# simulation parameters - initial settings
	set fvars 1
	set msc   1;	set svars 1
	set rvars 1
	set stop  0;	set tsc 0
	set seed	"1";	# random sumulation
	set jumpsteps	"0";	# guided simulation

	set s_typ 0
	# meaning s_type values:
	# 0 - Random Simulation (using seed)
	# 1 - Guided Simulation (using pan.trail)
	# 2 - Interactive Simulation

	set whichsim 0
	# meaning of whichsim values:
	# 0 - use pan_in.tra(il)
	# 1 - use user specified file

tkwait visibility .log
add_log "SPIN LOG:"
add_log " $version"
add_log "Xspin Version $xversion"
add_log "TclTk Version [info tclversion]/$tk_version\n"

	if {$Unix == 0} {
	if {[auto_execok $CC0] == "" \
	||  [auto_execok $CC0] == 0} {
		set m "Error: no C compiler found: $CC"
		add_log $m
		catch { tk_messageBox -icon info -message $m }
	}}

.inp.t configure -background $BG -foreground $FG

# process execution barchart

set Data(0) 0
set Name(0) "-"
set n 0
set bar_handle 0
set PlaceBar	""

proc stopbar {} {
	global Data Name n PlaceBar
	for {set i 0} {$i <= $n} {incr i} {
		set Data($i) 0
		set Name($i) ""
	}
	set n 0
	set PlaceBar [wm geometry .bar]
	set k [string first "\+" $PlaceBar]
	if {$k > 0} {
		set PlaceBar [string range $PlaceBar $k end]
	}
	catch { destroy .bar }
}

proc growbar {v} {
	global n Data
	set Data($n) $v
	incr n
	catch { fillbar }
}

proc shrinkbar {} {
	global n
	incr n -1
	set Data($n) 0
	catch { fillbar }
}

proc stepbar {v nm} {
	global n Data Name

	if {$v >= 0} {
		if { [info exists Data($v)] } {
			incr Data($v)
		} else {
			set Data($v) 1
		}
		if {$v >= $n} {
			set n [expr $v+1]
		}
		if { [string length $nm] > 4} {
			set Name($v) [string range $nm 0 4]
		} else {
			set Name($v) $nm
		}
		catch { fillbar }
	}
}

proc adjustbar {v w} {
	global Data

	set Data($v) $w
	catch { fillbar }
}

proc startbar {tl} {
	global n Data bar_handle Ptype PlaceBar

	catch { destroy .bar }
	toplevel .bar
	wm minsize .bar 200 200
	wm title .bar "$tl"

	set maxy [expr [winfo screenheight .] - 200]
	if {$PlaceBar == ""} {
		set PlaceBar "+[expr [winfo rootx .]+150]+$maxy"
	}
	wm geometry .bar $PlaceBar

	set bar_handle [canvas .bar.c -height 410 -width 410 -relief raised]
	frame  .bar.buts

	button .bar.buts.s1 -text "Save in:  panbar.ps" \
		-command ".bar.c postscript -file panbar.ps -colormode $Ptype"

	button .bar.buts.b -text " Close " -command "stopbar"
	pack .bar.buts.b .bar.buts.s1 -side right
	pack append .bar  .bar.c {top expand fill}  .bar.buts {bot frame e}
}

proc fillbar {} {
	global n Data Name

	.bar.c delete grid
	.bar.c delete data
	set sum 0
	for {set i 0} {$i < $n} {incr i} {
		if { [info exists Data($i)] } {
			incr sum $Data($i)
		} else {
			set Data($i) 0
			set Name($i) "-"
		}
	}
	for {set i 0} {$i < $n} {incr i} {
		.bar.c create line \
			$i 0 \
			$i 100 \
			-fill #222222 -tags grid
		.bar.c create text $i 105 \
			-text $i -tags grid
		.bar.c create text $i 110 \
			-text "$Name($i)" \
			-fill blue -tags grid

		if { [info exists Data($i)] } {
			set y [expr ($Data($i)*100)/$sum]
			.bar.c create line \
				$i 100  \
				$i [expr 100-$y] \
				-width 35 -fill red -tags data
			if {$y > 6} {
				set nrcol "yellow"
			} else {
				set nrcol "red"
			}
			.bar.c create text $i 95 \
				-text "$Data($i)" \
				-fill $nrcol -tags grid
		}
	}

	.bar.c create text [expr ($n)/2.0] -15 -text "Percentage of $sum System Steps" \
		-anchor c -justify center -tags grid
	.bar.c create text [expr ($n)/2.0] -8 -text "Executed Per Process ($n total)" \
		-anchor c -justify center -tags grid
	.bar.c scale all 0 0 55 3
	if {$n <= 5} {
		.bar.c move all 100 60
	} else {
		.bar.c move all 50 60
	}
}

proc barscales {} {
	global bar_handle

	catch {
		button .bar.buts.b4 -text "Larger" \
			-command "$bar_handle scale all 0 0 1.1 1.1"
		button .bar.buts.b5 -text "Smaller" \
			-command "$bar_handle scale all 0 0 0.9 0.9"
		pack append .bar.buts \
			.bar.buts.b4 {right padx 5} \
			.bar.buts.b5 {right padx 5}
	}
}

# Files and Generic Boxes

set file ""
set boxnr 0

proc rmfile {f} {
	global Unix CMD tk_major tk_minor

	set err ""
	catch { eval file delete -force $f } err
	if {$err == "" } { return }

	if {$Unix} {
		catch {exec rm -f $f}
	} else {
		set n [llength $f]
		for {set i 0} {$i < $n} {incr i} {
			set g [lindex $f $i]
			add_log "rm $g"
			if {$tk_major >= 4 && $tk_minor >= 2} {
				catch {exec $CMD /c del $g}
			} else {
				catch {exec $CMD >&@stdout /c del $g}
			}
	}	}
}

proc mvfile {f g} {
	global Unix CMD tk_major tk_minor

	set err ""
	catch { file rename -force $f $g } err
	if {$err == "" } { return }

	if {$Unix} {
		catch {exec mv $f $g}
	} else {
		if {$tk_major >= 4 && $tk_minor >= 2} {
			catch {exec $CMD /c move $f $g}
		} else {
			catch {exec $CMD >&@stdout /c move $f $g}
		}
	}
}

proc cpfile {f g} {
	global Unix CMD tk_major tk_minor

	set err ""
	catch { file copy -force $f $g } err
	if {$err == "" } { return }

	if {$Unix} {
		catch {exec cp $f $g}
	} else {
		if {$tk_major >= 4 && $tk_minor >= 2} {
		catch {exec $CMD /c copy $f $g}
		} else {
		catch {exec $CMD >&@stdout /c copy $f $g}
	}	}
}

proc cmpfile {f g} {
	global Unix

	set err ""
	if {$Unix} {
		catch {exec cmp $f $g} err
	} else {
		if {[file exists $f] == 0 \
		||  [file exists $g] == 0} {
			return "error"
		}
		set fd1 [open $f r]
		set fd2 [open $g r]
		while {1} {
			set n1 [gets $fd1 line1]
			set n2 [gets $fd2 line2]
			if {$n1 != $n2 \
			||  [string compare $line1 $line2] != 0} {
				set err "files differ"
				break
			}
			if {$n1 < 0} { break }
		}
		close $fd1
		close $fd2
	}
	return $err
}

proc file_ok {f} {
	if {[file exists $f]} {
		if {![file isfile $f] || ![file writable $f]} {
			set m "error: file $f is not writable"
			add_log $m
			catch { tk_messageBox -icon info -message $m }
			return 0
	}	}
	return 1
}

proc mkpan_in {} {
	global HasNever
	set fd [open pan_in w]

	fconfigure $fd -translation lf
	puts $fd [.inp.t get 0.0 end] nonewline

	if {$HasNever != ""} {
		if [catch {set fdn [open $HasNever r]} errmsg] {
			add_log $errmsg
			catch { tk_messageBox -icon info -message $errmsg }
		} else {
			while {[gets $fdn line] > -1} {
				puts $fd $line
			}
			catch "close $fdn"
	}	}
	catch "flush $fd"
	catch "close $fd"
}

proc no_ltlchange {} {

	if {![file exists pan.ltl]} {
		return 1
	}
	if {![file exists pan.otl]} {
		cpfile pan.ltl pan.otl
		return 0		; first time
	}
	set err [cmpfile pan.ltl pan.otl]
	if {[string length $err] > 0} {
		cpfile pan.ltl pan.otl
		return 0		;# different
	}
	return 1			;# unchanged
}

proc no_change {} {
	global nv_typ

	mkpan_in	;# keep this up to date
	if {![file exists pan.oin]} {
		cpfile pan_in pan.oin
		return 0		; first time
	}
	set err [cmpfile pan_in pan.oin]
	if {[string length $err] > 0} {
		cpfile pan_in pan.oin
		return 0		;# different
	}
	if {$nv_typ == 0} {
		return 1
	}
	return [no_ltlchange]		;# unchanged
}

proc mk_exec {} {
	global Unix CC SPIN notignored

	set nochange [no_change]
	if {$nochange == 1 && [file exists "pan"]} {
		add_log "<no recompilation needed>"
		return 1
	}

	add_log "<compiling executable>"
	catch {
	warner "Compiling" "Please wait until compilation of the \
executable produced by spin completes." 300
	}
	add_log "$SPIN -a pan_in"

	catch {exec $SPIN -a pan_in >&pan.tmp}
	set errmsg [msg_file pan.tmp 1]

	if {[string length $errmsg]>0} {
		add_log "$errmsg"
		catch { tk_messageBox -icon info -message $errmsg }
		add_log "<stopped compilation attempt>"
		catch { destroy .warn }
		return 0
	}

	add_log "$CC -o pan -D_POSIX_SOURCE pan.c"; update
	if {$Unix} {
		catch { eval exec $CC -o pan -D_POSIX_SOURCE pan.c >pan.tmp 2>pan.err} errmsg
	} else {
		catch { eval exec $CC -o pan -D_POSIX_SOURCE pan.c >pan.tmp 2>pan.err}
	}
	set errmsg [msg_file pan.tmp 0]
	set errmsg "$errmsg \n [msg_file pan.err 0]"

		if {[string length $errmsg]>0 && $notignored} {
			add_log "$errmsg"
			catch { tk_messageBox -icon info -message $errmsg }
			catch { destroy .warn }
			return 0
		}
	add_log "<compilation complete>"
	catch {destroy .warn}
	return 1
}

set PlaceWarn	"+20+20"

proc warner {banner msg w} {
	global PlaceWarn

	catch {destroy .warn}
	toplevel .warn

	wm title .warn "$banner"
	wm iconname .warn "Info"
	set k [string first "\+" $PlaceWarn]
	if {$k > 0} {
		set PlaceWarn [string range $PlaceWarn $k end]
	}
	wm geometry .warn $PlaceWarn

	message .warn.t -width $w -text $msg
	button .warn.ok -text "Ok" \
		-command "set PlaceWarn [wm geometry .warn]; destroy .warn"

	pack append .warn .warn.t {top expand fill}
	pack append .warn .warn.ok {bottom}

	update
}

proc dosave {} {
	global Fname xversion

	if {[file_ok $Fname]==0} return
	set fd [open $Fname w]
	add_log "<saved spec in $Fname>"
	puts $fd "[.inp.t get 0.0 end]" nonewline
	catch "flush $fd"
	catch "close $fd"
	wm title . "SPIN CONTROL $xversion -- File: $Fname"
}

proc save_spec {{curr 1}} {
#-
#- Save the input area into a file.
#-
#- If 'curr' is true then we save to the current file. Otherwise, a file
#- selection dialog is presented. If a file is selected (note that the
#- dialog can be canceled) then we try to write to it.
#-

  global Fname

  if $curr {
    switch -- $Fname "" {
      add_log "no file to save to, try \"Save as ...\""
      return
    }
    writeoutfile .inp.t $Fname
  } else {
    # try to use the predefined file selection dialog
    switch [info commands tk_getSaveFile] "" {
      # some old version of Tk so use our own file selection dialog
      set fileselect "FileSelect save"
    } default {
      set fileselect "tk_getSaveFile"
    }
  
    # get the file (return if the file selection dialog canceled)
    switch -- [set file [eval $fileselect]] "" return
  
    # write the file and update Fname if the file written successfully
    if [writeoutfile .inp.t $file] {
      set Fname $file
    }
  }
}

proc consider_it {} {
	global file Fname xversion

	if {[file isdirectory $file]} then {
		cd $file
		fillerup ""
		add_log "cd $file"
	} else {
		if {![file isfile $file]} {
			set file ""
		} else {
			readinfile .inp.t $file

			rmfile pan_in.trail
			cpfile $file.trail pan_in.trail

			set dir [pwd]
			set Fname $file
			wm title . "SPIN CONTROL $xversion -- File: $Fname"
			destroy .b
	}	}
}

#----------------------------------------------------------------------
# Improvements - by Leszek Holenderski <lhol@win.tue.nl>
# GUI configuration and File Selection dialogs
#----------------------------------------------------------------------

# predefined priorities for options stored in the option data base are
#	widgetDefault	20
#	startupFile	40
#	userDefault	60
#	interactive	80

# most of frames are used for layout purposes so they should be invisible
option add *Frame.borderWidth 0 interactive

proc try_with {xspinrc} {

  if ![file exists $xspinrc] return

  if ![file isfile $xspinrc] {
    puts "xspin warning: the resource file \"$xspinrc\" exists but is not\
          a normal file"
    return
  }
  if ![file readable $xspinrc] {
    puts "xspin warning: the resource file \"$xspinrc\" exists but is not\
          readable"
    return
  }
  if [catch {option readfile $xspinrc userDefault} result] {
    puts "xspin warning: some problems when trying to load the resource\
          file \"$xspinrc\"\n$result"
    return
  }
}

if [info exists env(HOME)] {
  if $Unix {
    try_with [file join $env(HOME) .xspinrc]
  } else {
    try_with [file join $env(HOME) xspinrc]
  }
}

proc FileSelect {mode {title ""}} {
  switch -- $mode open - save {} default { set mode open }

  switch $mode {
    open {
      set title Open
      set okButtonText Open
    }
    save {
      set title Save
      set okButtonText Save
    }
  }

  set w .fileselect
  upvar #0 $w this

  if [winfo exists $w] {
    wm title $w $title
    $this(okButton) config -text $okButtonText
    catch {wm geom $w $this(geom)}
    wm deiconify $w
  } else {
    toplevel $w -class Fileselect
    wm title $w $title
    # the minimal size is given in characters and lines (setgrid is on)
    wm minsize $w 14 7

    # layout frames
    pack [set f [frame $w.f]] -padx 5 -pady 5 -fill both -expand yes
    pack [set buttons [frame $f.b]] -side bottom -fill x
    pack [set name    [frame $f.n]] -side bottom -fill x -pady 5
    pack [set path    [frame $f.p]] -side top -fill x
    pack [set files   [frame $f.f]] -side top -fill both -expand yes
  
    # create ok/cancel buttons
    set okButton [button $buttons.ok -text $okButtonText \
			-command "FileSelect.Close $w 1"]
    pack $okButton -side right

    set cancelButton [button $buttons.cancel -text Cancel \
			-command "FileSelect.Close $w 0"]
    pack $cancelButton -side left

    MakeSameWidth "$okButton $cancelButton"
  
    # create path button
    set pathButton $path.path
    global $w|currDir
    set pathMenu [tk_optionMenu $pathButton $w|currDir ""]
    pack $pathButton -side top
  
    # create the list of files
    global scrollbarSide

    set fileList $files.l
    set scrollbar $files.s
    pack [scrollbar $files.s -command "$fileList yview"] \
	 -side $scrollbarSide -fill y
    pack [listbox $fileList -yscrollcommand "$files.s set" -selectmode single -setgrid on] \
	 -side $scrollbarSide -expand yes -fill both
  
    bind $fileList <ButtonPress-1>        "FileSelect.Selected $w %x %y"
    bind $fileList <Double-ButtonPress-1> "FileSelect.Chosen   $w %x %y"

    set fileEntry $name.e
    pack [label $name.l -text File:] -side left
    pack [entry $fileEntry] -side right -expand yes -fill x

    set this(okButton)   $okButton
    set this(pathButton) $pathButton
    set this(pathMenu)   $pathMenu
    set this(fileList)   $fileList
    set this(fileEntry)  $fileEntry

    foreach widget "$okButton $cancelButton $pathButton $fileList $scrollbar" {
      $widget config -highlightthickness 0
    }

    wm protocol $w WM_DELETE_WINDOW [$cancelButton cget -command]
  }

  # fill in the list of files
  if ![info exists this(path)] { set this(path) [pwd] }
  FileSelect.cd $w $this(path)

  # make the dialog modal (set a grab and claim the focus)

  set oldFocus [focus]
  set oldGrab [grab current $w]
  if {$oldGrab != ""} { set grabStatus [grab status $oldGrab] }
  grab $w
  focus $this(fileEntry)

  # make the contents of file entry selected (for easy deletion)
  $this(fileEntry) select from 0
  $this(fileEntry) select to end

  # Wait for the user to respond, then restore the focus and return the
  # contents of file entry.
  # Restore the focus before deleting the window, since otherwise the
  # window manager may take the focus away so we can't redirect it.
  # Finally, restore any grab that was in effect.

  global $w|response
  tkwait variable $w|response
  catch {focus $oldFocus}
  grab release $w
  set this(geom) [wm geom $w]
  wm withdraw $w
  if {$oldGrab != ""} {
      if {$grabStatus == "global"} {
	  grab -global $oldGrab
      } else {
	  grab $oldGrab
      }
  }
  return [set $w|response]
}

proc CompareNoCase {s1 s2} {
  return [string compare [string tolower $s1] [string tolower $s2]]
}

proc FileSelect.LoadFiles {w} {
    upvar #0 $w this

    # split all names in the current directory into dirs and files
    set dirs ""
    set files ""
    set filter ""
    if [info exists this(filter)] { set filter $this(filter) }
    switch -- $filter "" { set filter * }
    foreach f [lsort -command CompareNoCase [glob -nocomplain .* *]] {
	if [file isdir $f] {
	  # exclude the '.' and '..' directory
	  switch -- $f . - .. continue
	  lappend dirs $f
	}
	if [file isfile $f] {
	  # filter files
	  switch -glob -- $f $filter { lappend files $f }
	}
    }

    # Fill in the file list
    $this(fileList) delete 0 end
    foreach d $dirs {
	# append directory mark to the name (tricky)
	set d [string trimright [file join $d a] a]
	$this(fileList) insert end $d
    }
    foreach f $files { $this(fileList) insert end $f }
}

proc FileSelect.LoadPath {w} {
    upvar #0 $w this

    # convert path to list
    set dirs [file split $this(path)]

    # compute prefix paths
    set path ""
    set paths ""
    foreach d $dirs {
	set path [file join $path $d]
	lappend paths $path
    }

    # reverse dirs and paths
    set rev_dirs ""
    foreach d $dirs { set rev_dirs [concat [list $d] $rev_dirs] }
    set rev_paths ""
    foreach p $paths { set rev_paths [concat [list $p] $rev_paths] }

    # update the path menubutton
    global $w|currDir
    set $w|currDir [lindex $rev_dirs 0]

    # fill in the path menu
    $this(pathMenu) delete 0 end
    foreach d [lrange $rev_dirs 1 end] p [lrange $rev_paths 1 end] {
	$this(pathMenu) add command -label $d -command "FileSelect.cd $w $p"
    }
}

proc FileSelect.Selected {w x y} {
    upvar #0 $w this

    # determine the selected list element
    set elem [$this(fileList) get [$this(fileList) index @$x,$y]]
    switch -- $elem "" return

    # directories cannot be selected (they can only be chosen)
    if [file isdir $elem] return

    $this(fileEntry) delete 0 end
    $this(fileEntry) insert end $elem
}

proc FileSelect.Chosen {w x y} {
    upvar #0 $w this

    # determine the selected list element
    set elem [$this(fileList) get [$this(fileList) index @$x,$y]]
    switch -- $elem "" return

    # if directory then cd, otherwise close the dialog
    if [file isdir $elem] { FileSelect.cd $w $elem } { FileSelect.Close $w 1 }
}

proc FileSelect.Close {w {ok 1}} {
  # trigger the end of dialog
  upvar #0 $w this $w|response response
  if $ok {
    set response [file join $this(path) [$this(fileEntry) get]]
  } else {
    set response ""
  }
}

proc FileSelect.cd {w dir} {
    upvar #0 $w this

    if [catch {cd $dir} errmsg] {
	puts "xspin warning: $errmsg"
	return
    }

    set this(path) [pwd]
    FileSelect.LoadFiles $w
    FileSelect.LoadPath $w
}

proc open_spec {{curr 1}} {
  global Fname

  if $curr {
    switch -- $Fname "" {
      add_log "no file to reopen, try \"Open ...\""
      return
    }
    readinfile .inp.t $Fname
  } else {
    # try to use the predefined file selection dialog
    switch [info commands tk_getOpenFile] "" {
      # some old version of Tk so use our own file selection dialog
      set fileselect "FileSelect open"
    } default {
      set fileselect "tk_getOpenFile"
    }
    set init_dir [pwd]
    # get the file (return if the file selection dialog canceled)
    switch -- [set file [eval $fileselect -initialdir { $init_dir } ]] "" return
  
    # load the file and update some items if the file loaded successfully
    if [readinfile .inp.t $file] {
      rmfile pan_in.trail
      cpfile $file.trail pan_in.trail
      set Fname $file
      set_path $Fname
    }
  }
}


proc set_path {Fname} {
	#cd to directory in file
	set fullpath [split $Fname /]
	set nlen [llength $fullpath]
	set fullpath [lrange $fullpath 0 [expr $nlen - 2]] 	;# get rid of filename
	set wd [join $fullpath /] 				;#put path back together
 	catch {cd $wd}
}

set defname ""

proc loaddefault_tl {} {
	global Fname defname

	if {$defname ==""} {
		set file2 "$Fname.ltl"
	} else {
		set file2 $defname
	}
	catch {
		.tl.main.e1 delete 0 end
		.tl.macros.t delete 0.0 end
		.tl.notes.t delete 0.0 end
		.tl.never.t delete 0.0 end
		.tl.results.t delete 0.0 end
	}
	if {![file exists $file2]} {
		.tl.notes.t insert end "Use Load to open a file or a template."
		return
	}
	catch {
		.tl.notes.title configure -text "Notes \[file $file2]:"
	}
	readinfile .tl.never.t $file2
	catch { extract_defs }
}

set suffix "ltl"

proc browse_tl {} {
	global defname suffix

	set suffix "ltl"

	catch {destroy .b}
	toplevel .b
	wm iconname .b "Load"

	frame .b.top
	frame .b.bot
	scrollbar .b.top.scroll -command ".b.top.list yview"
	listbox .b.top.list -yscroll ".b.top.scroll set" -relief raised 

	button .b.zap -text "Cancel" -command "destroy .b"
	button .b.all -text "Show All Files" \
		-command {
			set suffix ""
			fillerup ""
			destroy .b.all
		}
	message .b.bot.msg -text "Dir: "
	entry .b.bot.entry -textvariable dir -relief sunken -width 20
	pack append .b.top \
		.b.top.scroll {right filly} \
		.b.top.list {left expand fill}
	pack append .b.bot \
		.b.bot.msg {left} \
		.b.bot.entry {left}
	pack append .b \
		.b.top {top fillx} \
		.b.all {left} \
		.b.zap {left} \
		.b.bot {left}
	
	bind .b.bot.entry <Return> {
		set nd [.b.bot.entry get]
		if {[file isdirectory $nd]} {
			cd $nd
			fillerup $suffix
			add_log "cd $nd"
		}
	}

	fillerup  $suffix
	bind .b.top.list <Double-Button-1> {
		set file2 [selection get]
		if {[string first "---" $file2] >= 0} {
			# ignore
		} elseif {[string first "Invariance" $file2] >= 0} {
			catch {
			.tl.main.e1 delete 0 end
			.tl.macros.t delete 0.0 end
			.tl.notes.t delete 0.0 end
			.tl.never.t delete 0.0 end
			.tl.results.t delete 0.0 end

			.tl.main.e1 insert end "\[\] (p)"
			.tl.notes.t insert end "'p' is invariantly true throughout
an execution"
			.tl.notes.title configure \
				-text "Notes \[template $file2]:"
			do_ltl
			destroy .b
			}
		} elseif {[string first "Response" $file2] >= 0} {
			catch {
			.tl.main.e1 delete 0 end
			.tl.macros.t delete 0.0 end
			.tl.notes.t delete 0.0 end
			.tl.never.t delete 0.0 end
			.tl.results.t delete 0.0 end

			.tl.main.e1 insert end "\[\] ((p) -> (<> (q)))"
			.tl.notes.t insert end "if 'p' is true in at least one state,
then sometime thereafter 'q' must also
become true at least once."
			.tl.notes.title configure \
				-text "Notes \[template $file2]:"
			do_ltl
			destroy .b
			}
		} elseif {[string first "Precedence" $file2] >= 0} {
			catch {
			.tl.main.e1 delete 0 end
			.tl.macros.t delete 0.0 end
			.tl.notes.t delete 0.0 end
			.tl.never.t delete 0.0 end
			.tl.results.t delete 0.0 end

			.tl.main.e1 insert end "\[\] ((p) -> ((q) U (r)))"
			.tl.notes.t insert end "'p' is a trigger, or 'enabling' condition that
determines when this requirement becomes applicable
'r' is the fullfillment of the requirement, and
'q' is a condition that must remain true in the interim."
			.tl.notes.title configure \
				-text "Notes \[template $file2]:"
			do_ltl
			destroy .b
			}
		} elseif {[string first "Objective" $file2] >= 0} {
			catch {
			.tl.main.e1 delete 0 end
			.tl.macros.t delete 0.0 end
			.tl.notes.t delete 0.0 end
			.tl.never.t delete 0.0 end
			.tl.results.t delete 0.0 end

			.tl.main.e1 insert end "\[\] ((p) -> <>((q) || (r)))"
			.tl.notes.t insert end "'p' is a trigger, or 'enabling' condition that
determines when this requirement becomes applicable
'q' is the fullfillment of the requirement, and
'r' is a discharging condition that terminates the
applicability of the requirement."

			.tl.notes.title configure \
				-text "Notes \[template $file2]:"
			do_ltl
			destroy .b
			}
		} elseif {[file isdirectory $file2]} then {
			cd $file2
			fillerup $suffix
			add_log "cd $file2"
		} else {
			if {![file isfile $file2]} {
				set file2 ""
			} else {
				catch {
				.tl.main.e1 delete 0 end
				.tl.macros.t delete 0.0 end
				.tl.notes.t delete 0.0 end
				.tl.never.t delete 0.0 end
				.tl.results.t delete 0.0 end
				.tl.notes.title configure \
					-text "Notes \[file $file2]:"
				}
				readinfile .tl.never.t $file2
				set defname $file2
				catch { extract_defs }
				set dir [pwd]
				destroy .b
			}
		}
	}
}

proc reopen {} {
	global Fname

	catch {readinfile .inp.t $Fname} ermsg
	if {[string length $ermsg]<=1} { return }
	add_log $ermsg
	catch { tk_messageBox -icon info -message $ermsg }
}

proc check_xsp {f} {
	set ff ${f}.xsp
	if [catch {set fd [open $ff r]} errmsg] {
		# add_log "no file $ff"
		return
	}
	add_log "reading $ff file"
	update
	while {[gets $fd line] > -1} {
		if {[string first "X:" $line] == 0} {
			set C [string range $line 3 end]
			add_log "X: $C"
			update
			catch { eval exec $C } errmsg
			if {$errmsg != ""} { add_log $errmsg }
		}
		if {[string first "L:" $line] == 0} {
			set N [string range $line 3 end]
			catch {.log.t configure -height $N -bg black -fg gold}
		}
	}
}

proc writeoutfile {from to} {

  if ![file_ok $to] { return 0 }

  if [catch {set fd [open $to w]} errmsg] {
    add_log $errmsg
    catch { tk_messageBox -icon info -message $ermsg }
    return 0
  }

  add_log "<saved spec in $to>"
  puts $fd [string trimright [$from get 0.0 end] "\n"]
# puts -nonewline $fd [$from get 0.0 end]
  close $fd
  return 1
}

proc readinfile {into from} {

  if [catch {set fd [open $from r]} errmsg] {
    add_log $errmsg
    catch { tk_messageBox -icon info -message $ermsg }
    return 0
  }
	
  $into delete 0.0 end
  while {[gets $fd line] > -1} { $into insert end $line\n }
  catch { close $fd }
  add_log "<open $from>"

  global LastGenerate
  set LastGenerate ""	;# used in Validation.tcl
  return 1
}

proc fillerup {suffix} {
	wm title .b [pwd]
	.b.top.list delete 0 end

	set dotdot 0
	set l {}
	catch {	if {$suffix != ""} {
			set l [lsort [glob *.$suffix]]
		} else {
			set l [lsort [glob *]]
	}	}
	foreach i $l {
		.b.top.list insert end $i
		if {$i == ".."} { set dotdot 1 }
	}
	if {$dotdot == 0} {
		.b.top.list insert 0 ".."
	}
	if {$suffix != ""} {
		.b.top.list insert 0 "------files:--------"
		.b.top.list insert 0 "Objective(p,q,r)"
		.b.top.list insert 0 "Precedence(p,q,r)"
		.b.top.list insert 0 "Response(p,q)"
		.b.top.list insert 0 "Invariance(p)"
		.b.top.list insert 0 "-----templates:-----"
	}
}

proc gotoline {} {
	global BG FG

	catch { destroy .ln }
	toplevel .ln
	wm title .ln "Goto Line"
	wm iconname .ln "Goto"

	label .ln.lab -text "Enter line number:"
	entry .ln.ent -width 20 -relief sunken -textvariable lno
	pack append .ln \
		.ln.lab {left padx 1m} \
		.ln.ent {right expand}
	bind .ln.ent <Return> {
		.inp.t tag remove hilite 0.0 end
		.inp.t tag add hilite $lno.0 $lno.1000
		.inp.t tag configure hilite \
			-background $FG -foreground $BG
		.inp.t yview -pickplace [expr $lno-1]
	}
	focus .ln.ent
}

proc savebox {b} {
	set fname [.c$b.f.e1 get]
	if {[file_ok $fname]==0} return
	set fd [open $fname w]
	add_log "<saved output in $fname>"
	puts $fd "[.c$b.z.t get 0.0 end]" nonewline
	catch "flush $fd"
	catch "close $fd"
}

proc doplace {a b} {
	global PlaceBox
	set PlaceBox($a) [wm geometry .c$b]
	set k [string first "\+" $PlaceBox($a)]
	if {$k > 0} {
		set PlaceBox($a) [string range $PlaceBox($a) $k end]
	}
}

proc mkbox {n m p {wd 60} {ht 10} {xp 200} {yp 200}} {
	global boxnr FG BG PlaceBox HelvBig
	global ind old_ss old_insert new_insert;# for search capability

	incr boxnr

	toplevel .c$boxnr
	wm title .c$boxnr $n
	wm iconname .c$boxnr $m

	if {[info exists PlaceBox($n)] == 0} {
		set PlaceBox($n) "+$xp+$yp"
	} 
	wm geometry .c$boxnr $PlaceBox($n)

	#initialize search parameters
	set ind 1.0
	set old_ss ""
	set old_insert ""
	set new_insert ""
	frame .c$boxnr.d ;# search bar
		label .c$boxnr.d.l -text "Search for: "
		entry .c$boxnr.d.e -width 15
		bind .c$boxnr.d.e <KeyPress-Return> "search_sim .c$boxnr.z.t .c$boxnr.d.e .c$boxnr" 
		button .c$boxnr.d.b -text "Find" -command "search_sim .c$boxnr.z.t .c$boxnr.d.e .c$boxnr"

	pack .c$boxnr.d -side top -fill x
	pack .c$boxnr.d.b -side right
	pack .c$boxnr.d.e -side right
	pack .c$boxnr.d.l -side right

	frame .c$boxnr.z
	
	text .c$boxnr.z.t -relief raised -bd 2 -font $HelvBig \
		-background $BG -foreground $FG \
		-yscrollcommand ".c$boxnr.z.s set" \
		-setgrid 1 -width $wd -height $ht -wrap word
	bind .c$boxnr.z.t <ButtonRelease-1> "+update idletasks; update_insert .c$boxnr.z.t"
	scrollbar .c$boxnr.z.s \
		-command ".c$boxnr.z.t yview"
	pack append .c$boxnr.z \
		.c$boxnr.z.s {left filly} \
		.c$boxnr.z.t {left expand fill}

	button .c$boxnr.b -text "Close" \
	-command "doplace {$n} {$boxnr}; destroy .c$boxnr"
	button .c$boxnr.c -text "Clear" \
		-command ".c$boxnr.z.t delete 0.0 end"
	pack append .c$boxnr \
		.c$boxnr.z {top expand fill} \
		.c$boxnr.b {right padx 5} \
		.c$boxnr.c {right padx 5}

	if {[string length $p]>0} {
		frame  .c$boxnr.f -relief sunken
		button .c$boxnr.f.e0 -text "Save in: " \
			-command "savebox $boxnr"
		entry  .c$boxnr.f.e1 -relief flat -width 10
		.c$boxnr.f.e1 insert end $p
		pack append .c$boxnr.f \
			.c$boxnr.f.e0 {left padx 5} \
			.c$boxnr.f.e1 {left}
		pack append .c$boxnr \
			.c$boxnr.f {right padx 5}
	}

	tkwait visibility .c$boxnr
	raise .c$boxnr
	return $boxnr
}
proc update_insert {t} {
	global new_insert
	update idletasks
	set new_insert [$t index insert]
}

proc search_sim {t e b} {
	global ind old_ss old_insert new_insert

	set ss [$e get]

	if {$ss == ""} {
		return
		}

	#if user has selected use that lin.char as 'ind'. otherwise use end of last word found
	#set new_insert [$t index insert]
	if {$new_insert != $old_insert} {
		set ind $new_insert ;# this is where we will start searching
		set old_insert $new_insert ;# update select location
	}
	set cur_ind $ind
	set ind [$t search $ss $ind]

	set old_ss $ss
	if {$ind != ""} {
		$t yview -pickplace $ind
		$t tag configure hilite -foreground black -background white
		$t tag delete hilite 
		set split_ind [split $ind "."]
		set char [lindex $split_ind 1]	
		set char [incr char [string length $ss]]
		set indstart $ind
		set indend ""
		append indend [lindex $split_ind 0] "." $char
		$t tag add hilite $indstart $indend
		$t tag configure hilite -foreground white -background black
		set ind $indend
	} else {
		# set ind 1.0
		catch { tk_messageBox -icon info -message "no match for $ss" }
		set ind $cur_ind ;# restore ind to last good value
		raise $b
	}	

}

# Tcl/Tk book, page 219
proc forAllMatches {w pattern} {
	global BG FG lno

	scan [$w index end] %d numLines
	for {set i 1} {$i < $numLines} { incr i} {
		$w mark set last $i.0
		if {[regexp -indices $pattern \
			[$w get last "last lineend"] indices]} {
				$w mark set first \
					"last + [lindex $indices 0] chars"
				$w mark set last "last + 1 chars \
					+ [lindex $indices 1] chars"
			.inp.t tag add hilite $i.0 $i.end
			.inp.t tag configure hilite \
				-background $FG -foreground $BG
#			.inp.t yview -pickplace [expr $i-1]
	}	}

#	move to the next line that matches

	for {set i [expr $lno+1]} {$i < $numLines} { incr i} {
		$w mark set last $i.0
		if {[regexp -indices $pattern \
			[$w get last "last lineend"] indices]} {
				$w mark set first \
					"last + [lindex $indices 0] chars"
				$w mark set last "last + 1 chars \
					+ [lindex $indices 1] chars"
			.inp.t yview -pickplace [expr $i-5]
			set lno $i
			return
	}	}
	for {set i 1} {$i <= $lno} { incr i} {
		$w mark set last $i.0
		if {[regexp -indices $pattern \
			[$w get last "last lineend"] indices]} {
				$w mark set first \
					"last + [lindex $indices 0] chars"
				$w mark set last "last + 1 chars \
					+ [lindex $indices 1] chars"
			.inp.t yview -pickplace [expr $i-5]
			set lno $i
			return
	}	}
	catch {
		tk_messageBox -icon info -message "No Match of \"$pattern\""
	}
}

set noprep 0

proc hasWord {pattern} {
	global SPIN noprep
	set err ""
	if {![file exists pan.pre] && $noprep == 0} {
		add_log "$SPIN -Z pan_in ;# preprocess input $pattern"
		catch {exec $SPIN -Z pan_in >&pan.tmp} err
		# leaves output in pan.pre
		add_log "<done preprocess>"
	}

	if {[string length $err] == 0 && $noprep == 0} {
		set fd [open pan.pre r]
		while {[gets $fd line] > -1} {
		  if {[regexp -indices $pattern $line indices]} {
			catch "close $fd"
			return 1
		} }
		catch "close $fd"
		return 0
	}

	# else, there were errors, just ignore the include files:
	set noprep 1
	add_log "$err"
	scan [.inp.t index end] %d numLines
	for {set i 1} {$i < $numLines} { incr i} {
		.inp.t mark set last $i.0
		if {[regexp -indices $pattern \
			[.inp.t get last "last lineend"] indices]} {
				return 1
		}
	}
	return 0
}

# FSM view option

set nodeX(0) 0
set nodeY(0) 0
set Label(0) 0
set Transit(0) {}
set TLabel(0) 0
set Lab2Node(0) 0
set Dist(0) 0
set State(0) 0
set Edges(0) {}
set New 0
set MaxDist 0
set Maxx 0
set Maxy 0
set Minx 50
set Miny 50
set reached_end 0
set Scale 1

set cnr	0
set x 0
set y 0

proc fsmview {} {
	global Unix

	add_log "<fsm view option>"

	if {[mk_exec] != 1} { return }

	catch {destroy .z}
	toplevel .z

	wm title .z "Double-Click Proc"

	listbox .z.list -setgrid 1
	button .z.b -text "Cancel" -command "destroy .z"

	pack append .z \
		.z.list {top expand fill} \
		.z.b {right padx 5}

	if {$Unix} {
		add_log "./pan -d	# find proc names"; update
		set fd [open "|./pan -d" w+]
	} else {
		add_log "pan -d	# find proc names"; update
		catch { eval exec pan -d >&pan.tmp } err
		if {$err != ""} {
			add_log "error: $err"
		}
		set fd [open pan.tmp r]
	}
	while {[gets $fd line] > -1} {
		if {[string first "proctype" $line] >= 0 } {
			.z.list insert end \
			[string trim [string range $line 9 end]]
	}	}
	catch { close $fd }

	bind .z.list <Double-Button-1> {
		set pfind [selection get]
		catch { destroy .z }
		findfsm $pfind
	}
	focus .z.list
}

proc findfsm {pfind} {
	global Unix New Dist State Edges Label
	global Transit MaxDist reached_end TLabel DOT

	if {[mk_exec] != 1} { return }

	set src  0; set trn 0; set trg 0
	set Want 0; set MaxDist 0; set startstate ""

	catch { foreach el [array names State] { unset State($el) } }
	catch { foreach el [array names Edges] { unset Edges($el) } }
	catch { foreach el [array names Dist]  { unset Dist($el) } }

	if {$Unix} {
		add_log "./pan -d	# compute fsm"; update
		set fd [open "|./pan -d" w+]
	} else {
		add_log "pan -d	# compute fsm"; update
		catch { eval exec pan -d >&pan.tmp }
		set fd [open pan.tmp r]
	}
	while {[gets $fd line] > -1} {
		set k [string first "proctype" $line]
		if { $k >= 0 } {
			if { $Want == 1 } {
				break
			}
			incr k 9
			set pname [string range $line $k end]

			if { [string first $pfind $line] >= 0 \
			&&   [string length $pfind] == [string length $pname]} {
				set Want 1
				set reached_end 0
				set nsrc "$pname:0"
				set Dist($nsrc) 0
				set Label($nsrc) "end"
				set Edges($nsrc) {}
			}
		} elseif { $Want == 1 \
			&& [string first "statement" $line] < 0	
			&& [string first "state" $line] >= 0} {
			scan $line "	state %d -(tr %d)-> state %d" \
				src trn trg
			if {$trg == 0} { set reached_end 1 }

			set nsrc "$pname:$src"
			set ntrg "$pname:$trg"

			if {$startstate == ""} { set startstate $nsrc }

			set k [string first "line" $line]
			if { $k > 0 } {
				set m [string first "=>" $line]
				incr m -1
				set lbl [string range $line $k $m]
				incr m 3
				set action [string range $line $m end]
			} else {
				set lbl "line 0"
				set action "line 0"
			}
			set Label($nsrc) $lbl
			if { [info exists Dist($nsrc)] == 0 } {
				set Dist($nsrc) 0
				set Edges($nsrc) {}
			}
			if { [info exists Dist($ntrg)] == 0 } {
				set Dist($ntrg) [expr $Dist($nsrc) + 1]
				set Edges($ntrg) {}
				if {$Dist($ntrg) > $MaxDist } {
					set MaxDist $Dist($ntrg)
				}
			} else {
				if { [expr $Dist($nsrc) + 1] < $Dist($ntrg) } {
					set Dist($ntrg) [expr $Dist($nsrc) + 1]
					if {$Dist($ntrg) > $MaxDist } {
						set MaxDist $Dist($ntrg)
					}
			}	}
if {0 \
&&  [auto_execok $DOT] != 0 \
&&  [auto_execok $DOT] != ""} {
	set z1 [string first "\[" $action]
	set z2 [string last  "\]" $action]
	if {$z1 > 0 && $z2 > $z1} {
		incr z1 -1; incr z2
		set a1 [string range $action 0 $z1]
		set a2 [string range $action $z2 end]
		set action "$a1$a2"
	}
			set kkk "$nsrc;$trg:$action"
			lappend Edges($nsrc) "$kkk"
			lappend Edges($kkk) $ntrg
			lappend Transit($nsrc) "$lbl"
			lappend Transit($kkk) ""
			set Dist($kkk) $Dist($ntrg)
			set Label($kkk) "T3"
} else {
			lappend Edges($nsrc) $ntrg
			lappend Transit($nsrc) $action
}
		}
	}
	if { $Want == 1 } {
		dograph $pfind $startstate
	} else {
		add_log "sorry, $pfind not found..."
		catch {
		tk_messageBox -icon info -message "$pfind not found..."
		}
	}
	catch "close $fd"
	add_log "<done>"
	update
}

proc plunknode {el prefix} {
	global State Label TLabel
	global x y

	set pn [string range $el $prefix end]
	set State($el) [mkNode "$Label($el)" $pn $x $y]

	if { $x > 250 } {
		set x [expr $x - 250]
		set x [expr 250 - $x]
	} else {
		set x [expr 250 - $x]
		incr x 75
		set x [expr 250 + $x]
	}
}

proc dograph {n s} {
	global Dist Edges Label Transit MaxDist State ELabel
	global cnr lcnr reached_end x y Unix DOT
	set count -1

	set lcnr [mkcanvas "FSM $n" $n 300 300 0]
	set prefix [string length $n]
	incr prefix
	set y 0

	while {$count < $MaxDist} {
		incr count
		incr y 50
		set x 250
		foreach el [array names Dist] {
			if { [ string first $n $el ] >= 0 \
			&&   $Dist($el) == $count \
			&&   $el != "$n:0" } {
				plunknode $el $prefix
		}	}
	}
	if {$reached_end == 1} {
		# force end state at the bottom
		incr y 50
		set x 250
		plunknode "$n:0" $prefix
	}

	foreach el [array names Edges] {
		if { [ string first $n $el ] >= 0 } {
			for {set i 0} { [lindex $Edges($el) $i] != "" } {incr i} {
				set ntrg [lindex $Edges($el) $i]
				set lbl  [lindex $Transit($el) $i]
				mkEdge $lbl $State($el) $State($ntrg)
				set ELabel($el,$ntrg) "$lbl"
			}
	}	}
	addscales $lcnr

	.f$cnr.c itemconfigure $State($s) -outline red; update

	if {[auto_execok $DOT] != 0 \
	&&  [auto_execok $DOT] != ""} {
		dodot $n
#		button .f$lcnr.b66 -text "Redraw with Dot"  -command "dodot $n"
#		pack append .f$lcnr .f$lcnr.b66 {right padx 5}
	}
	update
}

proc mkNode {n t x y} {
	# tcl book p. 213
	global cnr NodeWidth NodeHeight HelvBig
	global nodeX nodeY New TLabel Lab2Node

	if {[string first ";" $t] > 0} {
		set New [.f$cnr.c create rectangle [expr $x-1] [expr $y-1] \
			[expr $x+1] [expr $y+1] \
			-outline white \
			-fill white \
			-tags node]
		set z [string first ":" $t]; incr z
		set t [string range $t $z end]
		set Lab [.f$cnr.c create text $x $y -font $HelvBig -text $n -tags node]
	} else {
		set New [.f$cnr.c create oval [expr $x-10] [expr $y-10] \
			[expr $x+10] [expr $y+10] \
			-outline black \
			-fill white \
			-tags node]
		set Lab [.f$cnr.c create text $x $y -font $HelvBig -text $n -tags node]
	
		.f$cnr.c bind $Lab <Any-Enter> "
			.f$cnr.c itemconfigure {$Lab} -fill black -text {$t}
			if {[string first \"end\" $n] < 0 } { set_src {$t} }
		"
		.f$cnr.c bind $Lab <Any-Leave> "
			.f$cnr.c itemconfigure {$Lab} -fill black -text {$n}
		"
	}
	set nodeX($New) $x
	set nodeY($New) $y
	set TLabel($New) $Lab
	set Lab2Node($Lab) $New
	set NodeWidth($New) 10
	set NodeHeight($New) 10

	update
	return $New
}

proc set_src {n} {
	set where 0
	scan $n "line %d" where
	.inp.t tag remove hilite 0.0 end
	src_line $where
}

proc mkEdge {L a b} {
	global cnr Xrem Yrem TransLabel HelvBig
	global nodeX nodeY edgeHead edgeTail

	if { $nodeY($b) > $nodeY($a) } {
		set ydiff -11
	} elseif { $nodeY($b) < $nodeY($a) } {
		set ydiff 11
	} else {
		set ydiff 0
	}
	if { $nodeX($b) > $nodeX($a) } {
		set xdiff -6
	} elseif { $nodeX($b) < $nodeX($a) } {
		set xdiff 6
	} else {
		set xdiff 0
	}
	set edge [.f$cnr.c create line \
		$nodeX($a) $nodeY($a) \
		[expr $nodeX($b) + $xdiff] \
		[expr $nodeY($b) + $ydiff] \
		 -arrow both -arrowshape {4 3 3} ]
	.f$cnr.c lower $edge
	lappend edgeHead($a) $edge
	lappend edgeTail($b) $edge

	set Xrem($edge) $xdiff
	set Yrem($edge) $ydiff

	set midX [expr $nodeX($a) + $nodeX($b)]
	set midX [expr $midX / 2 ]
	set midY [expr $nodeY($a) + $nodeY($b)]
	set midY [expr $midY / 2 ]

	set TransLabel($a,$b) \
	[.f$cnr.c create text $midX $midY -font $HelvBig -tags texttag]

	.f$cnr.c bind $edge <Button-1> "
		.f$cnr.c coords $TransLabel($a,$b) \[.f$cnr.c canvasx %x\] \[.f$cnr.c canvasy %y\]
		.f$cnr.c itemconfigure $TransLabel($a,$b) \
			-fill black -text {$L} -font $HelvBig
	"
	.f$cnr.c bind $edge <ButtonRelease-1> "
		.f$cnr.c itemconfigure $TransLabel($a,$b) \
			-fill black -text {}
	"
}

proc moveNode {cnr node mx my together} {
	global edgeHead edgeTail TLabel NodeWidth NodeHeight
	global nodeX nodeY Lab2Node
	global Xrem Yrem Scale

	set x [.f$cnr.c coords $node]
	if {[llength $x] == 2} { set node $Lab2Node($node) }

	set mx [.f$cnr.c canvasx $mx]
	set my [.f$cnr.c canvasy $my]

	set wx $NodeWidth($node)
	set wy $NodeHeight($node)

	.f$cnr.c coords $node \
		[expr $mx-$wx] [expr $my-$wy] \
		[expr $mx+$wx] [expr $my+$wy]
	.f$cnr.c coords $TLabel($node) $mx $my

	set nodeX($node) $mx
	set nodeY($node) $my
	if {$together == 0} { return }
	catch {
	foreach edge $edgeHead($node) {
		set ec [.f$cnr.c coords $edge]
		set nx [expr $nodeX($node) + $Xrem($edge)*$Scale]
		set ny [expr $nodeY($node) + $Yrem($edge)*$Scale]
		.f$cnr.c coords $edge \
			$nx $ny \
			[lindex $ec 2] [lindex $ec 3]
	}}
	catch {
	foreach edge $edgeTail($node) {
		set ec [.f$cnr.c coords $edge]
		set nx [expr $nodeX($node) + $Xrem($edge)*$Scale]
		set ny [expr $nodeY($node) + $Yrem($edge)*$Scale]
		.f$cnr.c coords $edge \
			[lindex $ec 0] [lindex $ec 1] \
			$nx $ny
	}}
}

set PlaceCanvas(msc)	""

proc mkcanvas {n m geox geoy placed} {
	global cnr tk_version
	global Chandle Sticky
	global FG BG Ptype PlaceCanvas

	incr cnr
	toplevel .f$cnr
	wm title .f$cnr "$n"
	wm iconname .f$cnr $m
	if {$placed} {
		if {$PlaceCanvas($m) == ""} {
			set PlaceCanvas($m) "+$geox+$geoy"
		}
		set k [string first "\+" $PlaceCanvas($m)]
		if {$k > 0} {
		set PlaceCanvas($m) [string range $PlaceCanvas($m) $k end]
		}
		wm geometry .f$cnr $PlaceCanvas($m)
	}
	wm minsize .f$cnr 100 100

	if {[string first "4." $tk_version] == 0 \
	||  [string first "8." $tk_version] == 0} {
		set cv [canvas .f$cnr.c  -relief raised -bd 2\
			-scrollregion {-15c -5c 30c 40c} \
			-background $BG \
			-xscrollcommand ".f$cnr.hscroll set" \
			-yscrollcommand ".f$cnr.vscroll set"]
		scrollbar .f$cnr.vscroll -relief sunken \
			-command ".f$cnr.c yview"
		scrollbar .f$cnr.hscroll -relief sunken -orient horiz \
			-command ".f$cnr.c xview"
	} else {
		set cv [canvas .f$cnr.c  -relief raised -bd 2\
			-scrollregion {-15c -5c 30c 40c} \
			-background $BG \
			-xscroll ".f$cnr.hscroll set" \
			-yscroll ".f$cnr.vscroll set"]
		scrollbar .f$cnr.vscroll -relief sunken \
			-command ".f$cnr.c yview"
		scrollbar .f$cnr.hscroll -relief sunken -orient horiz \
			-command ".f$cnr.c xview"
	}
	set Chandle($cnr) $cv
	#-width 500 -height 500

	button .f$cnr.b1 -text "Close" \
		-command "set PlaceCanvas($m) [wm geometry .f$cnr]; destroy .f$cnr"
	button .f$cnr.b2 -text "Save in: $m.ps" \
		-command "$cv postscript -file $m.ps -colormode $Ptype"

	pack append .f$cnr \
		.f$cnr.vscroll {right filly} \
		.f$cnr.hscroll {bottom fillx} \
		.f$cnr.c {top expand fill}

	if {$m == "msc"} {
		set Sticky($cnr) 0
		checkbutton .f$cnr.b6 -text "Preserve" \
			-variable Sticky($cnr) \
			-relief flat
		pack append .f$cnr \
			.f$cnr.b6 { right padx 5}
	}
	pack append .f$cnr \
		.f$cnr.b1 {right padx 5} \
		.f$cnr.b2 {right padx 5}

	bind $cv <2> "$cv scan mark %x %y"	;# Geert Janssen, TUE
	bind $cv <B2-Motion> "$cv scan dragto %x %y"

	.f$cnr.c bind node <B1-Motion> "
		moveNode $cnr \[.f$cnr.c find withtag current] %x %y 1
	"

#	.f$cnr.c bind node <B2-Motion> "
#		moveNode $cnr \[.f$cnr.c find withtag current] %x %y 1
#	"

	tkwait visibility .f$cnr
	return $cnr
}

proc addscales {p} {
	global Chandle Scale

	catch {
		set cv $Chandle($p)
		button .f$p.b4 -text "Larger" \
			-command "$cv scale all 0 0 1.1 1.1; set Scale [expr $Scale*1.1]"
		button .f$p.b5 -text "Smaller" \
			-command "$cv scale all 0 0 0.9 0.9; set Scale [expr $Scale*0.9]"
		pack append .f$p \
			.f$p.b4 {right padx 5} \
			.f$p.b5 {right padx 5}
	}
}

proc dodot {n} {
	global Edges edgeHead edgeTail NodeWidth NodeHeight Maxx Maxy
	global State ELabel TransLabel Unix cnr lcnr Xrem Yrem DOT

	add_log "<dot graph layout...>"
	set fd [open "|$DOT" w+]

	puts $fd "digraph dodot {"

	foreach el [array names Edges] {
		if { [ string first $n $el ] >= 0 } {
		for {set i 0} { [lindex $Edges($el) $i] != "" } {incr i} {
			set ntrg [lindex $Edges($el) $i]
			puts $fd " \"$el\" -> \"$ntrg\"; "
		}
	}}

	puts $fd "}"
	flush $fd
	set ends ""
	set nodot 1
	while {[gets $fd line] > -1} {
		if {[string first "\}" $line] >= 0} {
			break;
		}
		set dd [string length $line]; incr dd -1
		while {[string range $line $dd $dd] == "\\"} {
			gets $fd more
			set line "[string range $line 0 [expr $dd-1]]$more"
			set dd [string length $line]; incr dd -1
		}
		if {[string first " -> " $line] >= 0} {
			set a [string first "\[pos=\"" $line]; incr a 8
			set b [string first "\"\];" $line]; incr b -1
			set b2 [string first "->" $line]
			set line1 [string range $line 0 [expr $a - 9]]
			set src [string range $line1 0 [expr $b2 - 1]]
			set src [string trim $src "	 \""]
			set trg [string range $line1 [expr $b2 + 3] [expr $a - 1]]
			set trg [string trim $trg "	 \""]
			set tp [string range $line [expr $a-2] [expr $a-2]]
			set line [string range $line $a $b]
			set k [split $line " "]
			set j [llength $k]
			set j2 [trunc [expr $j/2]]
			set coords ".f$cnr.c create line "
			set spline "-smooth 1"
			set nl $ELabel($src,$trg)
			if {$tp == "e"} {
				set ends "last"
				for {set i 1} {$i < $j} {incr i} {
					scan [lindex $k $i] "%d,%d" x y
					set coords " $coords[expr 50 + $x] [expr 50 + $Maxy - $y] "
					if {$i == $j2} {
						.f$cnr.c coords \
							$TransLabel($State($src),$State($trg)) \
							[expr 50 + $x] [expr 50 + $Maxy - $y]
						.f$cnr.c itemconfigure \
							$TransLabel($State($src),$State($trg)) \
							-fill black -text "$nl"
					}
				}
				scan [lindex $k 0] "%d,%d" x y
				set coords " $coords[expr 50 + $x] [expr 50 + $Maxy - $y] "
			} else {
				set ends "first"
				for {set i 0} {$i < $j} {incr i} {
					scan [lindex $k $i] "%d,%d" x y
					set coords " $coords[expr 50 + $x] [expr 50 + $Maxy - $y] "
					if {$i == $j2} {
						.f$cnr.c coords \
							$TransLabel($State($src),$State($trg)) \
							[expr 50 + $x] [expr 50 + $Maxy - $y]
						.f$cnr.c itemconfigure \
							$TransLabel($State($src),$State($trg)) \
							-fill black -text "$nl"
			}	}	}
			set coords "$coords -arrow $ends $spline -tags connector"

			set ne [eval $coords]
			set Xrem($ne) 10
			set Yrem($ne) 10

			continue
		}
		if {[string first "node " $line] >= 0 \
		||  [string first "\{" $line]    >= 0} {
			continue
		}
		if {[string first "graph " $line] >= 0} {
			set a [string first "\"" $line]; incr a
			set b [string last  "\"" $line]; incr b -1
			set line [string range $line $a $b]
			set k [split $line ","]
			if {[llength $k] == 4} {
				set Maxx [lindex $k 2]
				set Maxy [lindex $k 3]
			} else {
				set Maxx [lindex $k 0]
				set Maxy [lindex $k 1]
			}
			set nodot 0
		} else {	;# a node
			set a [string first "\[" $line]; incr a
			set b [string last  "\]" $line]; incr b -1
			set line1 [string range $line 0 [expr $a - 2]]
			set line  [string range $line $a $b]
			set nn [string trim $line1 "	 \""]

			set a [string first "pos=" $line]; incr a 5
			set b [string first "\"" [string range $line $a end]]
			set b [expr $a + $b - 1]
			set line1  [string range $line $a $b]
			set k [split $line1 ","]
			set x [lindex $k 0]
			set y [lindex $k 1]

			set a [string first "width=" $line]; incr a 7
			set b [string first "\"" [string range $line $a end]]
			set b [expr $a + $b - 1]
			set w [expr 75 * [string range $line $a $b]]

			set a [string first "height=" $line]; incr a 8
			set b [string first "\"" [string range $line $a end]]
			set b [expr $a + $b - 1]
			set h [expr 75 * [string range $line $a $b]]

		catch {
			set NodeWidth($State($nn)) [expr $w/2]
			set NodeHeight($State($nn)) [expr $h/2]
			moveNode $lcnr $State($nn) \
				[expr 50 + $x] [expr 50 + $Maxy - $y] 0
		} err
#puts $err
		}
	}
	catch { close $fd }

	if {$nodot} {
		add_log "<cannot find dot>"
		catch {
		tk_messageBox -icon info -message "<cannot find dot>"
		}
		return
	}

	foreach el [array names Edges] {
		if { [ string first $n $el ] >= 0 } {
			catch {
			foreach edge $edgeHead($State($el)) {
				.f$cnr.c delete $edge
			}
			unset edgeHead($State($el))
			unset edgeTail($State($el))
			}
	}	}
	.f$cnr.c bind node <B1-Motion> {}	;# no moving
	.f$cnr.c bind node <B2-Motion> {}
	catch { destroy .f$lcnr.b6 }
#	button .f$lcnr.b6 -text "No Labels" \
#		-command ".f$lcnr.c delete texttag; destroy .f$lcnr.b6"
	button .f$lcnr.b6 -text "No Labels" \
		-command "hide_automata_labels .f$lcnr.b6 .f$cnr.c"
	pack append .f$lcnr .f$lcnr.b6 {right padx 5}
}

proc hide_automata_labels {b c} {
	$b configure -text "Add Labels"
	$c itemconfigure texttag -fill white
	$b configure -command "show_automata_labels $b $c"
}

proc show_automata_labels {b c} {
	$b configure -text "No Labels"
	$c itemconfigure texttag -fill black
	$b configure -command "hide_automata_labels $b $c"
}

proc trunc {p} {
	set foo [string first "." $p]
	if {$foo >= 0} {
		incr foo -1
		set p [string range $p 0 $foo]
	}
	return $p
}

# Help menus

proc aboutspin {} {
	global FG BG HelvBig version xversion

	catch {destroy .h}
	toplevel .h

	wm title .h "About SPIN"
	wm iconname .h "About"
	message .h.t -width 600 -background $BG -foreground $FG -font $HelvBig \
	-text " $version
Xspin Version $xversion

Spin is an on-the-fly LTL model checking system
for proving properties of asynchronous software
system designs, first distributed in 1991.

The master sources for the latest version of
this software can always be found via:

http://spinroot.com/spin/whatispin.html

For help:  spin_list@spinroot.com

The Spin sources are (c) 1990-2004 Bell Labs,
Lucent Technologies, Murray Hill, NJ, USA.
All rights are reserved. This software is for
educational and research purposes only.
No guarantee whatsoever is expressed or implied
by the distribution of this code.
"
	button .h.b -text "Ok" -command {destroy .h}
	pack append .h .h.t {top expand fill}
	pack append .h .h.b {top}
}

proc promela {} {
	global FG BG HelvBig

	catch {destroy .h}
	toplevel .h

	wm title .h "Promela URL"
	wm iconname .h "Promela"
	message .h.t -width 600 -background $BG -foreground $FG -font $HelvBig \
	-text "All Promela references are available online:

http://spinroot.com/spin/Man/index.html

"
	button .h.b -text "Ok" -command {destroy .h}
	pack append .h .h.t {top expand fill}
	pack append .h .h.b {top}
}

proc helper {} {
	global FG BG HelvBig

	catch {destroy .h}
	toplevel .h

	wm title .h "Help with Xspin"
	wm iconname .h "Help"
	message .h.t -background $BG -foreground $FG -font $HelvBig \
	-text "\
Spin Version Controller - (c) 1993-2004 Bell Laboratories

Enter a Promela model into the main text window, or 'Open'
one via the File Menu (e.g., from Spin's Test directory).
Once loaded, you can revert to the stored version of the file
with option ReOpen.  Select Clear to empty the text window.

In the log, just below the text-window, background
commands are printed that Xspin generates.
Outputs from Simulation and Verification runs always
appear in separate windows.

All run-time options are available through the Run menu.
A typical way of working with Xspin is to use:

- First a Syntax Check to get hints and warnings
- Random Simulation for further debugging
- Add the properties to be verified (assertions, never claims)
- Perform a Slicing Check to check for redundancy
- Perform Verification for a correctness proof
- Guided Simulation to inspect errors reported by
  the Verification option

Clicking Button-1 in the main window updates the
Line number display at the top of the window -- as a
simple way of finding out at what line you are.

You can also use another editor to update the
specifications outside Xspin, and use the ReOpen
command from the File menu to refresh the Xspin
edit buffer before starting each new simulation or
verification run."
	button .h.b -text "Ok" -command {destroy .h}
	pack append .h .h.t {top expand fill}
	pack append .h .h.b {top}
}

# LTL interface

set formula ""
set tl_stat 0

proc put_template {s} {
	.tl.main.e1 delete 0 end
	.tl.main.e1 insert end "$s"
}

set PlaceTL	"+100+1"

proc call_tl {} {	;# expanded interface
	global formula tl_stat nv_typ an_typ cp_typ
	global FG BG Fname firstime PlaceTL

	catch {destroy .tl}
	toplevel .tl

	set k [string first "\+" $PlaceTL]
	if {$k > 0} {
		set PlaceTL [string range $PlaceTL $k end]
	}

	wm title .tl "Linear Time Temporal Logic Formulae"
	wm iconname .tl "LTL"
	wm geometry .tl $PlaceTL

	frame .tl.main
	entry .tl.main.e1 -relief sunken \
		-background $BG -foreground $FG
	label .tl.main.e2 -text "Formula: "

	frame .tl.op
	set alw {\[\] }
	set eve {\<\> }
	pack append .tl.op [label .tl.op.s0 -text "Operators: " \
		-relief flat] {left}
	pack append .tl.op [button .tl.op.always -width 1 -text "\[\]" \
		-command ".tl.main.e1 insert insert \"$alw \""] {left}
	pack append .tl.op [button .tl.op.event -width 1 -text "\<\>" \
		-command ".tl.main.e1 insert insert \"$eve \""] {left}
	pack append .tl.op [button .tl.op.until -width 1 -text "U" \
		-command ".tl.main.e1 insert insert \" U \""] {left}
	pack append .tl.op [button .tl.op.impl -width 1 -text "->" \
		-command ".tl.main.e1 insert insert \" -> \""] {left}
	pack append .tl.op [button .tl.op.and -width 1 -text "and" \
		-command ".tl.main.e1 insert insert \" && \""] {left}
	pack append .tl.op [button .tl.op.or -width 1 -text "or" \
		-command ".tl.main.e1 insert insert \" || \""] {left}
	pack append .tl.op [button .tl.op.not -width 1 -text "not" \
		-command ".tl.main.e1 insert insert \" ! \""] {left}

	frame .tl.b -relief ridge -borderwidth 4
	label .tl.b.s0 -text "Property holds for: "
	radiobutton .tl.b.s1 -text "All Executions (desired behavior)" \
		-variable tl_stat -value 0
	radiobutton .tl.b.s2 -text "No Executions (error behavior)" \
		-variable tl_stat -value 1
	pack append .tl.b \
		.tl.b.s0 {left} \
		.tl.b.s1 {left} \
		.tl.b.s2 {left}

	.tl.main.e1 insert end $formula

	button .tl.main.file -text "Load..." \
		-command "browse_tl"

	bind .tl.main.e1 <Return> { do_ltl }

	pack append .tl.main \
		.tl.main.e2 {top left}\
		.tl.main.e1 {top left expand fill} \
		.tl.main.file {top right}

	pack append .tl .tl.main {top fillx frame e}
	pack append .tl .tl.op {top frame w}
	pack append .tl .tl.b {top fillx frame w}

	frame .tl.macros -relief ridge -borderwidth 4
	label .tl.macros.title -text "Symbol Definitions:" -relief flat
	scrollbar .tl.macros.s -relief flat \
		-command ".tl.macros.t yview"
	text .tl.macros.t -height 4 -relief raised -bd 2 \
		-yscrollcommand ".tl.macros.s set" \
		-background $BG -foreground $FG \
		-setgrid 1 \
		-wrap word

	pack append .tl.macros \
		.tl.macros.title {top frame w} \
		.tl.macros.s {left filly} \
		.tl.macros.t {left expand fill}

	frame .tl.notes -relief ridge -borderwidth 4
	label .tl.notes.title -text "Notes: " -relief flat
	scrollbar .tl.notes.s -relief flat \
		-command ".tl.notes.t yview"
	text .tl.notes.t -height 4 -relief raised -bd 2 \
		-yscrollcommand ".tl.notes.s set" \
		-background $BG -foreground $FG \
		-setgrid 1 \
		-wrap word
	pack append .tl.notes \
		.tl.notes.title {top fillx frame w} \
		.tl.notes.s {left filly} \
		.tl.notes.t {left expand fill}

	frame .tl.never
	frame .tl.never.top
	label .tl.never.top.title -text "Never Claim:"\
		 -relief flat
	button .tl.never.top.gc -text "Generate" \
		-command "do_ltl"
	pack append .tl.never.top \
		.tl.never.top.gc {right}\
		.tl.never.top.title {left}

	scrollbar .tl.never.s -relief flat \
		-command ".tl.never.t yview"
	text .tl.never.t -height 8 -relief raised -bd 2 \
		-yscrollcommand ".tl.never.s set" \
		-setgrid 1 \
		-wrap word
	pack append .tl.never \
		.tl.never.top {top fillx frame w} \
		.tl.never.s {left filly} \
		.tl.never.t {left expand fill}

	frame .tl.results
	frame .tl.results.top

	button .tl.results.top.svp -text "Run Verification" \
		-command "do_ltl; basicval2"
	label .tl.results.top.title -text "Verification Result:"\
		 -relief flat
	pack append .tl.results.top \
		.tl.results.top.svp {right}\
		.tl.results.top.title {left}

	scrollbar .tl.results.s -relief flat \
		-command ".tl.results.t yview"
	text .tl.results.t -height 7 -relief raised -bd 2 \
		-yscrollcommand ".tl.results.s set" \
		-setgrid 1 \
		-wrap word
	pack append .tl.results \
		.tl.results.top {top fillx frame w} \
		.tl.results.s {left filly} \
		.tl.results.t {left expand fill}

	pack append .tl \
		.tl.notes {top expand fill} \
		.tl.macros {top expand fill} \
		.tl.never {top expand fill} \
		.tl.results {top expand fill} \

	pack append .tl [button .tl.sv -text "Save As.." \
		-command "save_tl"] {right}
	pack append .tl [button .tl.exit -text "Close" \
	-command "set PlaceTL [wm geometry .tl]; destroy .tl"] {right}

	pack append .tl [button .tl.help -text "Help" -fg red \
		-command "roadmap4"] {left}
	pack append .tl [button .tl.clear -text "Clear" \
		-command ".tl.main.e1 delete 0 end; .tl.never.t delete 0.0 end"] {left}

	loaddefault_tl
	focus .tl.main.e1
}

proc purge_nvr {foo} {
	set j [llength $foo]; incr j -1
	for {set i $j} {$i >= 0} {incr i -1} {
		set k [lindex $foo $i]
		set kk [expr $k+1]
		.tl.never.t delete $k.0 $kk.0
	}
}

proc grab_nvr {inp target} {

	set pattern $inp
	scan [.tl.never.t index end] %d numLines
	set foo {}
	set yes 0

	for {set i 1} {$i < $numLines} { incr i} {
		.tl.never.t mark set last $i.0
		set have [.tl.never.t get last "last lineend + 1 chars"]
		if {[regexp -indices $pattern $have indices]} {
			lappend foo $i
			set yes [expr 1 - $yes]
			if {$yes} {
				set pattern "#endif"
			} else {
				set pattern $inp
			}
		}
		if {$yes && [string first $inp $have] != 0} {
			$target insert end "$have"
			lappend foo $i
		}
	}
	purge_nvr $foo
}

proc extract_defs {} {
	global tl_stat

	set pattern "#define "
	scan [.tl.never.t index end] %d numLines
	set foo {}
	set tl_stat 1
	for {set i 1} {$i < $numLines} { incr i} {
		.tl.never.t mark set last $i.0
		set have [.tl.never.t get last "last lineend + 1 chars"]
		if {[regexp -indices $pattern $have indices]} {
			.tl.macros.t insert end "$have"
			lappend foo $i
		}
		set have [.tl.never.t get last "last lineend"]
		set k [string first "Formula As Typed: " $have]
		if {$k > 0} {
			set ff [string range $have [expr $k+18] end]
			.tl.main.e1 insert end $ff
		}
		if {[string first "To The Negated Formula " $have] > 0} {
			set tl_stat 0
		}
	}
	purge_nvr $foo

	grab_nvr "#ifdef NOTES"  .tl.notes.t
	grab_nvr "#ifdef RESULT" .tl.results.t
}

proc inspect_ltl {} {
	global formula
	set formula "[.tl.main.e1 get]"

	set x $formula
	regsub -all {\&\&} "$x" " " y; set x $y
	regsub -all {\|\|} "$x" " " y; set x $y
	regsub -all {\/\\} "$x" " " y; set x $y
	regsub -all {\\\/} "$x" " " y; set x $y
	regsub -all {\!}  "$x" " " y; set x $y
	regsub -all {<->} "$x" " " y; set x $y
	regsub -all {\->}  "$x" " " y; set x $y
	regsub -all {\[\]} "$x" " " y; set x $y
	regsub -all {\<\>} "$x" " " y; set x $y
	regsub -all {[()]} "$x" " " y; set x $y
	regsub -all {\ \ *} "$x" " " y; set x $y
	regsub -all { U} "$x" " " y; set x $y
	regsub -all { V} "$x" " " y; set x $y
	regsub -all { X} "$x" " " y; set x $y

	set predefs " np_ true false "

	set k [split $x " "]
	set j [llength $k]
	set line [.tl.macros.t get 0.0 end]
	for {set i 0} {$i < $j} {incr i} {
		if {[string length [lindex $k $i]] > 0 \
		&&  [string first " [lindex $k $i] " $predefs] < 0} {
		  set pattern "#define [lindex $k $i]"
		  if {[string first $pattern $line] < 0} {
			catch {
			.tl.macros.t insert end "$pattern\t?\n"
			}
			set line [.tl.macros.t get 0.0 end]
	}	} }
}

proc do_ltl {} {
	global formula tl_stat SPIN tk_major tk_minor

	set formula "[.tl.main.e1 get]"
	.tl.never.t delete 0.0 end
	update

	catch { inspect_ltl }

	set MostSystems 1	;# change to 0 only if there are problems
				;# see below

	if {$tl_stat == 0} {
		add_log "$SPIN -f \"!( $formula )\""
		if {$MostSystems} {
			catch {exec $SPIN -f "!($formula)" >&pan.ltl} err
		} else {
			# this variant was needed on some older systems,
			# but it causes problems on some of the newer ones...
			catch {exec $SPIN -f \"!($formula)\" >&pan.ltl} err
		}
	} else {
		add_log "$SPIN -f \"( $formula )\""
		if {$MostSystems} {
		catch {exec $SPIN -f "($formula)" >&pan.ltl} err
		} else {
		# see above
		catch {exec $SPIN -f \"($formula)\" >&pan.ltl} err
		}
	}
	set lno 0

	if {$err != ""} {
		add_log "<error: $err>"
		add_log "hint: check the Help Button for syntax rules"
	} else {
		.tl.never.t insert end \
		"	/*\n"
		.tl.never.t insert end \
		"	 * Formula As Typed: $formula\n"
		incr lno 2
		if {$tl_stat == 0} {
			.tl.never.t insert end \
			"	 * The Never Claim Below Corresponds\n"
			.tl.never.t insert end \
			"	 * To The Negated Formula !($formula)\n"
			.tl.never.t insert end \
			"	 * (formalizing violations of the original)\n"
			incr lno 3
		}
		.tl.never.t insert end \
		"	 */\n\n"
		incr lno 2
	}
	catch {
		set fd [open pan.ltl r]
		while {[gets $fd line] > -1} {
			.tl.never.t insert end "$line\n"
		}
		close $fd
	}
	rmfile pan.ltl
}

proc dump_tl {bb} {

	if {$bb != ""} {
		set fnm $bb
	} else {
		set fnm [.sv_tl.ent get]
	}

	if {[file_ok $fnm]==0} return
	set fd [open $fnm w]
	add_log "<save claim in $fnm>"
	catch {	puts $fd "[.tl.macros.t get 0.0 end]" nonewline }

	puts $fd "[.tl.never.t get 0.0 end]" nonewline

	catch {	puts $fd "#ifdef NOTES"
		puts $fd "[.tl.notes.t get 0.0 end]" nonewline
		puts $fd "#endif"
	}
	catch {	puts $fd "#ifdef RESULT"
		puts $fd "[.tl.results.t get 0.0 end]" nonewline
		puts $fd "#endif"
	}

	catch "flush $fd"
	catch "close $fd"
	catch "destroy .sv_tl"
	catch "focus .tl.main.e1"
}

proc save_tl {} {
	global Fname PlaceWarn
	catch {destroy .sv_tl}
	toplevel .sv_tl

	wm title .sv_tl "Save Claim"
	wm iconname .sv_tl "Save"
	wm geometry .sv_tl $PlaceWarn

	label  .sv_tl.msg -text "Name for LTL File: " -relief flat
	entry  .sv_tl.ent -width 6 -relief sunken -textvariable fnm
	button .sv_tl.b1 -text "Ok" -command { dump_tl "" }
	button .sv_tl.b2 -text "Cancel" -command "destroy .sv_tl"
	bind   .sv_tl.ent <Return> { dump_tl "" }

	set fnm [.sv_tl.ent get]
	if {$fnm == ""} {
		.sv_tl.ent insert end "$Fname.ltl"
	}

	pack append .sv_tl \
		.sv_tl.msg {top frame w} \
		.sv_tl.ent {top frame e expand fill} \
		.sv_tl.b1 {right frame e} \
		.sv_tl.b2 {right frame e}
	focus .sv_tl.ent
}

proc add_tl {} {
	global BG FG HelvBig PlaceWarn
	catch {destroy .warn}
	toplevel .warn
	set k [string first "\+" $PlaceWarn]
	if {$k > 0} {
		set PlaceWarn [string range $PlaceWarn $k end]
	}

	wm title .warn "Accept"
	wm iconname .warn "Accept"
	wm geometry .warn $PlaceWarn

	message .warn.t -width 300 \
		-background $BG -foreground $FG -font $HelvBig \
		-text " \
Instructions:

1. Save the Never Claim in a file, \
for instance a file called 'never', \
using the <Save> button.

2. Insert the line

#include \"never\"

(the name of the file with the claim) \
at the end of the main specification.

3. Insert macro definitions (#define's) for all \
propositional symbols used in the formula.

For instance, with LTL formula
'[] p -> <> q'  add the macro defs:

#define p	(cnt == 1)
#define q	(cnt > 1)

These macros must be defined just above the line \
with the #include \"never\" directive

4. Perform the verification, and make sure that \
the box 'Apply Never Claim' is checked in the \
Verification Panel (or else the claim is ignored).
You can have a library of claim files that you can \
choose from for verification, by changing only the \
name of the include file.

5. Never claims have no effect during simulation runs.

6. See the HELP->LTL menu for more information.

"
	button .warn.b -text "Ok" \
		-command {set PlaceWarn [wm geometry .warn]; destroy .warn}
	pack append .warn .warn.t {top expand fill}
	pack append .warn .warn.b {right frame e}
}

proc roadmap4 {} {
	global FG BG

	catch {destroy .h}
	toplevel .h

	wm title .h "LTL Help"
	wm iconname .h "Help"
	frame .h.z
	scrollbar .h.z.s -command ".h.z.t yview"
	text .h.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".h.z.s set" \
		-setgrid 1 -width 60 -height 30 -wrap word
	pack append .h.z \
		.h.z.s {left filly} \
		.h.z.t {left expand fill}
	.h.z.t insert end "GUIDELINES:
You can load an LTL template or a previously saved LTL
formula from a file via the LOAD button on the upper
right of the LTL Property Manager panel.

Define a new LTL formula using lowercase names for the
propositional symbols, for instance:
	[] (p U q)
The formula expresses either a positive (desired) or a
negative (undesired) property of the model.  A positive
property is negated automatically by the translator to
convert it in a never claim (which expresses the
corresponding negative property (the undesired behavior
that is claimed 'never' to occur).

When you type a <Return> or hit the <Generate> button,
the formula is translated into an equivalent never-claim.

You must add a macro-definition for each propositional
symbol used in the LTL formula.  Insert these definitions
in the symbols window of the LTL panel, they will be
remembered together with the annotations and verification
result if the formula is saved in an .ltl file.
Enclose the symbol definitions in braces, to secure correct
operator precedence, for instance:

#define p	(a > b)
#define q	(len(q) < 5)

Valid temporal logic operators are:
	\[\]  Always (no space between \[ and \])
	<>  Eventually (no space between < and >)
	U   (Strong) Until
	V   The Dual of Until: (p V q) == !(!p U !q)

	All operators are left-associative (incl. U and V).

Boolean Operators:
	&&  Logical And (alternative form: /\\, no spaces)
	!   Logical Negation
	||  Logical Or  (alternative form: \\/, no spaces)
	->  Logical Implication
	<-> Logical Equivalence

Boolean Predicates:
	true, false
	any name that starts with a lowercase letter

Examples:
	\[\] p
	!( <> !q )
	p U q
	p U (\[\] (q U r))

Generic properties/Templates:
	Invariance: \[\] p
	Response:   p -> \<\> q
	Precedence: p -> (q U r)
	Objective:  p -> \<\> (q || r)

	Each of the above 4 generic types of properties
	can (and will generally have to) be prefixed by
	temporal operators such as
		\[\], \<\>, \[\]\<\>, \<\>\[\]
	The last (objective) property can be read to mean
	that 'p' is a trigger, or 'enabling' condition that
	determines when the requirement becomes applicable
	(e.g. the sending of a new data message); then 'q'
	can be the fullfillment of the requirement (e.g.
	the arrival of the matching acknowledgement), and
	'r' could be a discharging condition that voids the
	applicability of the check (an abort condition).
"
	button .h.b -text "Ok" -command {destroy .h}
	pack append .h .h.z {top expand fill}
	pack append .h .h.b {top}
	.h.z.t configure -state disabled 
	.h.z.t yview -pickplace 1.0      
	focus .h.z.t
}



# Specific Help

proc roadmap1 {} {
	global FG BG

	catch {destroy .road1}
	toplevel .road1

	wm title .road1 "Help with Simulation"
	wm iconname .road1 "SimHelp"
	frame .road1.z
	scrollbar .road1.z.s -command ".road1.z.t yview"
	text .road1.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road1.z.s set" \
		-setgrid 1 -width 60 -height 30 -wrap word
	pack append .road1.z \
		.road1.z.s {left filly} \
		.road1.z.t {left expand fill}
	.road1.z.t insert end "GUIDELINES:
0.  Always use a Syntax Check before running simulations.\
It shakes out the more obvious flaws in the model.

1.  Random simulation option is used to debug a model.\
Other than assert statements, no correctness requirements\
are checked during simulation runs. All nondeterministic\
decisions are resolved randomly.  You can of course still\
force a selection to go into a specific direction by \
modifying the model.\
A random run is repeated precisely if the Seed Value\
for the random number generator is kept the same.

2.  Interactive simulation can be used to force the\
execution towards a known point.  The user is prompted\
at every point in the execution where a nondeterministic\
choice has to be resolved.

3.  A guided simulation is used to follow an error-trail that was\
produced by the verifier.  If the trail gets to be thousands of execution\
steps long, this can be time-consuming. \
You can skip the initial portion of such a long trail by typing\
a number in  the 'Steps Skipped' box in the Simulation Panel .

4. The options in the Simulations Panel allow you to enable or\
disable types of displays that are maintained during simulation\
runs.  Usually, it is not necessary to look at all possible display panels.\
Experiment to see which displays you find most useful.

5. To track the value changes of Selected variables, in the\
Message Sequence Chart and in the Variable Values Panel, add a prefix\
'show ' to the declaration of the selected variables in the Promela\
specification, e.g. use 'show byte cnt;' instead of 'byte cnt;'"

	button .road1.b -text "Ok" -command {destroy .road1}
	pack append .road1 .road1.z {top expand fill}
	pack append .road1 .road1.b {top}
	.road1.z.t configure -state disabled 
	.road1.z.t yview -pickplace 1.0      
	focus .road1.z.t
}

proc roadmap2a {} {
	global FG BG

	catch {destroy .road2}
	toplevel .road2

	wm title .road2 "Help with Verification Parameters"
	wm iconname .road2 "ValHelp"
	frame .road2.z
	scrollbar .road2.z.s -command ".road2.z.t yview"
	text .road2.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road2.z.s set" \
		-setgrid 1 -width 60 -height 18 -wrap word
	pack append .road2.z \
		.road2.z.s {left filly} \
		.road2.z.t {left expand fill}
	.road2.z.t insert end "Physical Memory Available:
Set this number to amount of physical (not virtual)
memory in your system, in MegaBytes, and leave it there for all runs.

When the limit is reached, the verification is stopped to
avoid trashing.

The number entered here is the number of MegaBytes directly
(not a power of two, as in previous versions of xspin).

If an exhaustive verification cannot be completed due to
lack of memory, use compression, or switch to a
supertrace/bitstate run under basic options."

	button .road2.b -text "Ok" -command {destroy .road2}
	pack append .road2 .road2.z {top expand fill}
	pack append .road2 .road2.b {top}
	.road2.z.t configure -state disabled 
	.road2.z.t yview -pickplace 1.0      
	focus .road2.z.t
}
proc roadmap2b {} {
	global FG BG

	catch {destroy .road2}
	toplevel .road2

	wm title .road2 "Help with Verification"
	wm iconname .road2 "ValHelp"
	frame .road2.z
	scrollbar .road2.z.s -command ".road2.z.t yview"
	text .road2.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road2.z.s set" \
		-setgrid 1 -width 60 -height 15 -wrap word
	pack append .road2.z \
		.road2.z.s {left filly} \
		.road2.z.t {left expand fill}
	.road2.z.t insert end "Estimated State Space Size:
This parameter is used to calculate the size of the
hash-table. It results in a selection of a numeric argument
for the -w flag of the verifier. Setting it too high may
cause an out-of-memory error with zero states reached
(meaning that the verification could not be started).
Setting it too low can cause inefficiencies due to
hash collisions.

In Bitstate runs begin with the default estimate for
this parameter.  After a run completes successfully,
double the estimate, and see if the number of reached
stated changes much.  Continue to do this until
it stops changing, or until you overflow the memory
bound and run out of memory.

The closest power of two is taken by xspin to set the
true number used for the number of reachable states.
(The selected power of two is visible as the number that
follow the -w flag in the run-line that xspin generates).

To set a specific -w parameter, use the Extra Run-Time Option
Field. If, for instance, -w32 is specified there, it will
override the value computed from the Estimated State Space Size."

	button .road2.b -text "Ok" -command {destroy .road2}
	pack append .road2 .road2.z {top expand fill}
	pack append .road2 .road2.b {top}
	.road2.z.t configure -state disabled 
	.road2.z.t yview -pickplace 1.0      
	focus .road2.z.t
}
proc roadmap2c {} {
	global FG BG

	catch {destroy .road2}
	toplevel .road2

	wm title .road2 "Help with Verification"
	wm iconname .road2 "ValHelp"
	frame .road2.z
	scrollbar .road2.z.s -command ".road2.z.t yview"
	text .road2.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road2.z.s set" \
		-setgrid 1 -width 60 -height 20 -wrap word
	pack append .road2.z \
		.road2.z.s {left filly} \
		.road2.z.t {left expand fill}
	.road2.z.t insert end "Maximum Search Depth:
This number determines the size of the depth-first
search stack that is used during the verification.
The stack uses memory, so a larger number increases
the memory requirements, and a lower number decreases
it.  In a tight spot, when there seems not to be
sufficient memory for the search depth needed, you
can reduce the estimated state space size to free some
more memory for the stack.

If you hit the maximum search depth during a verification
(noted as 'Search not completed' or 'Search Truncated'
in the verification output) without finding an error,
double the search depth parameter and repeat the run."

	button .road2.b -text "Ok" -command {destroy .road2}
	pack append .road2 .road2.z {top expand fill}
	pack append .road2 .road2.b {top}
	.road2.z.t configure -state disabled 
	.road2.z.t yview -pickplace 1.0      
	focus .road2.z.t
}

proc roadmap2k {} {
	global FG BG

	catch {destroy .road2}
	toplevel .road2

	wm title .road2 "Help with Verification"
	wm iconname .road2 "ValHelp"
	frame .road2.z
	scrollbar .road2.z.s -command ".road2.z.t yview"
	text .road2.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road2.z.s set" \
		-setgrid 1 -width 60 -height 10 -wrap word
	pack append .road2.z \
		.road2.z.s {left filly} \
		.road2.z.t {left expand fill}
	.road2.z.t insert end "Number of hash functions:
This number determines how many bits are set per
state when in Bitstate verification mode. The default is 2.
At the end of a Bitstate verification run, the verifier
can issue a recommendation for a different setting of
this flag (which is the -k flag), it a change can be
predicted to lead to better coverage."

	button .road2.b -text "Ok" -command {destroy .road2}
	pack append .road2 .road2.z {top expand fill}
	pack append .road2 .road2.b {top}
	.road2.z.t configure -state disabled 
	.road2.z.t yview -pickplace 1.0      
	focus .road2.z.t
}

proc roadmap2d {} {
	global FG BG

	catch {destroy .road2}
	toplevel .road2

	wm title .road2 "Help with Verification"
	wm iconname .road2 "ValHelp"
	frame .road2.z
	scrollbar .road2.z.s -command ".road2.z.t yview"
	text .road2.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road2.z.s set" \
		-setgrid 1 -width 60 -height 26 -wrap word
	pack append .road2.z \
		.road2.z.s {left filly} \
		.road2.z.t {left expand fill}
	.road2.z.t insert end "GENERAL GUIDELINES:
=> When just starting out, it is safe to leave all parameters in the\
Verification Options Panel set at their initial value and to simply\
push the Run button in the Basic Options Panel for a default\
exhaustive verification.\
If you run out of memory, adjust the parameter settings as \
indicated under the 'explain' options.

=> If an error is found, first try to reduce the search depth to \
find a shorter equivalent.  Once you're content with the length,\
move on to a guided simulation to inspect the error-trail in detail.\
Optionally, use the Find Shortest Trail option, but note that this\
can increase runtime and memory use. So: do not use this option until\
you are certain that an error exists -- leave it turned off by default).\
If you are verifying a Safety Property, try the Breadth-First Search\
mode to find the shortest counter-example. It may run out of memory\
sooner than the default depth-first search mode, but it often works.

=> It is always safe to leave the Partial Order Reduction option enabled.\
Turn it off only for debugging purposes, or when warned to do so by the \
verifier itself.  The Profiling option gathers statistics about the \
verification hot-spots in the model.

=> If you save all error-trails, you have to copy the one you are\
interested in inspecting with a guided simulation onto the file\
pan_in.trail manually (outside xspin) after the run completes.\
The trails are numbered in order of discovery."

	button .road2.b -text "Ok" -command {destroy .road2}
	pack append .road2 .road2.z {top expand fill}
	pack append .road2 .road2.b {top}
	.road2.z.t configure -state disabled 
	.road2.z.t yview -pickplace 1.0      
	focus .road2.z.t
}

proc roadmap2e {} {
	global FG BG

	catch {destroy .road2}
	toplevel .road2

	wm title .road2 "Help with Verification"
	wm iconname .road2 "ValHelp"
	frame .road2.z
	scrollbar .road2.z.s -command ".road2.z.t yview"
	text .road2.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road2.z.s set" \
		-setgrid 1 -width 60 -height 25 -wrap word
	pack append .road2.z \
		.road2.z.s {left filly} \
		.road2.z.t {left expand fill}
	.road2.z.t insert end "BASIC GUIDELINES:
When just starting out, it is safe to leave all parameters\
at their initial value and to push the Run button for a\
default exhaustive verification.\
If you run out of memory, adjust the parameter settings\
under Advanced Options.

=> Safety properties are properties of single states (like\
deadlocks = invalid endstates, or assertion violations).

=> Liveness properties are properties of sequences of\
states (typically: infinite sequences, i.e., cycles).\
There are two types of cycles: non-progress (not passing\
through any state marked with a 'progress' label) and\
acceptance (passing infinitely often through a state\
marked with an 'accept' label).

=> Use the Weak Fairness option only when really necessary,\
to avoid unwated error reports.  It can increase the CPU-time\
requirements by a factor roughly equal to twice the number of\
active processes.

=> It is safe to leave the Reduced Search option enabled.\
Turn it off only for debugging purposes, or when warned to do so by the \
verifier itself.   The Profiling option gathers statistics about the \
verification hot-spots in the model.

=> You can apply a Never Claim even when checking Safety Properties\
when you want to restrict the search to the sequences that are\
matched by the claim (a user-guided search pruning technique)."

	button .road2.b -text "Ok" -command {destroy .road2}
	pack append .road2 .road2.z {top expand fill}
	pack append .road2 .road2.b {top}
	.road2.z.t configure -state disabled 
	.road2.z.t yview -pickplace 1.0      
	focus .road2.z.t
}

proc roadmap2f {} {
	global FG BG

	catch {destroy .road2}
	toplevel .road2

	wm title .road2 "Help with Verification"
	wm iconname .road2 "ValHelp"
	frame .road2.z
	scrollbar .road2.z.s -command ".road2.z.t yview"
	text .road2.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road2.z.s set" \
		-setgrid 1 -width 60 -height 15 -wrap word
	pack append .road2.z \
		.road2.z.s {left filly} \
		.road2.z.t {left expand fill}
	.road2.z.t insert end "GUIDELINES:
This will run a verification for the specific LTL property\
that was defined in the LTL options panel that brought you\
here.  The claim was placed in a separate .ltl\
file, not included in the main specification.\
(It will be picked up in the verification automatically.)\
The separate .ltl file combines the notes, formula,\
macros, results, etc., for easier tracking.

On a first run, leave all choices at their initial\
value and push the Run button for a default verification.\
If you run out of memory, adjust the parameter settings\
under Advanced Options.

Use the Weak Fairness option only when really necessary,\
to avoid unwated error reports.  It can increase the CPU-time\
requirements by a factor roughly equal to twice the number of\
active processes."

	button .road2.b -text "Ok" -command {destroy .road2}
	pack append .road2 .road2.z {top expand fill}
	pack append .road2 .road2.b {top}
	.road2.z.t configure -state disabled 
	.road2.z.t yview -pickplace 1.0      
	focus .road2.z.t
}

proc roadmap2 {} {
	global FG BG

	catch {destroy .road2}
	toplevel .road2

	wm title .road2 "Help with Verification"
	wm iconname .road2 "ValHelp"
	frame .road2.z
	scrollbar .road2.z.s -command ".road2.z.t yview"
	text .road2.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road2.z.s set" \
		-setgrid 1 -width 60 -height 20 -wrap word
	pack append .road2.z \
		.road2.z.s {left filly} \
		.road2.z.t {left expand fill}
	.road2.z.t insert end "GUIDELINES:
When just starting out, it is safe to leave all
verification parameters set at their initial values
and to Run a default verification.
If you run out of memory, or encounter other problems,
look at the specific help options in the verification
settings panel.
One parameter is important to set correctly right from
the start: the physical memory size of your system.
It is by default set to 64 Mbytes.  Set it once to the
true amount of physical memory on your system, in Megabytes,
and never change it again (unless you buy more physical
memory for your machine of course).
You can find this parameter under advanced options in the
verification parameters panel.
Bitstate/Supertrace verifications are approximate, and
only used for models too large to verify exhaustively.
This option allows you to get at least an approximate
answer to the correctness of models that could otherwise
not be verified by a finite state system."

	button .road2.b -text "Ok" -command {destroy .road2}
	pack append .road2 .road2.z {top expand fill}
	pack append .road2 .road2.b {top}
	.road2.z.t configure -state disabled 
	.road2.z.t yview -pickplace 1.0      
	focus .road2.z.t
}

proc roadmap3 {} {
	global FG BG

	catch {destroy .road3}
	toplevel .road3

	wm title .road3 "Reducing Complexity"
	wm iconname .road3 "CompHelp"
	frame .road3.z
	scrollbar .road3.z.s -command ".road3.z.t yview"
	text .road3.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road3.z.s set" \
		-setgrid 1 -width 60 -height 30 -wrap word
	pack append .road3.z \
		.road3.z.s {left filly} \
		.road3.z.t {left expand fill}
	.road3.z.t insert end "
When a verification cannot be completed because of\
computational complexity; here are some strategies that\
can be applied to combat this problem.

0. Run the Slicing Algorithm (in the Run Menu) to find\
potential redundancy in your model for the stated properties.

1. Try to make the model more general, more abstract.\
Remember that you are constructing a verification model and not\
an implementation.  SPIN's strength is in proving properties of\
*interactions* in a distributed system (the implicit assumptions\
that processes make about each other) -- it's strength is not in\
proving things about local *computations*, data dependencies etc.

2. Remove everything that is not directly related to the property\
you are trying to prove: redundant computations, redundant data. \
*Avoid counters*; avoid incrementing variables that are used for\
only book-keeping purposes.
The Syntax Check option will warn about the gravest offenses.

3. Asynchronous channels are a significant source of complexity in\
verification.  Use a synchronous channel where possible.  Reduce the\
number of slots in asynchronous channels to a minimum (use 2, or 3\
slots to get started).

4. Look for processes that merely transfer messages. Consider if\
you can remove processes that only copy incoming messages from\
one channel into another, by letting the sender generate the\
final message right away.  If the intermediate process makes\
choices (e.g., to delete or duplicate, etc.), let the sender\
make that choice, rather than the intermediate process.

5. Combine local computations into atomic, or d_step, sequences.

6. Avoid leaving scratch data around in variables.  You can reduce\
the number of states by, for instance, resetting local variables\
that are used inside atomic sequences to zero at the end of those\
sequences; so that the scratch values aren't visible outside the\
sequence.  Alternatively: introduce some extra global 'hidden'\
variables for these purposes (see the WhatsNew.html document).
Use the predefined variable \"_\" as a write-only scratch variable\
wherever possible.

7. If possible to do so: combine the behavior of two processes into\
a single one.  Generalize behavior;  focus on coordination aspects\
(i.e., the interfaces between processes, rather than the local\
computation inside processes).

8. Try to exploit the partial order reduction strategies.\
Use the xr and xs assertions (see WhatsNew.html); avoid sharing\
channels between multiple receivers or multiple senders.\
Avoid merging independent data-streams into a single shared channel."

	button .road3.b -text "Ok" -command {destroy .road3}
	pack append .road3 .road3.z {top expand fill}
	pack append .road3 .road3.b {top}
	.road3.z.t configure -state disabled 
	.road3.z.t yview -pickplace 1.0      
	focus .road3.z.t
}

proc roadmap5 {} {
	global FG BG

	catch {destroy .road5}
	toplevel .road5

	wm title .road5 "Spin Automata"
	wm iconname .road5 "FsmHelp"
	frame .road5.z
	scrollbar .road5.z.s -command ".road5.z.t yview"
	text .road5.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road5.z.s set" \
		-setgrid 1 -width 60 -height 30 -wrap word
	pack append .road5.z \
		.road5.z.s {left filly} \
		.road5.z.t {left expand fill}
	.road5.z.t insert end "
The Spin Automata view option give a view of the
structure of the automata models that Spin uses in
the verification process.
Each proctype is represented by a unique automaton.

Chosing this option (in the Run menu) will cause Spin to
first generate a verifier, compile it, and then run it
(as pan -d) to obtain a description of the proctype
names and the corresponding automata.

After selecting (double-clicking) the proctype name desired,
the graph will be produced.  The default graph layout
algorithm is small and a self-contained part of Xspin,
but also rather crude.  Be on guard, therefore, for edges
that overlap (a typical case, for instance, is a backedge
that hides behind a series of forward edges. Use DOT
(see the README.html file on Spin) when possible for much
better graph layout.

In the default layout, the following button actions are
defined (the first one is not needed when using DOT):

1. Moving Nodes: either Button-1 or Button-2.
2. Displaying Edge Labels: hold Button-1 down on the edge.
3. Cross-References: Move the cursor over a Node to see the
   corresponding line in the Promela source, in the main
   Xspin window.

If labels look bad -- try changing the font definitions at
the start of the xspin.tcl file (hints are given there).
"
	button .road5.b -text "Ok" -command {destroy .road5}
	pack append .road5 .road5.z {top expand fill}
	pack append .road5 .road5.b {top}
	.road5.z.t configure -state disabled 
	.road5.z.t yview -pickplace 1.0      
	focus .road5.z.t
}

proc roadmap6 {} {
	global FG BG

	catch {destroy .road6}
	toplevel .road6

	wm title .road6 "Optional Compiler Directives"
	wm iconname .road6 "Optional"
	frame .road6.z
	scrollbar .road6.z.s -command ".road6.z.t yview"
	text .road6.z.t -relief raised -bd 2 \
		-background $BG -foreground $FG \
		-yscrollcommand ".road6.z.s set" \
		-setgrid 1 -width 80 -height 30 -wrap word
	pack append .road6.z \
		.road6.z.s {left filly} \
		.road6.z.t {left expand fill}
	.road6.z.t insert end "
		Use only when prompted:

NFAIR=N		size memory used for enforcing weak fairness (default N=2)
VECTORSZ=N	allocates memory (in bytes) for state vector (default N=1024)

		Related to partial order reduction:

CTL		limit p.o.reduction to subset consistent with branching time logic
GLOB_ALPHA	consider process death a global action
XUSAFE		disable validity checks of xr/xs assertions

		Speedups:

NOBOUNDCHECK	don't check array bound violations
NOCOMP		don't compress states with fullstate storage (uses more memory)
NOSTUTTER	disable stuttering rules (warning: changes semantics)

		Memory saving (slower):

MA=N		use a minimized DFA encoding for state vectors up to N bytes

		Experimental:

BCOMP		when in BITSTATE mode, computes hash over compressed state-vector
NIBIS		apply a small optimization of partial order reduction
NOVSZ		risky - removes 4 bytes from state vector - its length field.
PRINTF		enables printfs during verification runs
RANDSTORE=N	in BITSTATE mode, -DRANDSTORE=33 lowers prob of storing a state to 33%
W_XPT=N		with MA, write checkpoint files every multiple of N states stored
R_XPT		with MA, restart run from last checkpoint file written

		Debugging:

SDUMP		with CHECK: adds ascii dumps of state vectors
SVDUMP		add run option -pN to write N-byte state vectors into file sv_dump

		Already set by the other xspin options:

BITSTATE	use supertrace/bitstate instead of exhaustive exploration
HC		use hash-compact instead of exhaustive exploration
COLLAPSE	collapses state vector size by up to 80% to 90%
MEMCNT=N	set upperbound of 2^N bytes to memory that can be allocated
MEMLIM=N	set upperbound of N Mbytes to memory that can be allocated
NOCLAIM		exclude never claim from the verification, if present
NOFAIR		disable the code for weak-fairness (is faster)
NOREDUCE	disables the partial order reduction algorithm
NP		enable non-progress cycle detection (option -l, replacing -a),
PEG		add complexity profiling (transition counts)
REACH		guarantee absence of errors within the -m depth-limit
SAFETY		optimize for the case where no cycle detection is needed
VAR_RANGES	compute the effective value range of byte variables
CHECK		generate debugging information (see also below)
VERBOSE		elaborate debugging printouts
"
	button .road6.b -text "Ok" -command {destroy .road6}
	pack append .road6 .road6.z {top expand fill}
	pack append .road6 .road6.b {top}
	.road6.z.t configure -state disabled 
	.road6.z.t yview -pickplace 1.0      
	focus .road6.z.t
}


# simulation options panel

set s_options	""
set v_options	""
set a_options	""
set c_options	""

set Blue	"blue";		#"yellow"
set Yellow	"yellow";	#"red"
set White	"white";	#"yellow"
set Red		"red";		#"yellow"
set Green	"green";	#"green"

set fd		0
set Depth	0
set Seq(0)	0
set Sdbox	0
set Spbox(0)	0
set sbox	0

set simruns	0
set stepper	0
set stepped	0
set VERBOSE	0
set SYMBOLIC	0
set howmany	0
set Choice(1)	0
set Sticky(0)	0
set SparseMsc	1
set showvars	1
set vv		1
set qv		1
set gvars	1
set lvars	0
set hide_q1	""
set hide_q2	""
set hide_q3	""
set PlaceSim	"+200+100"

proc simulation_old {} {
	global s_typ l_typ showvars qv PlaceSim
	global fvars gvars lvars SparseMsc HelvBig
	global msc ebc tsc vv svars rvars seed jumpsteps
	global hide_q1 hide_q2 hide_q3

	catch { .menu.run.m entryconfigure 5 -state normal }

	catch {destroy .s}
	toplevel .s
	set k [string first "\+" $PlaceSim]
	if {$k > 0} {
		set PlaceSim [string range $PlaceSim $k end]
	}

	wm title .s "Simulation Options"
	wm iconname .s "SIM"
	wm geometry .s $PlaceSim

	frame .s.opt -relief flat

	mkpan_in

	frame .s.opt.mode -relief raised -borderwidth 1m
	label .s.opt.mode.fld0 \
		-font $HelvBig \
		-text "Display Mode" \
		-relief sunken -borderwidth 1m

	checkbutton .s.opt.mode.fld4b -text "Time Sequence Panel - with:" \
		-variable tsc \
		-relief flat
	frame .s.opt.mode.flds
	radiobutton .s.opt.mode.flds.fld3 \
		-text "    Interleaved Steps" \
		-variable m_typ -value 2 \
		-relief flat
	radiobutton .s.opt.mode.flds.fld1 \
		-text "    One Window per Process" \
		-variable m_typ -value 0 \
		-relief flat
	radiobutton .s.opt.mode.flds.fld2 \
		-text "    One Trace per Process" \
		-variable m_typ -value 1 \
		-relief flat
	frame .s.opt.mode.flds.fld0 -width 15
	pack append .s.opt.mode.flds \
		.s.opt.mode.flds.fld0 {left frame w}\
		.s.opt.mode.flds.fld3 {top frame w}\
		.s.opt.mode.flds.fld1 {top frame w}\
		.s.opt.mode.flds.fld2 {top frame w}

	checkbutton .s.opt.mode.fld4a -text "MSC Panel - with:" \
		-variable msc \
		-relief flat
	frame .s.opt.mode.steps
	radiobutton .s.opt.mode.steps.fld5 -text "    Step Number Labels" \
		-variable SYMBOLIC -value 0 \
		-relief flat
	radiobutton .s.opt.mode.steps.fld6 -text "    Source Text Labels" \
		-variable SYMBOLIC -value 1 \
		-relief flat
	radiobutton .s.opt.mode.steps.fld7 -text "    Normal Spacing" \
		-variable SparseMsc -value 1 \
		-relief flat
	radiobutton .s.opt.mode.steps.fld8 -text "    Condensed Spacing" \
		-variable SparseMsc -value 0 \
		-relief flat
	frame .s.opt.mode.steps.fld0 -width 15
	pack append .s.opt.mode.steps \
		.s.opt.mode.steps.fld0 {left frame w}\
		.s.opt.mode.steps.fld5 {top frame w}\
		.s.opt.mode.steps.fld6 {top frame w}\
		.s.opt.mode.steps.fld7 {top frame w}\
		.s.opt.mode.steps.fld8 {top frame w}

	checkbutton .s.opt.mode.fld4c -text "Execution Bar Panel" \
		-variable ebc \
		-relief flat
	checkbutton .s.opt.mode.fld4d -text "Data Values Panel" \
		-variable vv \
		-relief flat

	frame .s.opt.mode.vars

	checkbutton .s.opt.mode.vars.fld4c -text "    Track Buffered Channels" \
		-variable qv \
		-relief flat
	checkbutton .s.opt.mode.vars.fld4d -text "    Track Global Variables" \
		-variable gvars \
		-relief flat
	checkbutton .s.opt.mode.vars.fld4e -text "    Track Local Variables" \
		-variable lvars \
		-relief flat

	checkbutton .s.opt.mode.vars.fld4f \
		-text "    Display vars marked 'show' in MSC" \
		-variable showvars \
		-relief flat
	frame .s.opt.mode.vars.fld0 -width 15
	pack append .s.opt.mode.vars \
		.s.opt.mode.vars.fld0 {left frame w}\
		.s.opt.mode.vars.fld4c {top frame w}\
		.s.opt.mode.vars.fld4d {top frame w}\
		.s.opt.mode.vars.fld4e {top frame w}\
		.s.opt.mode.vars.fld4f {top frame w}

	pack append .s.opt.mode .s.opt.mode.fld0 {top pady 4 frame w fillx}

	pack append .s.opt.mode .s.opt.mode.fld4a {top pady 4 frame w}
	pack append .s.opt.mode .s.opt.mode.steps {top frame w}

	pack append .s.opt.mode .s.opt.mode.fld4b {top pady 4 frame w}
	pack append .s.opt.mode .s.opt.mode.flds  {top frame w}

	pack append .s.opt.mode .s.opt.mode.fld4d {top pady 4 frame w}
	pack append .s.opt.mode .s.opt.mode.vars {top frame w}

	pack append .s.opt.mode .s.opt.mode.fld4c {top pady 4 frame w}

	pack append .s.opt .s.opt.mode {left frame n}

	frame .s.opt.mesg -relief raised -borderwidth 1m
	label .s.opt.mesg.loss0 \
		-font $HelvBig \
		-text "A Full Queue" \
		-relief sunken -borderwidth 1m
	radiobutton .s.opt.mesg.loss1 -text "Blocks New Msgs" \
		-variable l_typ -value 0 \
		-relief flat
	radiobutton .s.opt.mesg.loss2 -text "Loses New Msgs" \
		-variable l_typ -value 1 \
		-relief flat
	pack append .s.opt.mesg .s.opt.mesg.loss0 {top pady 4 frame w fillx}
	pack append .s.opt.mesg .s.opt.mesg.loss1 {top pady 4 frame w}
	pack append .s.opt.mesg .s.opt.mesg.loss2 {top pady 4 frame w}

	frame .s.opt.hide -relief raised -borderwidth 1m
	label .s.opt.hide.txt  \
		-font $HelvBig \
		-text "Hide Queues in MSC" \
		-relief sunken -borderwidth 1m
	pack append .s.opt.hide .s.opt.hide.txt {top pady 4 frame w fillx }

	for {set i 1} {$i < 4} {incr i} {
		frame .s.opt.hide.q$i
		label .s.opt.hide.q$i.qno \
			-font $HelvBig \
			-text "Queue nr:"
		entry .s.opt.hide.q$i.entry \
			-relief sunken -width 8
		pack append .s.opt.hide.q$i .s.opt.hide.q$i.qno {left pady 4 frame n }
		pack append .s.opt.hide.q$i .s.opt.hide.q$i.entry {left pady 4 frame n}
		pack append .s.opt.hide .s.opt.hide.q$i {top pady 4 frame w fillx}
	}
	frame .s.opt.lframe -relief raised -borderwidth 1m
	label .s.opt.lframe.tl \
		-font $HelvBig \
		-text "Simulation Style" \
		-relief sunken -borderwidth 1m
	radiobutton .s.opt.lframe.is -text "Interactive" \
		-variable s_typ -value 2 \
		-relief flat
	radiobutton .s.opt.lframe.gs -text "Guided (using pan.trail)" \
		-variable s_typ -value 1 \
		-relief flat
	frame .s.opt.lframe.b
	entry .s.opt.lframe.b.entry -relief sunken -width 8 
	label .s.opt.lframe.b.label \
		-font $HelvBig \
		-text "Steps Skipped"
	pack append .s.opt.lframe.b \
		.s.opt.lframe.b.label {left} \
		.s.opt.lframe.b.entry {left}

	radiobutton .s.opt.lframe.rs -text "Random (using seed)" \
		-variable s_typ -value 0 \
		-relief flat
	frame .s.opt.lframe.s
	entry .s.opt.lframe.s.entry -relief sunken -width 8 
	label .s.opt.lframe.s.label \
		-font $HelvBig \
		-text "Seed Value"
	pack append .s.opt.lframe.s \
		.s.opt.lframe.s.label {left} \
		.s.opt.lframe.s.entry {left}

	pack append .s.opt.lframe .s.opt.lframe.tl {top pady 4 frame w fillx} \
		.s.opt.lframe.rs {top pady 4 frame w} \
		.s.opt.lframe.s  {top pady 4 frame e} \
		.s.opt.lframe.gs {top pady 4 frame w} \
		.s.opt.lframe.b  {top pady 4 frame e} \
		.s.opt.lframe.is {top pady 4 frame w}

	pack append .s.opt .s.opt.lframe {top frame n}
	pack append .s.opt .s.opt.mesg {top frame n fillx}
	pack append .s.opt .s.opt.hide {top frame n expand fillx filly}

	pack append .s .s.opt { top frame n }

	pack append .s [button .s.rewind -text "Start" \
		-command "Rewind"  ] {right frame e}
	pack append .s [button .s.exit -text "Cancel" \
		-command "Stopsim" ] {right frame e}
	pack append .s [button .s.help -text "Help" -fg red \
		-command "roadmap1" ] {right frame e}

	.s.opt.lframe.s.entry  insert end $seed
	.s.opt.lframe.b.entry insert end $jumpsteps

	.s.opt.hide.q1.entry insert end $hide_q1
	.s.opt.hide.q2.entry insert end $hide_q2
	.s.opt.hide.q3.entry insert end $hide_q3

	raise .s
}


proc simulation {} {
	global s_typ l_typ showvars qv PlaceSim
	global fvars gvars lvars SparseMsc HelvBig
	global msc ebc tsc vv svars rvars seed jumpsteps
	global hide_q1 hide_q2 hide_q3
	global whichsim

	catch { .menu.run.m entryconfigure 5 -state normal }

	catch {destroy .s}
	toplevel .s
		catch {rmfile pan_in9999999.trail}
		debug {about to remove pan_in9999999.trail}
		rmfile pan_in9999999.trail
	set k [string first "\+" $PlaceSim]
	if {$k > 0} {
		set PlaceSim [string range $PlaceSim $k end]
	}

	wm title .s "Simulation Options"
	wm iconname .s "SIM"
	wm geometry .s $PlaceSim

	mkpan_in

	# lower frame with 'start', 'cancel' and 'help' buttons
	frame .s.l -relief flat
	pack .s.l -side bottom -fill both

	  pack [button .s.l.rewind -text " Start " \
		-command "Rewind"  ] -side right -fill y -padx 4
	  pack [button .s.l.exit -text "Cancel" \
		-command "
			Stopsim;
			catch {rmfile pan_in9999999.trail}
			"
		] -side right -fill y -padx 4
	  pack [button .s.l.help -text " Help " -fg red \
		-command "roadmap1" ]  -side right -fill y -padx 4

	# upper frame with modes and options
	frame .s.u -relief flat
	pack .s.u -side top -fill both

	  frame .s.u.mode -relief raised -borderwidth 1m
	  pack .s.u.mode -side left -fill both -expand 1

	    frame .s.u.mode.fdis -relief flat
	    pack .s.u.mode.fdis -side top -fill x -expand 1

	      label .s.u.mode.fdis.fld0 \
		-font $HelvBig \
		-text "Display Mode" \
		-relief sunken -borderwidth 1m
	      pack .s.u.mode.fdis.fld0 -side top -fill x

#MSC Panel

	    frame .s.u.mode.fmsc -relief flat
	    pack  .s.u.mode.fmsc -side top -fill x

	      checkbutton .s.u.mode.fmsc.fld4a -text "MSC Panel - with:" \
		  -variable msc \
		  -relief flat
	      pack .s.u.mode.fmsc.fld4a -side left

	    frame .s.u.mode.msc -relief flat
	    pack  .s.u.mode.msc -side top -fill x

	      frame .s.u.mode.msc.lab -relief flat
	      pack  .s.u.mode.msc.lab -side top -fill x

	        frame .s.u.mode.msc.lab.dummy -width 18 -relief flat
	        pack .s.u.mode.msc.lab.dummy -side left -fill y

		frame .s.u.mode.msc.lab.radios -relief flat
		pack .s.u.mode.msc.lab.radios -side left -fill x
	
		  frame .s.u.mode.msc.lab.radios.fnum -relief flat
		  pack .s.u.mode.msc.lab.radios.fnum -side top -fill x

		    radiobutton .s.u.mode.msc.lab.radios.fnum.fld5 \
			  -text "    Step Number Labels" \
		  	  -variable SYMBOLIC -value 0 \
		  	  -relief flat
		    pack .s.u.mode.msc.lab.radios.fnum.fld5 -side left

		  frame .s.u.mode.msc.lab.radios.ftext -relief flat
		  pack .s.u.mode.msc.lab.radios.ftext -side top -fill x

		    radiobutton .s.u.mode.msc.lab.radios.ftext.fld6 \
			-text "    Source Text Labels" \
			-variable SYMBOLIC -value 1 \
		  	-relief flat
		    pack .s.u.mode.msc.lab.radios.ftext.fld6 -side left

		frame .s.u.mode.msc.lab.bracket
		pack .s.u.mode.msc.lab.bracket -side left -fill y
		
		  canvas .s.u.mode.msc.lab.bracket.c -width 10 -height 40
		  pack .s.u.mode.msc.lab.bracket.c -side top
		    .s.u.mode.msc.lab.bracket.c create line 5 15 10 15 10 38 5 38

	      frame .s.u.mode.msc.space -relief flat
	      pack  .s.u.mode.msc.space -side top -fill x

	        frame .s.u.mode.msc.space.dummy -width 18 -relief flat
	        pack .s.u.mode.msc.space.dummy -side left -fill y

		frame .s.u.mode.msc.space.radios -relief flat
		pack  .s.u.mode.msc.space.radios -side left -fill x
	
		  frame .s.u.mode.msc.space.radios.fnorm -relief flat
		  pack  .s.u.mode.msc.space.radios.fnorm -side top -fill x

		    radiobutton .s.u.mode.msc.space.radios.fnorm.fld7 \
			-text "    Normal Spacing" \
			-variable SparseMsc -value 1 \
			-relief flat
		    pack .s.u.mode.msc.space.radios.fnorm.fld7 -side left

	  	  frame .s.u.mode.msc.space.radios.fcond -relief flat
		  pack  .s.u.mode.msc.space.radios.fcond -side top -fill x

		    radiobutton .s.u.mode.msc.space.radios.fcond.fld8 \
			-text "    Condensed Spacing" \
			-variable SparseMsc -value 0 \
			-relief flat
		    pack .s.u.mode.msc.space.radios.fcond.fld8 -side left

		frame .s.u.mode.msc.space.bracket
		pack .s.u.mode.msc.space.bracket -side left -fill y
		
		  canvas .s.u.mode.msc.space.bracket.c -width 10 -height 40
		  pack .s.u.mode.msc.space.bracket.c -side top
		    .s.u.mode.msc.space.bracket.c create line 5 15 10 15 10 38 5 38

# Time Sequence Panel

	    frame .s.u.mode.ftsp -relief flat 
	    pack  .s.u.mode.ftsp -side top -fill x

	      checkbutton .s.u.mode.ftsp.fld4b \
		-text "Time Sequence Panel - with:" \
		-variable tsc \
		-relief flat
	      pack .s.u.mode.ftsp.fld4b -side left

	    frame .s.u.mode.tsp
	    pack  .s.u.mode.tsp -side top -fill x

	      frame .s.u.mode.tsp.proc
	      pack  .s.u.mode.tsp.proc -side top -fill x

		frame .s.u.mode.tsp.proc.dummy -width 18
		pack  .s.u.mode.tsp.proc.dummy -side left -fill y

		frame .s.u.mode.tsp.proc.radios -relief flat
	        pack  .s.u.mode.tsp.proc.radios -side left -fill y

	          frame .s.u.mode.tsp.proc.radios.is
	          pack  .s.u.mode.tsp.proc.radios.is -side top -fill x

		    radiobutton .s.u.mode.tsp.proc.radios.is.fld3 \
			    -text "    Interleaved Steps" \
			    -variable m_typ -value 2 \
		  	    -relief flat
		    pack .s.u.mode.tsp.proc.radios.is.fld3 -side left

		  frame .s.u.mode.tsp.proc.radios.1win -relief flat
		  pack .s.u.mode.tsp.proc.radios.1win -side top -fill x

		    radiobutton .s.u.mode.tsp.proc.radios.1win.fld1 \
			-text "    One Window per Process" \
			-variable m_typ -value 0 \
			-relief flat
		    pack .s.u.mode.tsp.proc.radios.1win.fld1 -side left

		  frame .s.u.mode.tsp.proc.radios.1trace -relief flat
		  pack  .s.u.mode.tsp.proc.radios.1trace -side top -fill x

		    radiobutton .s.u.mode.tsp.proc.radios.1trace.fld2 \
			-text "    One Trace per Process" \
			-variable m_typ -value 1 \
			-relief flat
		    pack .s.u.mode.tsp.proc.radios.1trace.fld2 -side left

		frame .s.u.mode.tsp.proc.bracket
		pack .s.u.mode.tsp.proc.bracket -side left -fill y

		set y 13
		  canvas .s.u.mode.tsp.proc.bracket.c -width 10 -height 66
		  pack .s.u.mode.tsp.proc.bracket.c -side top
		    .s.u.mode.tsp.proc.bracket.c create line    5  [expr 0 + $y] \
								10 [expr 0 + $y] \
								10 [expr 25 + $y] \
								5  [expr 25 + $y] \
								10 [expr 25 + $y] \
								10 [expr 50 + $y] \
								5  [expr 50 + $y]

	frame .s.u.mode.fdvp -relief flat
	pack .s.u.mode.fdvp -side top -fill x

	  checkbutton .s.u.mode.fdvp.fld4d -text "Data Values Panel" \
		-variable vv \
		-relief flat
	  pack .s.u.mode.fdvp.fld4d -side left

	frame .s.u.mode.vars
	pack .s.u.mode.vars -side top -fill x

	  frame .s.u.mode.vars.dummy -width 18
	  pack .s.u.mode.vars.dummy -side left -fill y

	  frame .s.u.mode.vars.chks -relief flat
	  pack  .s.u.mode.vars.chks -side left -fill y
	    
	    frame .s.u.mode.vars.chks.ftbc
	    pack  .s.u.mode.vars.chks.ftbc -side top -fill x

	      checkbutton .s.u.mode.vars.chks.ftbc.fld4c -text "    Track Buffered Channels" \
		-variable qv \
		-relief flat
	      pack .s.u.mode.vars.chks.ftbc.fld4c -side left

	    frame .s.u.mode.vars.chks.ftgv
	    pack  .s.u.mode.vars.chks.ftgv -side top -fill x

	      checkbutton .s.u.mode.vars.chks.ftgv.fld4d -text "    Track Global Variables" \
		-variable gvars \
		-relief flat
	      pack .s.u.mode.vars.chks.ftgv.fld4d -side left

	    frame .s.u.mode.vars.chks.ftlv
	    pack  .s.u.mode.vars.chks.ftlv -side top -fill x

	      checkbutton .s.u.mode.vars.chks.ftlv.fld4e -text "    Track Local Variables" \
		-variable lvars \
		-relief flat
	      pack .s.u.mode.vars.chks.ftlv.fld4e -side left

	    frame .s.u.mode.vars.chks.fshow
	    pack  .s.u.mode.vars.chks.fshow -side top -fill x

	      checkbutton .s.u.mode.vars.chks.fshow.fld4f \
		-text "    Display vars marked 'show' in MSC" \
		-variable showvars \
		-relief flat

	      pack .s.u.mode.vars.chks.fshow.fld4f -side left

	frame .s.u.mode.fexecbar -relief flat
	pack .s.u.mode.fexecbar -side top -fill x

	  checkbutton .s.u.mode.fexecbar.fld4c -text "Execution Bar Panel" \
		-variable ebc \
		-relief flat
          pack .s.u.mode.fexecbar.fld4c -side left

#Right upper frame
	frame .s.u.r -relief flat
	pack .s.u.r -side right -fill y -expand 1

#Simulation Style
	  frame .s.u.r.sim -relief raised -borderwidth 1m
	  pack .s.u.r.sim -side top -fill both -expand 1

	    frame .s.u.r.sim.flab -relief sunken
	    pack .s.u.r.sim.flab -side top -fill x

	      label .s.u.r.sim.flab.tl \
		-font $HelvBig \
		-text "Simulation Style" \
		-relief sunken -borderwidth 1m
	      pack .s.u.r.sim.flab.tl -side top -fill x

	    frame .s.u.r.sim.random
	    pack .s.u.r.sim.random -side top -fill x

	   	radiobutton .s.u.r.sim.random.rs -text "Random (using seed)" \
		-variable s_typ -value 0 \
		-relief flat \
		-command "enable_disable_sub_buttons"
		pack .s.u.r.sim.random.rs -side left

	    frame .s.u.r.sim.seedvalue
	    pack .s.u.r.sim.seedvalue -side top -fill x

	      frame .s.u.r.sim.seedvalue.dummy -width 18
	      pack .s.u.r.sim.seedvalue.dummy -side left -fill y

	      label .s.u.r.sim.seedvalue.label \
		-font $HelvBig \
		-text "Seed Value"
	      pack .s.u.r.sim.seedvalue.label -side left

	      entry .s.u.r.sim.seedvalue.entry -relief sunken -width 8
	      pack .s.u.r.sim.seedvalue.entry -side left

	    frame .s.u.r.sim.guided
	    pack .s.u.r.sim.guided -side top -fill x

		radiobutton .s.u.r.sim.guided.gs -text "Guided" \
		   -variable s_typ -value 1 \
		   -relief flat \
		   -command "enable_disable_sub_buttons"
		pack .s.u.r.sim.guided.gs -side left
		
		frame .s.u.r.sim.guided_type
		pack .s.u.r.sim.guided_type -side top -fill x

		  frame .s.u.r.sim.guided_type.dummy -width 18
		  pack .s.u.r.sim.guided_type.dummy -side left -fill y

		    frame .s.u.r.sim.guided_type.radios
		    pack .s.u.r.sim.guided_type.radios -side left

		      frame .s.u.r.sim.guided_type.radios.pan_trail
		      pack  .s.u.r.sim.guided_type.radios.pan_trail -side top -fill x

			radiobutton .s.u.r.sim.guided_type.radios.pan_trail.rb \
				-text "Using pan_in.trail" \
				-variable whichsim -value 0 \
				-relief flat
			pack .s.u.r.sim.guided_type.radios.pan_trail.rb -side left

		      frame .s.u.r.sim.guided_type.radios.trail_other
		      pack  .s.u.r.sim.guided_type.radios.trail_other -side top -fill x

			radiobutton .s.u.r.sim.guided_type.radios.trail_other.rb \
				-text "Use" \
				-variable whichsim -value 1 \
				-relief flat
		        pack .s.u.r.sim.guided_type.radios.trail_other.rb -side left

			entry .s.u.r.sim.guided_type.radios.trail_other.entry \
				-width 20
		        pack .s.u.r.sim.guided_type.radios.trail_other.entry -side left

			button .s.u.r.sim.guided_type.radios.trail_other.button -text "Browse" \
				-command select_trail_file
		        pack .s.u.r.sim.guided_type.radios.trail_other.button -side left	  

 	    frame .s.u.r.sim.skipstep
	    pack  .s.u.r.sim.skipstep -side top -fill x

	      label .s.u.r.sim.skipstep.label \
		-font $HelvBig \
		-text "Steps Skipped"
	      pack .s.u.r.sim.skipstep.label -side left

	      entry .s.u.r.sim.skipstep.entry -relief sunken -width 8
	      pack .s.u.r.sim.skipstep.entry -side left

	    frame .s.u.r.sim.interactive
	    pack .s.u.r.sim.interactive -side top -fill x

 		radiobutton .s.u.r.sim.interactive.is -text "Interactive" \
			-variable s_typ -value 2 \
			-relief flat \
		-command "enable_disable_sub_buttons"
	        pack .s.u.r.sim.interactive.is -side left

#A Full Queue
	  frame .s.u.r.fq -relief raised -borderwidth 1m
	  pack .s.u.r.fq -side top -fill both -expand 1

	    frame .s.u.r.fq.label -relief sunken
	    pack .s.u.r.fq.label -side top -fill x

	      label .s.u.r.fq.label.loss0 \
		-font $HelvBig \
		-text "A Full Queue" \
		-relief sunken -borderwidth 1m
	      pack .s.u.r.fq.label.loss0 -side top -fill x

	    frame .s.u.r.fq.block
	    pack .s.u.r.fq.block -side top -fill x

	      radiobutton .s.u.r.fq.block.loss1 -text "Blocks New Msgs" \
		-variable l_typ -value 0 \
		-relief flat
	      pack .s.u.r.fq.block.loss1 -side left

	    frame .s.u.r.fq.lose
	    pack .s.u.r.fq.lose -side top -fill x

	      radiobutton .s.u.r.fq.lose.loss2 -text "Loses New Msgs" \
		-variable l_typ -value 1 \
		-relief flat
	      pack .s.u.r.fq.lose.loss2 -side left

#Hide Queues in MSC
	  frame .s.u.r.hq -relief raised -borderwidth 1m
	  pack .s.u.r.hq -side top -fill both -expand 1

	    frame .s.u.r.hq.flabel -relief sunken
	    pack .s.u.r.hq.flabel -side top -fill x

		label .s.u.r.hq.flabel.txt  \
		  -font $HelvBig \
		  -text "Hide Queues in MSC" \
		  -relief sunken -borderwidth 1m
	    pack .s.u.r.hq.flabel.txt -side top -fill x

	for {set i 1} {$i < 4} {incr i} {
		frame .s.u.r.hq.q$i
		pack .s.u.r.hq.q$i -side top -fill x
		label .s.u.r.hq.q$i.qno \
			-font $HelvBig \
			-text "Queue nr:"
		pack .s.u.r.hq.q$i.qno -side left
		entry .s.u.r.hq.q$i.entry \
			-relief sunken -width 8
		pack .s.u.r.hq.q$i.entry -side left
	}

	.s.u.r.sim.seedvalue.entry insert end $seed
	.s.u.r.sim.skipstep.entry insert end $jumpsteps

	.s.u.r.hq.q1.entry insert end $hide_q1
	.s.u.r.hq.q2.entry insert end $hide_q2
	.s.u.r.hq.q3.entry insert end $hide_q3
	enable_disable_sub_buttons

	tkwait visibility .s
	raise .s
}

proc enable_disable_sub_buttons {} {
	global s_typ
	switch -regexp $s_typ {
		0|2 { .s.u.r.sim.guided_type.radios.pan_trail.rb configure -state disabled
		      .s.u.r.sim.guided_type.radios.trail_other.rb configure -state disabled
		      .s.u.r.sim.guided_type.radios.trail_other.button configure -state disabled
		    }
		1   { .s.u.r.sim.guided_type.radios.pan_trail.rb configure -state normal
		      .s.u.r.sim.guided_type.radios.trail_other.rb configure -state normal
		      .s.u.r.sim.guided_type.radios.trail_other.button configure -state normal
		      .s.u.r.sim.guided_type.radios.pan_trail.rb select
		    }

	}
}

proc select_trail_file {} {
	global Trail_filename
	.s.u.r.sim.guided_type.radios.trail_other.rb select
	# try to use the predefined file selection dialog
	switch [info commands tk_getOpenFile] "" {
		# some old version of Tk so use our own file selection dialog
		set fileselect "FileSelect open"
	} default {
		set fileselect "tk_getOpenFile"
	}
	set init_dir [pwd]
	# get the file (return if the file selection dialog canceled)
	switch -- [set file [eval $fileselect -initialdir { { $init_dir } } ]] "" return
	.s.u.r.sim.guided_type.radios.trail_other.entry insert end $file
	raise .s
	
}

proc bld_s_options {} {
	global fvars gvars lvars svars qv
	global rvars l_typ showvars vv
	global s_typ seed jumpsteps s_options
	global hide_q1 hide_q2 hide_q3 ival whichsim trail_file trail_num

	set s_options "-X -p -v $ival(5)"

	if {$showvars && $gvars == 0 && $lvars == 0} {
		catch { tk_messageBox -icon info \
		-message "Display variables marked 'show' selected, \
		but no local or global vars are being tracked"
	}	}
	if {$showvars==1} { set s_options [format "%s -Y" $s_options] }
	if {$s_typ==2} { set s_options [format "%s -i" $s_options] }
	if {$vv && $gvars} { set s_options [format "%s -g" $s_options] }
	if {$vv && $lvars} { set s_options [format "%s -l" $s_options] }
	if {$svars} { set s_options [format "%s -s" $s_options] }
	if {$rvars} { set s_options [format "%s -r" $s_options] }
	if {$l_typ} { set s_options [format "%s -m" $s_options] }
	if {$hide_q1 != ""} { set s_options [format "%s -q%s" $s_options $hide_q1] }
	if {$hide_q2 != ""} { set s_options [format "%s -q%s" $s_options $hide_q2] }
	if {$hide_q3 != ""} { set s_options [format "%s -q%s" $s_options $hide_q3] }
	if {$s_typ==1} then {
		set trail_num ""
		#Guided
		if {$whichsim == 1} {
			#using user specified file
			if ![file exists $trail_file] {
				catch { tk_messageBox -icon info \
					-message "Trail file $trail_file does not exist."
				}
				return 0
			}
			# see if file is in current directory. if not, copy to 
			# pan_in9999999.trail in current directory
			set ind [string last "\/" $trail_file]
			if {$ind > -1} {
				if {[pwd] != [string range $trail_file 0 [expr $ind - 1]]} { 
					cpfile $trail_file pan_in9999999.trail
					set trail_file "pan_in9999999.trail"
				} else {
					#strip off path
					set trail_file [string range $trail_file \
						[expr $ind + 1] \
						[expr [string length $trail_file] - 1]]
				}
			}	
			#see if it's a 'pan_in<#>.trail' file
			set is_pan_in_trail_file 0
			if {[string range $trail_file 0 5] == "pan_in"} {
				set l [string length $trail_file]

				if {[string range $trail_file \
					[expr $l-6] [expr $l-1]] == ".trail"} {
					set num [string range $trail_file 6 [expr $l-7]]
					if [string is integer $num] {
						set trail_num $num
						set is_pan_in_trail_file 1
			}	}	}
			if !($is_pan_in_trail_file) {
				# not a 'pan_in<#>.trail' file - copy file to pan_in9999999.trail
				# in current directory
				cpfile $trail_file pan_in9999999.trail
				if [file exists pan_in9999999.trail] {
					set trail_num 9999999
				} else {
					catch {tk_messageBox -icon info \
						-message "Unable to create input file in $pwd \
							check write permissions."
					}
					return 0
				}
			}
		} else {
			if {![file exists pan_in.trail] && ![file exists pan_in.tra]} {
				catch { tk_messageBox -icon info \
					-message "Trail file \'pan_in.tra(il)\' does not exist."
				}
				return 0
			}
		}
			
		set s_options [format "%s -t%s" $s_options $trail_num]
	} else {
		if {[string length $seed] > 0} {
			set s_options [format "%s -n%s" $s_options $seed]
	}	}
	if {$s_typ!=2} then {
		if {[string length $jumpsteps] > 0} {
			set s_options [format "%s -j%s" $s_options $jumpsteps]
	}	}
	return 1
}

proc Stopsim {} {
	global stop dbox2 Sticky PlaceSim PlaceCanvas
	global stepper stepped howmany fd

	set stop 1
	set stepped 0
	set stepper 0
	add_log "<stop simulation>"
	if {[winfo exists .s]} {
		set PlaceSim [wm geometry .s]
		destroy .s
	}
	catch {set howmany 0}
	catch {stopbar}
	catch {	if {$Sticky($dbox2) == 0} {
			set PlaceCanvas(msc) [wm geometry .f$dbox2]
			destroy .f$dbox2
	}	}
	catch {
		puts $fd "q"
		flush $fd
	}
	update
}

proc Step_forw {} {
	global stepper stepped sbox simruns PlaceSim

	set stepped 1
	set stepper 1
	if {$simruns == 0} {
		if {[winfo exists .s]} {
			set PlaceSim [wm geometry .s]
			destroy .s
		}
		runsim
	} else {
		catch { .c$sbox.run configure \
		-text "Run" -command "Runsim" }
	}
}

proc Rewind {} {
	global Depth s_typ whichsim trail_file
	global Sdbox Spbox
	global seed jumpsteps simruns
	global hide_q1 hide_q2 hide_q3 trail_file

	catch { set jumpsteps [.s.u.r.sim.skipstep.entry get] }
	catch { set hide_q1 [.s.u.r.hq.q1.entry get] }
	catch { set hide_q2 [.s.u.r.hq.q2.entry get] }
	catch { set hide_q3 [.s.u.r.hq.q3.entry get] }

	if {$s_typ == 0} {
		catch { set seed [.s.u.r.sim.seedvalue.entry get] }
	}
	if {$s_typ == 1} {
		#Guided
		set Depth 0
		catch {
			foreach el [array names Spbox] {
				set Sdbox $Spbox($el)
				.c$Sdbox.z.t tag remove Rev 1.0 end
		}	}
		if {$whichsim == 1} {
			set trail_file ""
			catch {set trail_file [.s.u.r.sim.guided_type.radios.trail_other.entry get]}
		}
	}

	set simruns 0

	Step_forw
}

proc Runsim {} {
	global stepper stepped sbox

	catch { .c$sbox.run configure \
		-text "Suspend" -command "Step_forw" }
	set stepper 1
	set stepped 0
}

proc BreakPoint {} {
	global stepped sbox

	set stepped 1
	catch { .c$sbox.run configure \
		-text "BreakPoint" -command "Runsim" }
}

proc runsim {} {
	global Unix SPIN tk_major
	global s_options s_typ dbox2
	global stepper stepped
	global simruns m_typ
	global gvars lvars
	global fd stop Depth Seq
	global Sdbox Spbox pbox howmany Choice
	global sbox VERBOSE SYMBOLIC msc ebc vv tsc
	global Blue Yellow White Red Green
	global SmallFont BigFont Sticky SparseMsc
	global FG BG qv gvars lvars PlaceBox
	global dbox Vvbox 
	global whichsim trail_num

	set simruns 1
	set Vvbox 0
	set pno 0
	set Varnm("") ""
	set Queues("")	""
	set Depth 0
	set Seq(0) 0
	set Pstp 1
	set Seenpno 1
	set Banner "Select"

#	catch { unset Spbox(0) }
	catch {
		foreach el [array names pbox] {
			catch { destroy .c$pbox($el) }
			catch { unset pbox($el) }
		}
		foreach el [array names Spbox] {
			catch { destroy .c$Spbox($el) }
			catch { unset Spbox($el) }
		}
	}
	if ![bld_s_options] {
		return
	}

	add_log "<starting simulation>"
	add_log "$SPIN $s_options pan_in"
	update
	set s_options [format "%s pan_in" $s_options]

	mkpan_in

	set sbox [mkbox "Simulation Output" "SimOut" "sim.out" 71 11 100 100]

	pack append .c$sbox [button .c$sbox.stepf -text "Single Step" \
		-command "Step_forw" ] {left frame w}
	pack append .c$sbox [button .c$sbox.run -text "Run" \
		-command "Runsim" ] {left frame w}

	.c$sbox.b configure -text "Cancel" -command "Stopsim"

	raise .c$sbox

	set YSZ 12
	set XSZ 84
	set YNR 60
	set NPR 10
	set SMX 250
	set Easy 1
	set HAS 0
	set HAS_CYCLE 0
	set dontwait 0
	set notexecutable 0
	set lastexecutable 0

	if {$m_typ == 2} {
		if {$tsc} {
		set pbox(0) \
		[mkbox "Time Sequence" "Sequence" "seq.out" 80 10 100 325]
		set dbox $pbox(0)
	}	}
	if {$msc} {
		if {[hasWord "!!"] || [hasWord "\\?\\?"]} {
			set Easy 0
		}

		set maxx [expr [winfo screenwidth .]  - 400]	;# button widths
		set maxh [expr [winfo screenheight .] - (5+120)]	;# borders+buttons
		set dbox2 \
		[mkcanvas "Sequence Chart" "msc" $maxx 5 1]
		.f$dbox2.c configure -height $maxh \
			-scrollregion "[expr -$XSZ/2] 0 \
				[expr $NPR*$XSZ] [expr 100+$SMX*$YSZ]"

		raise .f$dbox2
	}

	raise .c$sbox

	set stop 0
	set good_trail 0
	if {$s_typ == 1} {
		if $whichsim {
			set filen "pan_in${trail_num}.trail"
			if [file exists $filen] {
				set good_trail 1
			}
		} else {
			if {[file exists pan_in.trail] || [file exists pan_in.tra]} {
				set good_trail 1
			}
		} 
		if $good_trail {
			catch { .c$sbox.z.t insert end "preparing trail, please wait..." }
			update
			rmfile trail.out
			catch {eval exec $SPIN $s_options >&trail.out} errmsg
		} else {
			set errmsg "error: no trail file for guided simulation"
			return
		}
		if {[string length $errmsg]>0} {
			add_log "$errmsg"
			catch {
			tk_messageBox -icon info -message $errmsg
			}
			catch {
				set fd [open trail.out r]
				while {[gets $fd line] > -1} {
					add_log "$line"
				}
				close $fd
			}
			Stopsim
			catch { destroy .c$sbox }
			catch { destroy .c$dbox }
			set simruns 0
			update
			return
		}
		set fd [open trail.out r]
		catch { .c$sbox.z.t insert end "done\n" }
	} else {
		update
		set fd [open "|$SPIN $s_options" r+]
		catch "flush $fd"
		update
	}

	if {$s_typ == 2} {
		Runsim
	}

	if {$ebc} { startbar "Xspin Bar Chart" }

	set pstp -1
	set bailout 0
	set realstring ""

	update
	raise .c$sbox
	lower .

	while {$stop == 0 && [eof $fd] == 0} {
	if {$bailout == 0 && [gets $fd line] > -1} {
		set pln 0
		set syntax 0
		set isvar 0
		set pname ""
		set i 0
		set VERBOSE 0
		set Fnm "pan_in"

		raise .c$sbox

		if {$Unix == 0} {
			if {[string first "processes created" $line] > 0} {
				set bailout 1
		}	}
		if {[string first "type return to proceed" $line] > 0} {
			puts $fd ""
			flush $fd
			update
			continue
		}

		set i [string first "<merge" $line]
		if {$i > 0} {
			set line [string range $line 0 $i]
		}

		set lastpstp $pstp
		set pmtch [scan $line \
			"%d: proc %d (%s line %d \"%s\" " \
			pstp pno pname pln Fnm]
		incr pmtch -1
		set i [string first "\[" $line]
		if {$i > 0} {
			set i [expr $i + 1]
			set j [string length $line]
			set j [expr $j - 2]
			set stmnt [string range $line $i $j]
		} else {
			set stmnt "-"
		}
		if {$pmtch != 4} {
			set pmtch [scan $line \
				"	proc %d (%s line %d \"%s\" " \
				pno pname pln Fnm]
		}
		if {$pmtch != 4} {
			if {[string first "spin: line" $line] == 0 } {
				scan $line "spin: line %d \"%s\" " pln Fnm
				if {[string first "pan_in" $Fnm] >= 0} {
				.inp.t tag add Rev $pln.0 $pln.end
				.inp.t tag configure Rev \
					-background $FG -foreground $BG
				.inp.t yview -pickplace $pln.0
				}
				if {[string first "assertion viol" $line] < 0} {
					set syntax 1
				}
			}
			if {[string first "Error: "   $line] >= 0 \
			||  [string first "warning: " $line] >= 0 } {
				set syntax 1
			}
		}
		if {$pmtch != 4 && $syntax == 0} {
			set pmtch [scan $line \
				"%d: proc - (%s line %d \"%s\" " \
				pstp pname pln Fnm]
			if { $pmtch == 4 } {
				set pno -1
			}
		}
		#	set Fnm [string trim $Fnm "\""]
		set pname [string trim $pname "()"]

		if {[string first "TRACK" $pname] >= 0} {
			set nwcol([expr $pno+1]) 1
		} elseif {[string length $pname] > 0} {
			if {[info exists nwcol([expr $pno+1])] \
			&&  $nwcol([expr $pno+1])} {
				unset Plabel($pno)
##
				set TMP1 [expr ($pno + 1)*$XSZ]
				set TMP2 [expr $Pstp*$YSZ]
				.f$dbox2.c create line \
					[expr $TMP1 - 20] $TMP2 \
					[expr $TMP1 + 20] $TMP2 \
					-width 2 \
					-fill $Red
				incr TMP2 4
				.f$dbox2.c create line \
					[expr $TMP1 - 20] $TMP2 \
					[expr $TMP1 + 20] $TMP2 \
					-width 2 \
					-fill $Red
##
			}
			set nwcol([expr $pno+1]) 0
		}
		if {$pmtch == 4 && $syntax == 0} {
			if {$ebc} {
				if {[string first "values:" $line] < 0} {
					stepbar $pno $pname
			}	}
			if {$m_typ == 1 && $tsc} {
				if { [info exists pbox($pno)] == 0 } {
					set pbox($pno) [mkbox \
						"Proc $pno ($pname)" \
						"Proc$pno" "proc.$pno.out" \
						60 10 \
						[expr 100+$pno*25] \
						[expr 325+$pno*35] ]
				}
				set dbox $pbox($pno)
			} elseif {$m_typ == 0 && $tsc} {
				if { [info exists Spbox($pno)] == 0 } {
					set Spbox($pno) \
						[mkbox "$pname (proc $pno)" \
						"$pname" "" \
						60 10 \
						[expr 100+$pno*25] \
						[expr 325+$pno*35] ]
					readinfile .c$Spbox($pno).z.t "pan_in"
				}
				set Sdbox $Spbox($pno)
			}
		} elseif { [string first "..." $line] > 0 && \
			   [regexp "^\\\t*MSC: (.*)" $line] == 0 } {
			set $line ""
			set syntax 1
			set pln 0
		} elseif {$s_typ == 2 \
		      && [string first "Select " $line] == 0 } {
			set Banner $line
			set pln 0
			set notexecutable 0
			set lastexecutable 0
			set has_timeout 0
		} elseif {$s_typ == 2 \
		      && [string first "choice" $line] >= 0 } {
			scan $line "	choice %d" howmany
			set NN [string first ":" $line]; incr NN 2
			set Choice($howmany) [string range $line $NN end]
			if {[string first "timeout" $Choice($howmany)] > 0} {
				set has_timeout 1
			}
			if {[string first "unexecutable," $line] >= 0} {
				incr notexecutable
			} else {
				set lastexecutable $howmany
			}
			set pln 0
		} elseif {$s_typ == 2 \
		      && [string first "Make Selection" $line] >= 0 } {
			scan $line "Make Selection %d" howmany
			if {$notexecutable == $howmany-1 && $has_timeout == 0} {
				set howmany $lastexecutable
				add_log "selected: $howmany (forced)"
				catch {
					foreach el [array names Choice] {
					unset Choice($el)
				} }
			} else {
				pickoption $Banner
				add_log "selected: $howmany"
			}
			puts $fd $howmany
			catch "flush $fd"
			set dontwait 1
			set pln 0
		} elseif { [regexp "^\\\t*MSC: (.*)" $line mch rstr] != 0 } {
			if {$realstring != ""} {
			set realstring "$realstring $rstr"
			} else {
			set realstring $rstr
			}
			# picked up in next cycle
		} elseif { [string first "processes" $line] > 0 \
		      ||   [string first "timeout" $line]  == 0 \
		      ||   [string first "=s==" $line]  > 0 \
		      ||   [string first "=r==" $line]  > 0 } {

			if { $m_typ == 1 && $tsc} {
				set dbox $pbox(0)
			} elseif { $m_typ == 0 && $tsc} {
				if { [info exists Spbox($pno)] == 0 } {
					set Spbox($pno) \
						[mkbox "$pname (proc $pno)" \
						"$pname" "" \
						60 10 \
						[expr 100+$pno*25] \
						[expr 325+$pno*35] ]
					readinfile .c$Spbox($pno).z.t "pan_in"
				}
				set Sdbox $Spbox($pno)
			}
			set pln 0;	# prevent tag update
		} elseif {$syntax == 0 && [string first " = " $line] > 0 } {
				set isvar [string first "=" $line]
				set isvar [expr $isvar + 1]
				set varvl [string range $line $isvar end]
				set isvar [expr $isvar - 2]
				set varnm [string range $line 0 $isvar]
				set varnm [string trim $varnm "	"]
				set Varnm($varnm) $varvl
				set isvar 1
		} elseif { [scan $line " %s %d " varnm qnr] == 2} {
			if {$syntax == 0 &&  [string compare $varnm "queue"] == 0} {
				set isvar [string last ":" $line]
				set isvar [expr $isvar + 1]
				set varvl [string range $line $isvar end]
				set XX [string first "(" $line]
				set YY [string last  ")" $line]
				set ZZ [string range $line $XX $YY]
				set Queues($qnr) $varvl
				if {[info exists Alias($qnr)]} {
				if {[string first $ZZ $Alias($qnr)] < 0} {
					set Alias($qnr) "$Alias($qnr), $ZZ"
				}
				} else {
					set Alias($qnr) $ZZ
				}
				set isvar 1
			}
		} elseif {[string length $line] == 0} {
			if {$dontwait == 0} { set stepper 0 }
			set pln 0
			set Depth [expr $Depth + 1]
			set Seq($Depth) [tell $fd]
			set dontwait 0
		}


	if {$syntax == 0} {
		if {[string first "terminates" $line] > 0} {
			set pln -1
			set stmnt "<stop>"
		}
##NEW
		if {$pln > 0 && [string first "pan_in" $Fnm] >= 0} {
			.inp.t tag remove hilite 0.0 end
			src_line $pln
		}
##END
		if {$m_typ == 0} {
			if {$pln > 0 && [string first "pan_in" $Fnm] >= 0} {
				catch {
				.c$Sdbox.z.t yview -pickplace $pln.0
				.c$Sdbox.z.t tag remove Rev 1.0 end
				.c$Sdbox.z.t tag add Rev $pln.0 $pln.end
				.c$Sdbox.z.t tag configure Rev \
					-background $FG -foreground $BG
			}	}
		} elseif {$m_typ == 1} {
			if { [info exists pbox($pno)] == 0 } {
				set pbox($pno) [mkbox \
					"Proc $pno ($pname)" \
					"Proc$pno" "proc.$pno.out" \
					60 10 \
					[expr 100+$pno*25] \
					[expr 325+$pno*35] ]
			}
			set dbox $pbox($pno)
			catch {
				.c$dbox.z.t yview -pickplace end
				.c$dbox.z.t insert end "$line\n"
			}
		} elseif {$m_typ == 2 && $pln != 0 \
		&&  [string first "unexecutable, " $line] < 0} {
			catch { .c$dbox.z.t yview -pickplace end }
			catch { .c$dbox.z.t insert end "$pno:$pln" }
			for {set i $pno} {$i > 0} {incr i -1} {
				catch { .c$dbox.z.t insert end "\t|" }
			}
			catch { .c$dbox.z.t insert end "\t|>$stmnt\n" }
		}
		if {$msc && $pln != 0} {
			set Mcont "--"
			set HAS 0
			if { [scan $stmnt "values: %d!%d" inq inp1] == 2 \
			||   [scan $stmnt "values: %d!%s" inq inp2] == 2 } {
				set HAS   [string first "!" $stmnt]
				incr HAS
				set Mcont [string range $stmnt $HAS end]
				set HAS 1
			} elseif { [scan $stmnt "values: %d?%d" inq inp1] == 2 \
			|| [scan $stmnt "values: %d?%s" inq inp2] == 2 } {
				set HAS   [string first "?" $stmnt]
				incr HAS
				set Mcont [string range $stmnt $HAS end]
				set HAS 2
			} elseif { [string first "-" $stmnt] == 0} {
				set HAS 3
				if {$HAS_CYCLE} {
					set stmnt [format "Cycle>>"]
				} else {
					set stmnt [format "<waiting>"]
				}
			} elseif { [string first "<stop>" $stmnt] == 0} {
				set HAS 3
				set stmnt [format "<stopped>"]
			}
			if {$pno+1 > $Seenpno} { set Seenpno [expr $pno+1] }
			set XLOC [expr (1+$pno)*$XSZ]
			set YLOC [expr $Pstp*$YSZ]
			if {[string first "printf('MSC: " $stmnt] == 0} {
				set VERBOSE 1
				set stmnt $realstring
				if {[string first "BREAK" $realstring] >= 0} {
					BreakPoint
				}
				set realstring ""
			} else {
				set VERBOSE 0
			}
			catch {
			if {$VERBOSE \
			||  $HAS != 0 \
			||  [info exists R($pstp,$pno)]} {

				if { $SparseMsc == 1 \
				||   [info exists Plabel($pno)] == 0 \
				||   ([info exists R($pstp,$pno)] == 0 \
				&&   ($HAS != 1 \
				||   [info exists HasBox($YLOC,[expr 1+$pno])])) } {
					incr Pstp
					for	{set i 1} \
						{$Pstp > 1 && $i <= $Seenpno} \
						{incr i} {
						if {[info exists HasBox($YLOC,$i)]} {
							continue
						}
						set TMP1 [expr $i*$XSZ]
						set lncol $Blue
						set lnwdt 1
						catch {
							if {$nwcol($i)} {
								set lncol "gray"
								set lnwdt 15
						}	}
						.f$dbox2.c create line \
						$TMP1 $YLOC $TMP1 \
						[expr $YLOC+$YSZ] \
						-width $lnwdt \
						-fill $lncol
					}
					if {[info exists HasBox($YLOC,[expr 1+$pno])]} {
						set YLOC [expr $Pstp*$YSZ]
				}	}
				if {$HAS == 1 || $HAS == 2} {
					set stmnt [string range $stmnt 8 end]
				}
				if { [info exists Plabel($pno)] == 0} {
					set Plabel($pno) 0
					if {$SparseMsc == 0} {
						set HasBox($YLOC,[expr 1+$pno]) 1
					}
					.f$dbox2.c create rectangle \
						[expr $XLOC-20] $YLOC \
						[expr $XLOC+20] \
						[expr $YLOC+$YSZ] \
						-outline $Red -fill $Yellow

						if {$pname != "TRACK"} {
							.f$dbox2.c create text $XLOC \
								[expr $YLOC+$YSZ/2] \
								-font $SmallFont \
								-text "$pname:$pno"
						} else {
							.f$dbox2.c create text $XLOC \
								[expr $YLOC+$YSZ/2] \
								-font $SmallFont \
								-text "<show>"
						}

					set YLOC [expr $Pstp*$YSZ]
					incr Pstp
					for	{set i 1} \
						{$Pstp > 1 && $i <= $Seenpno} \
						{incr i} {
						set TMP1 [expr $i*$XSZ]
						set lncol $Blue
						set lnwdt 1
						catch {
							if {$nwcol($i)} {
								set lncol "gray"
								set lnwdt 15
						}	}
						.f$dbox2.c create line \
						$TMP1 $YLOC $TMP1 \
						[expr $YLOC+$YSZ] \
						-width $lnwdt \
						-fill $lncol
					}
				}
				if {(1+$pno) > $NPR} {
					set NPR [expr $pno+2]
					.f$dbox2.c configure \
					 -scrollregion \
					 "[expr -$XSZ/2] 0 \
					  [expr $NPR*$XSZ] [expr $SMX*$YSZ]"
				}
				if {$Pstp > $SMX-2} {
					set SMX [expr 2*$SMX]
					.f$dbox2.c configure \
					 -scrollregion \
					 "[expr -$XSZ/2] 0 \
					  [expr $NPR*$XSZ] [expr $SMX*$YSZ]"
				}

				if { [info exists R($pstp,$pno)] == 0 } {
					if {$VERBOSE == 1} {
						if {[string first "~W " $stmnt] == 0} {
							set BoxFil $White
							set stmnt [string range $stmnt 3 end] 
						} else { if {[string first "~G " $stmnt] == 0} {
							set BoxFil $Green
							set stmnt [string range $stmnt 3 end]
						} else { if {[string first "~R " $stmnt] == 0} {
							set BoxFil $Red
							set stmnt [string range $stmnt 3 end]
						} else { if {[string first "~B " $stmnt] == 0} {
							set BoxFil $Blue
							set stmnt [string range $stmnt 3 end]
						} else { set BoxFil $Yellow } } } }
						set BoxLab $stmnt
						if {[string first "line " $stmnt] == 0} {
							scan $stmnt "line %d" pln
							set Fnm "pan_in"	;# not necessarily right...
						}
					} else {
						set BoxLab $pstp
						set BoxFil $White
					}
					if {$SparseMsc == 0} {
						set HasBox($YLOC,[expr 1+$pno]) 1
					}
					set R($pstp,$pno) \
						[.f$dbox2.c create rectangle \
						 [expr $XLOC-20] $YLOC \
						 [expr $XLOC+20] \
						 [expr $YLOC+$YSZ] \
						 -outline $Blue -fill $BoxFil]
					set T($pstp,$pno) \
						[.f$dbox2.c create text \
						 $XLOC \
						 [expr $YLOC+$YSZ/2] \
						-font $SmallFont \
						-text $BoxLab]
					#if {$Pstp > $YNR-2} {
					#	.f$dbox2.c yview \
					#	 [expr ($Pstp-$YNR)]
					#}
				}
				if { $HAS == 3 } {
					.f$dbox2.c itemconfigure \
					 $R($pstp,$pno) \
					 -outline $Red -fill $Yellow
				}

				if {$SYMBOLIC} {
					.f$dbox2.c itemconfigure $T($pstp,$pno) \
						-font $SmallFont -text "$stmnt"
				} else {
					if {$VERBOSE == 0 } {
						.f$dbox2.c bind $T($pstp,$pno) <Any-Enter> "
						.f$dbox2.c itemconfigure $T($pstp,$pno) \
							-font $BigFont -text {$stmnt}
						.inp.t tag remove hilite 0.0 end
						if {[string first "pan_in" $Fnm] >= 0} {
							src_line $pln
						}
						"
						.f$dbox2.c bind $T($pstp,$pno) <Any-Leave> "
						.f$dbox2.c itemconfigure $T($pstp,$pno) \
							-font $SmallFont -text {$pstp}
						"
					} else {
						.f$dbox2.c bind $T($pstp,$pno) <Any-Enter> "
						.inp.t tag remove hilite 0.0 end
						if {[string first "pan_in" $Fnm] >= 0} {
							src_line $pln
						}
						"
					}
				}
			}

			set YLOC [expr $YLOC+$YSZ/2]
			if {$HAS == 1} {
				if { [info exists Q_add($inq)] == 0 } {
					set Q_add($inq) 0
					set Q_del($inq) 0
				}
				set Slot $Q_add($inq)
				incr Q_add($inq) 1

				set Mesg_y($inq,$Slot) $YLOC
				set Mesg_x($inq,$Slot) $XLOC
				set Q_val($inq,$Slot) $Mcont
			
				set Rem($inq,$Slot) \
					[.f$dbox2.c create text \
					[expr $XLOC-40] $YLOC \
					-font $SmallFont -text $stmnt]
			} elseif { $HAS == 2 } {
				if {$Easy} {
					set Slot $Q_del($inq)
					incr Q_del($inq) 1
				} else {
					for {set Slot $Q_del($inq)} \
						{$Slot < $Q_add($inq)} \
						{incr Slot} {
					if {$Q_val($inq,$Slot) == "_X_"} {
							incr Q_del($inq) 1
						} else {
							break
					}	}

					for {set Slot $Q_del($inq)} \
						{$Slot < $Q_add($inq)} \
						{incr Slot} {
					if {$Mcont == $Q_val($inq,$Slot)} {
						set Q_val($inq,$Slot) "_X_"
						break
					}	}
				}
				if {$Slot >= $Q_add($inq)} {
					add_log "<<error: cannot match $stmnt>>"
				} else {
					set TMP1 $Mesg_x($inq,$Slot)
					set TMP2 $Mesg_y($inq,$Slot)
					if {$XLOC < $TMP1} {
						set Delta -20
					} else {
						set Delta 20
					}
					.f$dbox2.c create line \
						[expr $TMP1+$Delta] $TMP2 \
						[expr $XLOC-$Delta] $YLOC \
						-fill $Red -width 2 \
						-arrow last -arrowshape {5 5 5}

					if {$SparseMsc == 0} {
						set TMP3 5
					} else {
						set TMP3 0
					}

					.f$dbox2.c coords $Rem($inq,$Slot) \
						[expr ($TMP1 + $XLOC)/2] \
						[expr ($TMP2 + $YLOC)/2 - $TMP3]
					.f$dbox2.c raise $Rem($inq,$Slot)
				}
			} }
		}
		if {$pln == 0 && ($gvars || $lvars || $qv)} {
			if {$Vvbox == 0} {
				if {$vv} { set Vvbox [mkbox "Data Values" \
					"Vars" "var.out" 71 19 100 350] }
			} else {
				catch { .c$Vvbox.z.t delete 0.0 end }
			}
			if {$vv} {
				if {$gvars || $lvars} {
					raise .c$Vvbox
					foreach el [lsort [array names Varnm]] {
					if {[string length $Varnm($el)] > 0} {
					catch { .c$Vvbox.z.t insert \
						end "$el = $Varnm($el)\n" }
					}	}
				}
				if {$qv} {
					foreach el [lsort [array names Queues]] {
					catch {
					.c$Vvbox.z.t insert end "queue $el ($Alias($el))\n"
					.c$Vvbox.z.t insert end "	$Queues($el)\n"
					}	}
			}	}
		}
	} else {
			set stepper 0
	}
		if {$isvar == 0} {
			if {$syntax == 1} {
				if {[string first "..." $line] < 0} {
					add_log "$line"
				catch { .c$sbox.z.t insert end "$line\n" }
				catch { .c$sbox.z.t yview -pickplace end }
				}
			} else {
				if {[string length $line] > 0} {
				catch { .c$sbox.z.t insert end "$line\n" }
				catch { .c$sbox.z.t yview -pickplace end }
				}
				if {$m_typ == 2 && \
					[string first "START OF CYCLE" $line] > 0} {
					catch { .c$dbox.z.t yview -pickplace end }
					catch { .c$dbox.z.t insert end "$line\n" }
					catch {
					set XLOC [expr $Seenpno*$XSZ+$XSZ/2]
					set YLOC [expr $Pstp*$YSZ+$YSZ/2]

					.f$dbox2.c create text \
						[expr $XLOC+$XSZ] $YLOC \
						-font $SmallFont \
						-text "Cycle/Waiting" \
						-fill $Red

					.f$dbox2.c create line \
						$XLOC $YLOC \
						[expr $XLOC+$XSZ/2] $YLOC \
						-fill $Red \
						-arrow first -arrowshape {5 5 5}
					}
					set HAS_CYCLE [expr $YLOC+1]
				}
				if {$m_typ == 2 && $HAS == 3 && $HAS_CYCLE != 0} {
					catch {
					set YLOC [expr $Pstp*$YSZ+$YSZ/2]
					set XLOC0 [expr $pno*$XSZ+$XSZ]
					set XLOC [expr $Seenpno*$XSZ+$XSZ]
					.f$dbox2.c create line \
						$XLOC0 [expr $YLOC-$YSZ/2] \
						$XLOC0 $YLOC \
						-fill $Red
					.f$dbox2.c create line \
						$XLOC0 $YLOC $XLOC $YLOC \
						-fill $Red

					set XLOC [expr $Seenpno*$XSZ+$XSZ]

					.f$dbox2.c create line \
						$XLOC $YLOC $XLOC \
						[expr $HAS_CYCLE-1] \
						-fill $Red
		}	}	}	}
		# mystery update:
		if {$tk_major >= 4 || $m_typ != 1} {
			update	;# tk 3.x can crash on this
		}

		if {$syntax == 0 \
		&&  $stop == 0 \
		&&  $stepped == 1 \
		&&  $stepper == 0 \
		&&  $dontwait == 0} {
			update	;# here it is harmless also with tk 3.x
			tkwait variable stepper
		}
	} else {
		if {$s_typ == 0 || $s_typ == 2} {
			add_log "<at end of run>"
		} else {
			add_log "<at end of trail>"
		}
		catch { addscales $dbox2 }
		if {$ebc} { barscales }
		update
		tkwait variable stepper
	}
	}
	# end of guided trail

	while {$stepper == 1} {
		tkwait variable stepper
	}
	teardown
	
	catch "close $fd"
	add_log "<done>"

	update
}

proc teardown {} {
	global m_typ pbox sbox dbox Spbox Vvbox
	global simruns stop stepped stepper howmany

	set simruns 0
	set stop 1
	set stepped 0
	set stepper 0
	catch { set howmany 0 }
	catch {
		if { $m_typ == 1 } {
			foreach el [array names pbox] {
				catch { destroy .c$pbox($el) }
				catch { unset pbox($el) }
			}
		} elseif { $m_typ == 0 } {
			foreach el [array names Spbox] {
				catch { destroy .c$Spbox($el) }
				catch { unset Spbox($el) }
		}	}
	}
	if {[winfo exists .c$sbox]} {
		set x "Simulation Output"
		set PlaceBox($x) [wm geometry .c$sbox]
		set k [string first "\+" $PlaceBox($x)]
		if {$k > 0} {
			set PlaceBox($x) [string range $PlaceBox($x) $k end]
		}
		destroy .c$sbox
	}
	catch { destroy .c$dbox }
	if {[winfo exists .c$Vvbox]} {
		set x "Data Values"
		set PlaceBox($x) [wm geometry .c$Vvbox]
		set k [string first "\+" $PlaceBox($x)]
		if {$k > 0} {
			set PlaceBox($x) [string range $PlaceBox($x) $k end]
		}
		destroy .c$Vvbox
	}
}

set PlaceMenu	"+150+150"

proc pickoption {nm} {
	global howmany Choice PlaceMenu

	catch {destroy .prompt}
	toplevel .prompt
	wm title .prompt "Select"
	wm iconname .prompt "Select"
	wm geometry .prompt $PlaceMenu

	text .prompt.t -relief raised -bd 2 \
		-width [string length $nm] -height 1 \
		-setgrid 1
	pack append .prompt .prompt.t { top expand fillx }
	.prompt.t insert end "$nm"
	for {set i 0} {$i <= $howmany} {incr i} {
		if {[info exists Choice($i)] \
		&&  $Choice($i) != 0 \
		&&  [string first "outside range" $Choice($i)] < 0 \
		&&  [string first "unexecutable," $Choice($i)] <= 0} {
			pack append .prompt \
			  [button .prompt.b$i -text "$i: $Choice($i)" \
			  -anchor w \
			  -command "set howmany $i" ] \
			  {top expand fillx}

			set j [string first "line " $Choice($i)]
			if {$j > 0} {
			  set k [string range $Choice($i) $j end]
			  scan $k "line %d" k
			  bind .prompt.b$i <Enter> "report $k"
			  bind .prompt.b$i <Leave> "report 0"
			  bind .prompt.b$i <ButtonRelease-1> "set howmany $i"
	}	}	}
	tkwait variable howmany
	set PlaceMenu [wm geometry .prompt]
	set k [string first "\+" $PlaceMenu]
	if {$k > 0} {
		set PlaceMenu [string range $PlaceMenu $k end]
	}
	catch { foreach el [array names Choice]  { unset Choice($el) } }
	destroy .prompt
}

proc report {n} {
	.inp.t tag remove hilite 0.0 end
	if {$n > 0} { src_line $n }
}

# validation options panel

set an_typ -1;	set cp_typ 0; set cyc_typ 0
set as_typ -1;  set ie_typ 1; set ebc 0
set ct_typ 0;	set et_typ 1
set st_typ 0;	set se_typ 0; set bf_typ 0
set oct_typ -1;	# remembers last setting used for compilation
set nv_typ 1
set po_typ -1;	set cm_typ 0; set vb_typ 0
set pr_typ 0;	set where 0
set vr_typ 0;	set xu_typ -1
set ur_typ 1;	set vbox 0
set killed 0;	set Job_Done 0;	set tcnt 0
set waitwhat "none"
set not_warned_yet 1

set LastGenerate	""
set LastCompile	""
set NextCompile	""

proc syntax_check {a T} {
	global SPIN BG FG

	mkpan_in
	add_log "$SPIN $a pan_in"
	catch {exec $SPIN $a pan_in >&pan.tmp} err ;# added -v
	set cnt 0
	set maxln 50
	set ef [open pan.tmp r]
	.inp.t tag remove hilite 0.0 end
	.inp.t tag remove sel 0.0 end
	set pln 0
	set allmsg ""
	while {[gets $ef line] > -1} {
		add_log "$line"
		set allmsg "$allmsg\n$line"
		if {[string first "spin: line" $line] >= 0} {
			scan $line "spin: line %d" pln
			src_line $pln
		}
		if {[string first "spin: warning, line" $line] >= 0} {
			scan $line "spin: warning, line %d" pln
			src_line $pln
		}
		incr cnt
	}
	close $ef
	if {$cnt == 0} { add_log "no syntax errors" } else {
		warner $T "$allmsg" 800
	}
	update
}

proc prescan {} {
	global an_typ cp_typ nv_typ po_typ
	global xu_typ as_typ ie_typ

	mkpan_in

	if {$an_typ == -1} {
		set an_typ 0
		set nv_typ [hasWord "never"]
		if {[hasWord "accept.*:"]} {
			set an_typ 1
			set cp_typ 2
		} elseif {[hasWord "progress.*:"]} {
			set an_typ 1
			set cp_typ 1
		}
	}
	if {$po_typ == -1} {
		if {[hasWord "_last"] \
		||  [hasWord "provided.*\\("] \
		||  [hasWord "enabled\\("]} {
			set po_typ 0
		} else {
			set po_typ 1
		}
	}
	if {$xu_typ == -1} {
		if {[hasWord "xr"] || [hasWord "xs"]} {
			set xu_typ 1
		} else {
			set xu_typ 0
		}
	}
	if {$as_typ == -1} {
		if {$an_typ == 0} {
			set as_typ [hasWord "assert"]
			set ie_typ 1
		} else {
			set as_typ 0
			set ie_typ 0
		}
	}
}

proc basicval2 {} {
	global e ival expl HelvBig PlaceSim
	global an_typ cp_typ nv_typ firstime
	global cyc_typ ct_typ lt_typ
	global et_typ st_typ se_typ bf_typ stop
	global vb_typ pr_typ vr_typ ur_typ xu_typ

	set nv_typ 1
	set an_typ 1
	set cp_typ 2

	dump_tl "pan.ltl"

	catch { .menu.run.m entryconfigure 8 -state normal }
	catch { .tl.results.top.rv configure -state normal }
	set stop 0
	set firstime 0
	set lt_typ 1

	catch {destroy .v}
	toplevel .v

	set k [string first "\+" $PlaceSim]
	if {$k > 0} {
		set PlaceSim [string range $PlaceSim $k end]
	}

	wm title .v "LTL Verification Options"
	wm iconname .v "VAL"
	wm geometry .v $PlaceSim

	prescan

	frame .v.correct -relief flat -borderwidth 1m
	frame .v.cframe  -relief raised -borderwidth 1m

	set z .v.correct

	frame $z.rframe  -relief raised -borderwidth 1m

	label $z.rframe.lb \
		-font $HelvBig \
		-text "Options" \
		-relief sunken -borderwidth 1m

	checkbutton $z.rframe.fc -text "With Weak Fairness" \
		-variable cyc_typ \
		-relief flat 
	checkbutton $z.rframe.xu -text "Check xr/xs Assertions" \
		-variable xu_typ \
		-relief flat

	pack append $z.rframe \
		$z.rframe.lb {top pady 4 frame w fillx} \
		$z.rframe.fc {top pady 4 frame w} \
		$z.rframe.xu {top pady 4 frame w filly}

	pack append $z $z.rframe {top frame nw filly}

	label .v.cframe.lb \
		-font $HelvBig \
		-text "Verification" \
		-relief sunken -borderwidth 1m

	radiobutton .v.cframe.ea -text "Exhaustive" \
		-variable ct_typ -value 0 \
		-relief flat 
	radiobutton .v.cframe.sa -text "Supertrace/Bitstate" \
		-variable ct_typ -value 1 \
		-relief flat 
	radiobutton .v.cframe.hc -text "Hash-Compact" \
		-variable ct_typ -value 2 \
		-relief flat 

	pack append .v.cframe .v.cframe.lb {top pady 4 frame nw fillx} \
		.v.cframe.ea {top pady 4 frame nw} \
		.v.cframe.sa {top pady 4 frame nw} \
		.v.cframe.hc {top pady 4 frame nw}

	frame .v.pf -relief raised -borderwidth 1m
	frame .v.pf.mesg -borderwidth 1m
	
	label .v.pf.mesg.loss0 \
		-font $HelvBig \
		-text "A Full Queue" \
		-relief sunken -borderwidth 1m
	radiobutton .v.pf.mesg.loss1 -text "Blocks New Msgs" \
		-variable l_typ -value 0 \
		-relief flat
	radiobutton .v.pf.mesg.loss2 -text "Loses New Msgs" \
		-variable l_typ -value 1 \
		-relief flat
	pack append .v.pf.mesg \
		.v.pf.mesg.loss0 {top pady 4 frame w expand fillx} \
		.v.pf.mesg.loss1 {top pady 4 frame w} \
		.v.pf.mesg.loss2 {top pady 4 frame w}
	pack append .v.pf \
		.v.pf.mesg   {left frame w expand fillx}

	pack append .v \
		.v.cframe  {top frame w fill}\
		.v.correct {top frame w fill}\
		.v.pf {top frame w expand fill}

	pack append .v [button .v.adv -text "\[Set Advanced Options]" \
		-command "advanced_val" ] {top fillx}
	pack append .v [button .v.run -text "Run" \
		-command {runval ".tl.results.t"} ] {right frame se}
	pack append .v [button .v.exit -text "Cancel" \
		-command "set PlaceSim [wm geometry .v]; \
		set stop 1; destroy .v"] {right frame se}
	pack append .v [button .v.help -text "Help" -fg red \
		-command "roadmap2f" ] {right frame se}

	tkwait visibility .v
	raise .v
}

set PlaceBasic	"+200+10"
set PlaceAdv	"+150+10"

proc basicval {} {
	global e ival expl HelvBig PlaceBasic
	global an_typ nv_typ firstime as_typ ie_typ
	global cyc_typ ct_typ lt_typ as_typ ie_typ
	global et_typ st_typ se_typ bf_typ stop
	global vb_typ pr_typ vr_typ ur_typ xu_typ

	catch { .menu.run.m entryconfigure 8 -state normal }
	catch { .tl.results.top.rv configure -state normal }
	set stop 0
	set firstime 0
	set lt_typ 0

	catch {destroy .v}
	toplevel .v

	wm title .v "Basic Verification Options"
	wm iconname .v "VAL"
	wm geometry .v $PlaceBasic

	prescan

	frame .v.correct -relief flat -borderwidth 1m
	frame .v.cframe  -relief raised -borderwidth 1m

	set z .v.correct

	frame $z.rframe  -relief raised -borderwidth 1m

	label $z.rframe.lb \
		-font $HelvBig \
		-text "Correctness Properties" \
		-relief sunken -borderwidth 1m
	radiobutton $z.rframe.sp -text "Safety (state properties)" \
		-variable an_typ -value 0 \
		-relief flat \
		-command { set cyc_typ 0; set cp_typ 0
			if {$an_typ == 0} {
				set as_typ [hasWord "assert"]
				set ie_typ 1
			}
		 }
	frame $z.rframe.sf
	checkbutton $z.rframe.sf.as -text "Assertions" \
		-variable as_typ \
		-relief flat \
		-command {
			set an_typ 0
			if {![hasWord "assert"] && $as_typ==1} then {
				complain6
			}
		 }
	checkbutton $z.rframe.sf.ie -text "Invalid Endstates" \
		-variable ie_typ \
		-relief flat \
		-command { set an_typ 0 }

	frame $z.rframe.sf.fill -width 15
	pack append $z.rframe.sf \
		$z.rframe.sf.fill {left frame w} \
		$z.rframe.sf.as {top pady 4 frame w} \
		$z.rframe.sf.ie {top pady 4 frame w}

	radiobutton $z.rframe.cp -text "Liveness (cycles/sequences)" \
		-variable an_typ -value 1 \
		-relief flat \
		-command {
			set as_typ 0; set ie_typ 0
			if {[hasWord "accept"]} then { set cp_typ 2 }\
			elseif {[hasWord "progress"]} then { set cp_typ 1 } \
			else complain5
		}

	frame $z.rframe.sub
	radiobutton $z.rframe.sub.np -text "Non-Progress Cycles" \
		-variable cp_typ -value 1 \
		-relief flat \
		-command {
			set an_typ 1
			if {![hasWord "progress"] && $cp_typ==1} then {
				complain4
			}
		 }
	radiobutton $z.rframe.sub.ac -text "Acceptance Cycles" \
		-variable cp_typ -value 2 \
		-relief flat \
		-command {
			set an_typ 1
			if {![hasWord "accept"] && $cp_typ==2} then {
				complain1
			}
		}
	checkbutton $z.rframe.sub.fc -text "With Weak Fairness" \
		-variable cyc_typ \
		-relief flat  \
		-command { if {$an_typ==0} then "set cyc_typ 0; complain3" }

	checkbutton $z.rframe.nv -text "Apply Never Claim (If Present)" \
		-variable nv_typ \
		-relief flat \
		-command { if {![hasWord "never"] && $nv_typ==1} then "complain2" }
	checkbutton $z.rframe.ur -text "Report Unreachable Code" \
		-variable ur_typ \
		-relief flat
	checkbutton $z.rframe.xu -text "Check xr/xs Assertions" \
		-variable xu_typ \
		-relief flat

	frame $z.rframe.sub.fill -width 15
	pack append $z.rframe.sub \
		$z.rframe.sub.fill {left frame w} \
		$z.rframe.sub.np {top pady 4 frame w} \
		$z.rframe.sub.ac {top pady 4 frame w} \
		$z.rframe.sub.fc {top pady 4 frame w}

	pack append $z.rframe \
		$z.rframe.lb {top pady 4 frame w fillx} \
		$z.rframe.sp {top pady 4 frame w} \
			$z.rframe.sf {top pady 4 frame w} \
		$z.rframe.cp {top pady 4 frame w} \
			$z.rframe.sub {top pady 4 frame w} \
		$z.rframe.nv {top pady 4 frame w} \
		$z.rframe.ur {top pady 4 frame w} \
		$z.rframe.xu {top pady 4 frame w filly}

	pack append $z $z.rframe {top frame nw filly}

	label .v.cframe.lb \
		-font $HelvBig \
		-text "Search Mode" \
		-relief sunken -borderwidth 1m

	radiobutton .v.cframe.ea -text "Exhaustive" \
		-variable ct_typ -value 0 \
		-relief flat 
	radiobutton .v.cframe.sa -text "Supertrace/Bitstate" \
		-variable ct_typ -value 1 \
		-relief flat 
	radiobutton .v.cframe.hc -text "Hash-Compact" \
		-variable ct_typ -value 2 \
		-relief flat 

	pack append .v.cframe .v.cframe.lb {top pady 4 frame nw fillx} \
		.v.cframe.ea {top pady 4 frame nw} \
		.v.cframe.sa {top pady 4 frame nw} \
		.v.cframe.hc {top pady 4 frame nw}

	frame .v.pf -relief raised -borderwidth 1m
	frame .v.pf.mesg -borderwidth 1m
	
	label .v.pf.mesg.loss0 \
		-font $HelvBig \
		-text "A Full Queue" \
		-relief sunken -borderwidth 1m
	radiobutton .v.pf.mesg.loss1 -text "Blocks New Msgs" \
		-variable l_typ -value 0 \
		-relief flat
	radiobutton .v.pf.mesg.loss2 -text "Loses New Msgs" \
		-variable l_typ -value 1 \
		-relief flat
	pack append .v.pf.mesg \
		.v.pf.mesg.loss0 {top pady 4 frame w fillx} \
		.v.pf.mesg.loss1 {top pady 4 frame w} \
		.v.pf.mesg.loss2 {top pady 4 frame w}
	pack append .v.pf \
		.v.pf.mesg {left frame nw expand fill}

	pack append .v \
		.v.correct {left} \
		.v.cframe  {top frame n expand fill}\
		.v.pf {top frame n expand fill}

	pack append .v [button .v.nvr -text "\[Add Never Claim from File]" \
		-command "call_nvr" ] {top fillx}
	pack append .v [button .v.ltl -text "\[Verify an LTL Property]" \
		-command "destroy .v; call_tl" ] {top fillx}
	pack append .v [button .v.adv -text "\[Set Advanced Options]" \
		-command "advanced_val" ] {top fillx}
	pack append .v [button .v.run -text "Run" \
		-command {set PlaceBasic [wm geometry .v]; runval "0"} ] {right frame se}
	pack append .v [button .v.exit -text "Cancel" \
		-command {set PlaceBasic [wm geometry .v]; \
		set stop 1; destroy .v}] {right frame se}
	pack append .v [button .v.help -text "Help" -fg red \
		-command "roadmap2e" ] {right frame se}

	tkwait visibility .v
	raise .v
}

set HasNever ""

proc call_nvr {} {
	global HasNever
	global xversion Fname nv_typ

	switch [info commands tk_getOpenFile] "" {
		set fileselect "FileSelect open"
	} default {
		set fileselect "tk_getOpenFile \
			-filetypes { \
				{ { Aut files }  .aut } \
				{ { Nvr files }  .nvr } \
				{ { All Files }   *   } \
				}"
	}

	set HasNever [eval $fileselect]

	if {$HasNever == ""} {
		wm title . "SPIN CONTROL $xversion -- File: $Fname"
		set nv_typ 0
	} else {
		set nv_typ 1
		set z [string last "\/" $HasNever]
		if {$z > 0} {
			incr z
			set HsN [string range $HasNever $z end]
		} else {
			set HsN $HasNever
		}
		wm title . "SPIN CONTROL $xversion -- File: $Fname  Claim: $HsN"
	}
}

proc advanced_val {} {
	global e ival expl HelvBig
	global nv_typ firstime PlaceAdv
	global cyc_typ ct_typ
	global et_typ st_typ se_typ bf_typ stop po_typ cm_typ
	global vb_typ pr_typ vr_typ ur_typ xu_typ

	catch { .menu.run.m entryconfigure 8 -state normal }
	catch { .tl.results.top.rv configure -state normal }
	set stop 0
	set firstime 0

	catch {destroy .av}
	toplevel .av

	wm title .av "Advanced Verification Options"
	wm iconname .av "VAL"
	wm geometry .av $PlaceAdv

	frame .av.pans
	frame .av.pans.correct -relief flat
	frame .av.memopts -relief flat;		# memory options panel
	frame .av.oframe  -relief raised -borderwidth 1m ;# error trail options
	frame .av.recomp  -relief raised -borderwidth 1m ;# recompilation

	prescan

	for {set x 0} {$x<=2} {incr x} {
		frame .av.memopts.choice$x -relief flat
		entry .av.memopts.choice$x.e1 -relief sunken -width 20
		label .av.memopts.choice$x.e2 -text $e($x) -relief flat
		.av.memopts.choice$x.e1 insert end $ival($x)

		pack append .av.memopts.choice$x \
			.av.memopts.choice$x.e2 {left  frame w fillx} \
			[button .av.memopts.choice$x.e3 -text $expl($x) \
			-command "d_list $x" ] {right frame e} \
			.av.memopts.choice$x.e1 {right frame e fillx}

		pack append .av.memopts \
			.av.memopts.choice$x { top frame w pady 5 fillx}
	}
	for {set x 7} {$x<=7} {incr x} {
		frame .av.memopts.choice$x -relief flat
		entry .av.memopts.choice$x.e1 -relief sunken -width 20
		label .av.memopts.choice$x.e2 -text $e($x) -relief flat
		.av.memopts.choice$x.e1 insert end $ival($x)

		pack append .av.memopts.choice$x \
			.av.memopts.choice$x.e2 {left  frame w fillx} \
			[button .av.memopts.choice$x.e3 -text $expl($x) \
			-command "d_list $x" ] {right frame e} \
			.av.memopts.choice$x.e1 {right frame e fillx}

		pack append .av.memopts \
			.av.memopts.choice$x { top frame w pady 5 fillx}
	}
	for {set x 3} {$x<=5} {incr x} {
		frame .av.memopts.choice$x -relief flat
		entry .av.memopts.choice$x.e1 -relief sunken -width 20
		label .av.memopts.choice$x.e2 -text $e($x) -relief flat
		.av.memopts.choice$x.e1 insert end $ival($x)

		pack append .av.memopts.choice$x \
			.av.memopts.choice$x.e2 {left  frame w fillx} \
			[button .av.memopts.choice$x.e3 -text $expl($x) \
			-command "d_list $x" ] {right frame e} \
			.av.memopts.choice$x.e1 {right frame e fillx}

		pack append .av.memopts \
			.av.memopts.choice$x { top frame w pady 5 fillx}
	}
	set z .av.pans.correct
	frame $z.rframe  -relief raised -borderwidth 1m
	label $z.rframe.lb3 \
		-font $HelvBig \
		-text "   Error Trapping   " \
		-relief sunken -borderwidth 1m
	radiobutton $z.rframe.c0 -text "Don't Stop at Errors" \
		-variable et_typ -value 0 \
		-relief flat 
	checkbutton $z.rframe.c0a -text "Save All Error-trails" \
		-variable st_typ \
		-relief flat 
	checkbutton $z.rframe.c0b -text "Find Shortest Trail (iterative)" \
		-variable se_typ \
		-relief flat
	checkbutton $z.rframe.c0c -text "Use Breadth-First Search" \
		-variable bf_typ \
		-relief flat
	frame  $z.rframe.basic1 

	frame $z.rframe.cc
	radiobutton $z.rframe.cc.c1 -text "Stop at Error Nr:" \
		-variable et_typ -value 1 \
		-relief flat 
	entry $z.rframe.cc.c2 -relief sunken -width 8
	$z.rframe.cc.c2 insert end "$ival(6)"
	pack append $z.rframe.cc \
		$z.rframe.cc.c1 {left}\
		$z.rframe.cc.c2 {right expand fillx}

	pack append $z.rframe \
		$z.rframe.lb3 { top expand fillx frame nw} \
		$z.rframe.cc  {top pady 4 frame w} \
		$z.rframe.c0  {top pady 4 frame w} \
		$z.rframe.c0a {top pady 4 frame w} \
		$z.rframe.c0b {top pady 4 frame w} \
		$z.rframe.c0c {top pady 4 frame w} \
		$z.rframe.basic1 {top frame w}
	pack append $z $z.rframe {top frame nw expand fill}

	frame .av.pans.pf -relief flat
	set z .av.pans.pf
	frame $z.mesg -relief raised -borderwidth 1m;		# queue loss options
	frame $z.pframe -relief raised -borderwidth 1m
	label $z.pframe.lb2 \
		-font $HelvBig \
		-text "   Type of Run   " \
		-relief sunken -borderwidth 1m
	checkbutton $z.pframe.po -text "Use Partial Order Reduction" \
		-variable po_typ \
		-relief flat 
	checkbutton $z.pframe.cm -text "Use Compression" \
		-variable cm_typ \
		-relief flat 
#	checkbutton $z.pframe.vb -text "Verbose (For Debugging Only)" \
#		-variable vb_typ \
#		-relief flat
	checkbutton $z.pframe.pr -text "Add Complexity Profiling" \
		-variable pr_typ \
		-relief flat
	checkbutton $z.pframe.vr -text "Compute Variable Ranges" \
		-variable vr_typ \
		-relief flat

	pack append $z.pframe \
		$z.pframe.lb2 {top fillx pady 4 frame w} \
		$z.pframe.po {top pady 4 frame w} \
		$z.pframe.cm {top pady 4 frame w} \
		$z.pframe.pr  {top pady 4 frame w} \
		$z.pframe.vr  {top pady 4 frame w}

	pack append .av.pans.pf \
		.av.pans.pf.pframe  {top frame nw expand fill}
	pack append .av.pans \
		.av.pans.correct {left frame nw expand fillx}\
		.av.pans.pf {left frame nw expand fillx}

	button .av.help -text "Help" -fg red -command "roadmap2d"
	button  .av.basic1 -text "Set" -fg red -command "stopval 1" 
	button  .av.basic0 -text "Cancel" -command "stopval 0" 

	pack append .av \
		.av.memopts {top frame w} \
		.av.pans {top fillx} \
		.av.help  {left frame w} \
		.av.basic1 {right frame e} \
		.av.basic0 {right frame e}

	raise .av
}

proc g_list {} {
	global FG BG

	catch {destroy .r}
	toplevel .r

	wm title .r "Options"
	wm iconname .r "Options"

	frame .r.top
		listbox .r.top.list -width  6 -height 3 -relief raised
		listbox .r.top.expl -width 40 -height 3  -relief flat
	pack append .r.top \
		.r.top.list {left}\
		.r.top.expl {left}
	frame .r.bot
		text .r.bot.text -width 40 -height 1 -relief flat
		.r.bot.text insert end "(Double-Click option or Cancel)"
		button .r.bot.quit -text "Cancel" -command "destroy .r"
	pack append .r.bot \
		.r.bot.text {top}\
		.r.bot.quit {frame s}

	frame .r.caps
		text .r.caps.cap1 -width 6 -height -1 -fg blue
		text .r.caps.cap2 -width 30 -height -1 -fg blue
		.r.caps.cap1 insert end "Option:"
		.r.caps.cap2 insert end "Meaning:"
	pack append .r.caps \
		.r.caps.cap1 {left} \
		.r.caps.cap2 {left}

	pack append .r \
		.r.caps  {frame w}\
		.r.top {top expand} \
		.r.bot {bottom expand}

	foreach i { "-o1" "-o2" "-o3" } {
		.r.top.list insert end $i
	}
 
	foreach i { \
		"disable dataflow-optimizations" \
		"disable dead variables elimination" \
		"disable statement merging" } {
		.r.top.expl insert end $i
	}
	bind .r.top.list <Double-Button-1> {
		set extra [selection get]
		.av.memopts.choice5.e1 insert end " $extra"
		destroy .r
	}
}

proc r_list {} {
	global FG BG

	catch {destroy .r}
	toplevel .r

	wm title .r "Options"
	wm iconname .r "Options"

	frame .r.top
		listbox .r.top.list -width  6 -height 8 -relief raised
		listbox .r.top.expl -width 40 -height 8  -relief flat
	pack append .r.top \
		.r.top.list {left}\
		.r.top.expl {left}
	frame .r.bot
		text .r.bot.text -width 40 -height 1 -relief flat
		.r.bot.text insert end "(Double-Click option or Cancel)"
		button .r.bot.quit -text "Cancel" -command "destroy .r"
	pack append .r.bot \
		.r.bot.text {top}\
		.r.bot.quit {frame s}

	frame .r.caps
		text .r.caps.cap1 -width 6 -height -1 -fg blue
		text .r.caps.cap2 -width 30 -height -1 -fg blue
		.r.caps.cap1 insert end "Option:"
		.r.caps.cap2 insert end "Meaning:"
	pack append .r.caps \
		.r.caps.cap1 {left} \
		.r.caps.cap2 {left}

	pack append .r \
		.r.caps  {frame w}\
		.r.top {top expand} \
		.r.bot {bottom expand}

	foreach i { "-d" "-q" "-I" "-h?" "-s" "-A" "-E" "-w?" } {
		.r.top.list insert end $i
	}
 
	foreach i { \
		"print state tables and stop" \
		"require all chans to be empty in valid endstates" \
		"try to find shortest trail" \
		"choose another seed for hash 1..32 (default 1)" \
		"use 1-bit hashing (default is 2-bit)" \
		"ignore assertion violation errors" \
		"ignore invalid endstate errors" \
		"set explicit -w parameter" \
		"" } {
		.r.top.expl insert end $i
	}
	bind .r.top.list <Double-Button-1> {
		set extra [selection get]
		.av.memopts.choice4.e1 insert end " $extra"
		destroy .r
	}
}

proc d_list {nr} {

	if {$nr == 0} { roadmap2a; return }
	if {$nr == 1} { roadmap2b; return }
	if {$nr == 2} { roadmap2c; return }
	if {$nr == 4} { r_list; return }
	if {$nr == 5} { g_list; return }
	if {$nr == 7} { roadmap2k; return }
#	if {$nr != 3} { roadmap2; return }

	catch {destroy .b}
	toplevel .b

	wm title .b "Options"
	wm iconname .b "Options"

	frame .b.top
		scrollbar .b.top.scroll -command ".b.top.list yview"
		listbox   .b.top.list -yscroll ".b.top.scroll set" -relief raised
	pack append .b.top \
		.b.top.scroll {right filly}\
		.b.top.list {left expand}

	frame .b.bot
		text .b.bot.text -width 21 -height 1 -relief flat
		.b.bot.text insert end "(Double-Click option)"
		button .b.bot.quit -text "Cancel" -command "destroy .b"
		button .b.bot.expl -text "Explanations" -command "roadmap6"
	pack append .b.bot \
		.b.bot.text {top frame nw}\
		.b.bot.expl {left}\
		.b.bot.quit {left}


	pack append .b \
		.b.top {top frame nw expand} \
		.b.bot {bottom}

	foreach i { \
		"-DBCOMP" \
		"-DCTL" \
		"-DGLOB_ALPHA" \
		"-DMA=?" \
		"-DNFAIR=?" \
		"-DNIBIS" \
		"-DNOBOUNDCHECK" \
		"-DNOREDUCE" \
		"-DNOCOMP" \
		"-DNOSTUTTER" \
		"-DNOVSZ" \
		"-DPRINTF" \
		"-DRANDSTORE=?" \
		"-DR_XPT" \
		"-DSDUMP" \
		"-DSVDUMP" \
		"-DVECTORSZ=?" \
		"-DW_XPT=?" \
		"-DXUSAFE" \
		} {
		.b.top.list insert end $i
	}
	bind .b.top.list <Double-Button-1> {
		set directive [selection get]
		.av.memopts.choice3.e1 insert end " $directive"
		destroy .b
	}
}

proc complain1 {} {
	set m "warning: there are no accept labels"
	add_log $m
	catch { tk_messageBox -icon info -message $m }
}

proc complain2 {} {
	set m "warning: there is no never claim"
	add_log $m
	catch { tk_messageBox -icon info -message $m }
}

proc complain3 {} {
	set m "weak fairness is irrelevant to state properties"
	add_log $m
	catch { tk_messageBox -icon info -message $m }
}

proc complain4 {} {
	set m "warning: there are no progress labels"
	add_log $m
	catch { tk_messageBox -icon info -message $m }
}

proc complain5 {} {
	global an_typ
	set m "warning: there are neither accept nor progress labels"
	add_log $m
	set an_typ 0
	catch { tk_messageBox -icon info -message $m }
}

proc complain6 {} {
	set m "warning: there are no assert statements in the spec"
	add_log $m
	catch { tk_messageBox -icon info -message $m }
}

proc complain7 {} {
	set m "warning: Breadth-First Search implies a restriction to Safety properties"
	add_log $m
	catch { tk_messageBox -icon info -message $m }
}

proc complain8 {} {
	set m "error: cannot combine -DMA and -DBITSTATE"
	add_log $m
	catch { tk_messageBox -icon info -message $m }
}

proc stopval {how} {
	global stop ival PlaceAdv

	if {$how} {
	set ival(0) "[.av.memopts.choice0.e1 get]"
	set ival(1) "[.av.memopts.choice1.e1 get]"
	set ival(2) "[.av.memopts.choice2.e1 get]"
	set ival(3) "[.av.memopts.choice3.e1 get]"
	set ival(4) "[.av.memopts.choice4.e1 get]"
	set ival(5) "[.av.memopts.choice5.e1 get]"
	set ival(7) "[.av.memopts.choice7.e1 get]"
	set ival(6) "[.av.pans.correct.rframe.cc.c2 get]"
	}
	set stop 1
	if {[winfo exists .av]} {
		set PlaceAdv [wm geometry .av]
		set k [string first "\+" $PlaceAdv]
		if {$k > 0} {
			set PlaceAdv [string range $PlaceAdv $k end]
		}
		destroy .av
	}
}

proc log {n} {
	set m 1
	set cnt 0
	while {$m<$n} {
		set m [expr $m*2]
		incr cnt
	}
	return $cnt
}

proc bld_v_options {} {
	global an_typ cp_typ cyc_typ as_typ ie_typ
	global et_typ st_typ se_typ l_typ bf_typ
	global ct_typ ival v_options a_options
	global c_options po_typ cm_typ vb_typ
	global pr_typ vr_typ ur_typ xu_typ
	global ol_typ oct_typ nv_typ lt_typ

	set ol_typ $l_typ
	set oct_typ $ct_typ

	set a_options "-a -X"
	if {$l_typ==1}  { set a_options [format "%s -m" $a_options] }
	if {$lt_typ==1} { set a_options [format "%s -N pan.ltl" $a_options] }

	set Mbytes $ival(0)
	catch { set Mbytes [.av.memopts.choice0.e1 get] }

	# the Mstate scale resolution is in thousands: 2^10
	set Mstate [expr 10+[log $ival(1)]]
	catch { set Mstate [expr 10+[log [.av.memopts.choice1.e1 get]]] }
	set Mdepth $ival(2)
	catch { set Mdepth [.av.memopts.choice2.e1 get] }
	catch { set ival(0) "[.av.memopts.choice0.e1 get]" }
	catch { set ival(1) "[.av.memopts.choice1.e1 get]" }
	catch { set ival(2) "[.av.memopts.choice2.e1 get]" }
	catch { set ival(3) "[.av.memopts.choice3.e1 get]" }
	catch { set ival(4) "[.av.memopts.choice4.e1 get]" }
	catch { set ival(5) "[.av.memopts.choice5.e1 get]" }
	catch { set ival(7) "[.av.memopts.choice7.e1 get]" }

	if {$ct_typ == 2} {	;# hash-compact
		set c_options [format "-DHC -DMEMLIM=%d" $Mbytes]

		# in exhaustive mode: #hashtable ~~ #states

		set v_options "-X -m$Mdepth -w$Mstate"

		if {$Mstate >= $Mbytes} {
		catch {
		tk_messageBox -icon info \
			-message "The Estimated Statespace size exceeds \
the maximum that the Memory limit you set would allow."
		}
			return 0
		}
	} elseif {$ct_typ==1}	{
		set c_options [format "-DBITSTATE -DMEMLIM=%d" $Mbytes]

		# in supertrace mode: #bits ~~ 128x #states
		# (effectively the #bytes will be ~~ 16x #states)

		set Mstate [expr 7+$Mstate]
		set v_options "-X -m$Mdepth -w$Mstate"

		if {$Mstate-3 >= $Mbytes} {
		catch {
		tk_messageBox -icon info \
			-message "The Estimated Statespace size exceeds \
maximum allowed by the Memory limit that you set would allow."
		}
			return 0
		}
	} else {
		set c_options [format "-DMEMLIM=%d" $Mbytes]

		# in exhaustive mode: #hashtable ~~ #states

		set v_options "-X -m$Mdepth -w$Mstate"

		if {$Mstate >= $Mbytes} {
		catch {
		tk_messageBox -icon info \
			-message "The Estimated Statespace size exceeds \
the maximum that the Physical Memory limit allows."
		}
			return 0
		}
	}
	set c_options [format "-D_POSIX_SOURCE %s" $c_options]
	if {$an_typ==0}	{ set c_options [format "%s -DSAFETY" $c_options] }
	if {$an_typ==1 && $cp_typ==1}	{ set c_options [format "%s -DNP" $c_options] }
	if {$po_typ==0}	{ set c_options [format "%s -DNOREDUCE" $c_options] }
	if {$cm_typ==1 && $ct_typ!=1}	{ set c_options [format "%s -DCOLLAPSE" $c_options] }
	if {$vb_typ==1}	{ set c_options [format "%s -DCHECK" $c_options] }
	if {$nv_typ==0}	{ set c_options [format "%s -DNOCLAIM" $c_options] }
	if {$se_typ!=0}	{ set c_options [format "%s -DREACH" $c_options] }
	if {$bf_typ!=0}	{
		if {$an_typ != 0} {
			complain7
			set c_options [format "%s -DBFS -DSAFETY" $c_options]
		} else {
			set c_options [format "%s -DBFS" $c_options]
	}	}
	if {$xu_typ==0 && $po_typ!=0}	{ set c_options [format "%s -DXUSAFE" $c_options] }
	if {$pr_typ==1}	{ set c_options [format "%s -DPEG" $c_options] }
	if {$vr_typ==1}	{ set c_options [format "%s -DVAR_RANGES" $c_options] }
	if {$cyc_typ==1}	{
		set c_options [format "%s -DNFAIR=3" $c_options]
	} else {
		set c_options [format "%s -DNOFAIR" $c_options]
	}
	set foo $ival(3)
	catch { set foo [.av.memopts.choice3.e1 get] }

	if {[string first "-DBITSTATE" $c_options] > 0 && [string first "-DMA" $foo] > 0} {
		complain8
	}
	set c_options [format "%s %s" $c_options $foo ]

	set foo $ival(4)
	catch { set foo [.av.memopts.choice4.e1 get] }
	set v_options [format "%s %s" $v_options $foo ] 

	set foo $ival(5)
	catch { set foo [.av.memopts.choice.5.e1 get] }
	set a_options [format "%s %s" $a_options $foo ]

	if {$an_typ==0 && $as_typ==0} { set v_options [format "%s -A" $v_options] }
	if {$an_typ==0 && $ie_typ==0} { set v_options [format "%s -E" $v_options] }
	if {$an_typ==1 && $cp_typ==1} { set v_options [format "%s -l" $v_options] }
	if {$an_typ==1 && $cp_typ==2} { set v_options [format "%s -a" $v_options] }
	if {$cyc_typ==1} { set v_options [format "%s -f" $v_options] }
	if {$ur_typ==0}	{ set v_options [format "%s -n" $v_options] }
	if {$st_typ==1}	{ set v_options [format "%s -e" $v_options] }
	if {$se_typ!=0}	{ set v_options [format "%s -i" $v_options] }
	if {$et_typ==0}	{ set v_options [format "%s -c0" $v_options] }
	if {$et_typ==1}	{ set v_options [format "%s -c%s" $v_options $ival(6)] }
	if {$ct_typ==1 && $ival(7) != 2} { set v_options [format "%s -k%s" $v_options $ival(7)] }
	return 1
}

set mt 0
set skipmax 10

proc runval {havedest} {
	global Unix CC SPIN Fname PlaceSim
	global v_options a_options notignored
	global c_options Job_Done mt skipmax
	global stop s_typ vbox waitwhat not_warned_yet
	global LastGenerate LastCompile NextCompile

	set stop 0
	if {[bld_v_options] == 0} {
		advanced_val
		return
	}
	if {[winfo exists .v]} {
		set PlaceSim [wm geometry .v]
		destroy .v
	}
	catch {destroy .av}
	if {[string first "\?" $c_options] > 0} {
		add_log "error: undefined '?' in optional compiler directives"
		return
	}
	if {[string first "\?" $v_options] > 0} {
		add_log "error: undefined '?' in extra runtime options"
		return
	}
	add_log "<starting verification>"
	if {$havedest != "0"} {
		$havedest insert end "<starting verification>\n"
	}

	set nochange [no_change]
	if {$nochange == 0} { set LastGenerate "" }

	if {$LastGenerate == $a_options} {
		add_log "<no code regeneration necessary>"
		if {$havedest != "0"} {
			$havedest insert end "<no code regeneration necessary>\n"
		}
		set errmsg 0
	} else {
		set LastCompile ""
		add_log "$SPIN $a_options pan_in"; update
		if {$havedest != "0"} {
			$havedest insert end "$SPIN $a_options pan_in\n"
		}
		if {$Unix} {
			catch {eval exec $SPIN $a_options pan_in} errmsg
		} else {
			catch {eval exec $SPIN $a_options pan_in >&pan.tmp}
			set errmsg [msg_file pan.tmp 0]
		}
		if {[string length $errmsg]>0} {
			set foo [string first "Exit-Status 0" $errmsg]
			if {$foo<0} {
				add_log "$errmsg"
				if {$havedest != "0"} {
					$havedest insert end "$errmsg\n"
				}
				update
				return
			}
			incr foo -2
			set errmsg [string range $errmsg 0 $foo]
			add_log "$errmsg"
			if {$havedest != "0"} {
				$havedest insert end "$errmsg\n"
			}
	}	}
	set LastGenerate $a_options

	set NextCompile "$CC -o pan $c_options pan.c"

	if {$havedest != "0"} {
		catch { $havedest yview -pickplace end }
	}
	if {$LastCompile == $NextCompile && [file exists pan]} {
		add_log "<no recompilation necessary>"
		if {$havedest != "0"} {
			$havedest insert end "<no recompilation necessary>\n"
		}
		set errmsg 0
	} else {
		add_log $NextCompile
		if {$havedest != "0"} {
			$havedest insert end "$NextCompile\n"
		}
		catch {
			warner "Compiling" "Please wait until compilation of the \
executable produced by spin completes." 300
		}
		update
		if {$Unix} {
			rmfile "./pan"
			catch {eval exec $NextCompile >pan.tmp 2>pan.err}
		} else {
			rmfile "pan.exe"
			catch {eval exec $NextCompile >pan.tmp 2>pan.err}
		}

		set errmsg [msg_file pan.tmp 0]
		if {[string length $errmsg] == 0} {
			set errmsg [msg_file pan.err 0]
		} else {
			set errmsg "$errmsg\n[msg_file pan.err 0]"
		}

		catch {destroy .warn}

		auto_reset
		if {$Unix} {
			if {[auto_execok "./pan"] == "" \
			||  [auto_execok "./pan"] == 0} {
				add_log "$errmsg"
				add_log "compilation failed"
				if {$havedest != "0"} {
					$havedest insert end "<compilation failed>\n"
				}
				update
				return
			}
		} else {
			if {[auto_execok "pan"] == "" \
			||  [auto_execok "pan"] == 0} {
				add_log "$errmsg"
				add_log "compilation failed"
				if {$havedest != "0"} {
					$havedest insert end "<compilation failed>\n"
				}
				update
				return
	}	}	}

	set LastCompile $NextCompile
	set NextCompile ""

	if {$Unix} {
		set PREFIX "time ./pan -v"
	} else {
		set PREFIX "pan -v"
	}
	add_log "$PREFIX $v_options"; update
	if {$havedest != "0"} {
		$havedest insert end "$PREFIX $v_options\n"
		catch { $havedest yview -pickplace end }
	}
	set errmsg ""
	update
	rmfile pan.out
	set errmsg [catch {eval exec $PREFIX $v_options >&pan.out &}]
	if {$errmsg} {
		add_log "$errmsg"
		add_log "could not run verification"
		if {$havedest != "0"} {
			$havedest insert end "<could not run verification>\n"
		}
		update
		return
	}

	set not_warned_yet 1
	if {$havedest != "0"} {
		set vbox $havedest
	} else {
		set vv [mkbox "Verification Output" "VerOut" "$Fname.out" 80 20]
		set vbox .c$vv.z.t
	}
	update
	set mt 0
	after 5000 outcheck $vbox
	update
}

proc outcheck {vbox} {
	global stop where not_warned_yet runtime mt Unix Fname FG skipmax

	set firstline 1
	set have_trail 0
	set po_red_viol 0

	if {[winfo exists $vbox] == 0} {
		add_log "<verification output panel $vbox was closed>"
		return
	}

	set erline 0; set lnr 0
	set nomem 0; set nerr 0

	if {$stop == 0} {
	  catch { set nt [file mtime pan.out] }
	  if {$mt == $nt && $skipmax > 0} {
		incr skipmax -1
	  } else {
	  set mt $nt
	  set skipmax 10
	  set fd [open pan.out r]
	  while {[gets $fd line] > -1} {
		if {$firstline} {
			set firstline 0
			set nerr 0
			set trunc 0
			set nomem 0
			.inp.t tag remove hilite 0.0 end
			catch { $vbox delete 0.0 end }
			set lnr 0
		}
		catch { $vbox insert end "$line\n" }
		incr lnr
		catch { $vbox yview -pickplace end }
		update

		if {[string first "line" $line]>=0} {
			scan $line "\tline %d" where
			catch { src_line $where }
		}
		if {[string first "State-vector" $line] == 0 \
		||  ([string first "error:" $line] == 0 \
		&&   [string first "error: max search depth too small" $line] != 0)} {
			set stop 1	;# run must have completed
			set nerr [string first "errors:" $line]
			if {$nerr > 0} {
			set part [string range $line $nerr end]
			scan $part "errors: %d" nerr
			if {$nerr == 0} {
				catch { .tl.results.top.title configure\
					-text "Verification Result: valid" -fg $FG
				}
			} else {
				catch { .tl.results.top.title configure\
					-text "Verification Result: not valid" -fg red
				}
			}
			set erline $lnr
			incr erline -1
			}
		}
		if {[string first "search depth too small" $line]>0} {
			set trunc 1
		}
		if {[string first "wrote pan_in.trail" $line]>0} {
			set have_trail 1
		}
		if {[string first "violated: access to chan" $line]>0} {
			set po_red_viol 1
		}
		if {[string first "out of memory" $line]>0} {
			set nomem 1
		}
		if {[string first "A=atomic;" $line]>0} {
			set stop 1
		}
		update
	  }
	  catch "close $fd"
	} }
	if {$nomem || $po_red_viol} { set erline 0 }

	if {$stop || ($have_trail && $po_red_viol==0 && ($nerr>0 || $trunc>0))} {
		catch { $vbox yview 0.0 }
		add_log "<verification done>"
		if {$nerr > 0 || $trunc == 1 || $nomem == 1} {
			if {[file exists pan_in.trail]} {
				cpfile pan_in.trail $Fname.trail
			} elseif {[file exists pan_in.tra]} {
				cpfile pan_in.tra $Fname.trail
			}
			catch { repeatbox $nerr $trunc $nomem }
		}
	} else {
		if {$not_warned_yet} {
			set runtime 0
			set stop 0
			set line "Running verification -- waiting for output"
			catch { $vbox insert end "$line\n" }
			set line "\t(kill background process 'pan' to abort run)"
			catch { $vbox insert end "$line\n" }
			if {$Unix == 0} {
			set line "\t(use ctl-alt-del to kill pan)"
			catch { $vbox insert end "$line\n" }
			} else {
			set line "\t(use /bin/ps to find pid; then: kill -2 pid)"
			catch { $vbox insert end "$line\n" }
			catch { $vbox yview -pickplace end }
			}
			set not_warned_yet 0
		} else {
			incr runtime 5
			if {$stop} {
				add_log "<done>"
				return
			} else {
			if {$runtime%60 == 0} {
				set rt [expr $runtime / 60]
				add_log "verification now running for approx $rt min.."
			}}
			update
		}
		after 5000 outcheck $vbox
	}
}

proc src_line {s} {
	global FG BG
	scan $s %d tgt_lnr

	if {$tgt_lnr > 0} {
	.inp.t tag add hilite $tgt_lnr.0 $tgt_lnr.end
	.inp.t tag configure hilite -background $FG -foreground $BG
		if {$tgt_lnr > 10} {
	.inp.t yview -pickplace [expr $tgt_lnr-10]
		} else {
	.inp.t yview -pickplace [expr $tgt_lnr-1]
		}
	}
}

proc repeatbox { {nerr} {trunc} {nomem}} {
	global s_typ PlaceWarn whichsim

	if {$nerr <= 0 && $trunc <= 0 && $nomem <= 0} { return }

	catch {destroy .rep}
	toplevel .rep

	wm title .rep "Suggested Action"
	wm iconname .rep "Suggest"
	wm geometry .rep $PlaceWarn
	button .rep.b0 -text "Close" -command {destroy .rep}
	button .rep.b1 -text "Run Guided Simulation.." \
		-command {destroy .rep; Rewind }
	button .rep.b2 -text "Setup Guided Simulation.." \
		-command {destroy .rep; simulation }

	if {$nerr>0} {
		message .rep.t -width 500 -text "\
Optionally, repeat the run with a different search\
depth to find a shorter path to the error.

Or, perform a GUIDED simulation to retrace\
the error found in this run, and skip\
the first series of steps if the error was\
found at a depth greater than about 100 steps)."
		set s_typ 1
		set whichsim 0

		pack append .rep .rep.t {top expand fill}
		pack append .rep .rep.b0 {right}
		pack append .rep .rep.b1 {right}
		pack append .rep .rep.b2 {right}
	} else {

		if {$nomem>0} {
			message .rep.t -width 500 -text "\
Make sure the Physical Memory parameter (under Advanced\
Options) is set to the maximum physical memory available.\
If not, repeat the verification with the true maximum.\
(Don't set it higher than the physical limit though.)\
Otherwise, use compression, or switch to a\
Supertrace Verification."
		} else {

			if {$trunc} {

			message .rep.t -width 500 -text "\
If the search was incomplete (truncated)\
because the max search depth was too small,\
try to increase the depth limit (it is set\
in Advanced Options of the Verification Parameters\
Panel), and repeat the verification."
			}
		}
		pack append .rep .rep.t {top expand fill}
		pack append .rep .rep.b0 {right}
	}
}

# Main startup and arg processing
# this is at the end - to make sure all
# proc declarations were seen

if {$argc == 1} {
	set Fname "$argv"
	wm title . "SPIN CONTROL $xversion -- File: $Fname"
	cleanup 0
	cpfile $argv.trail pan_in.trail
	reopen

	check_xsp $argv
}

focus .inp.t
update
