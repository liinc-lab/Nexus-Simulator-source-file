# nxUtils.tcl
#
# 	Some utilities for tk-nexus
#

#----------------------------------------------------------------------
# nxMkListbox
#
#	Create a listbox with a label, and an entry. Bind the entry to
# the current selection of the listbox.
#----------------------------------------------------------------------
proc nxMkListbox {w args} {
    set opt(-label)     {}
    set opt(-geom)      10x8
    set opt(-listbox)   $w:slistbox
    set opt(-entry)     $w:entry
    set opt(-scrollbar) y
    set opt(-width)     20

    tixHandleOptions opt {-label -geom -listbox -entry -scrollbar} $args

    frame $w
    frame $w:bot
    frame $w:ext -width 19

    tixScrolledListbox $opt(-listbox) -scrollbar $opt(-scrollbar) \
	    -geom $opt(-geom)
    label $w:lab -text $opt(-label)
    entry $opt(-entry) -width $opt(-width)

    pack $opt(-listbox) -in $w:bot -side bottom -anchor w -fill x -pady 2
    pack $w:ext         -in $w:bot -side right
    pack $opt(-entry)   -in $w:bot -side left -fill x  -expand yes

    pack $w:lab -in $w -side top -anchor w -padx 4
    pack $w:bot -in $w -side bottom -anchor w -pady 2 -padx 4 \
	     -expand yes -fill y
}

#----------------------------------------------------------------------
# nxMkControl
#
#	Create a control with the appropriate headings and trace variable.
#----------------------------------------------------------------------
proc nxMkControl {frame control} {
    frame $frame
    tixControl $control
    label $frame:hint -bitmap [tix getbitmap carriage_return]

    pack $control $frame:hint -in $frame -side left -anchor w
}

#----------------------------------------------------------------------
# nxMkScale
#
#	Create a control with the appropriate headings and trace variable.
#----------------------------------------------------------------------
proc nxMkScaleSelect {w} {
    frame $w -class ScaleSelect
    label $w.label
    scale $w.scale -orient horizontal -length 150
    pack $w.label $w.scale -in $w -padx 4 -side left
}

#----------------------------------------------------------------------
# nxMkFileSelect
#
#	Create a control with the appropriate headings and trace variable.
#----------------------------------------------------------------------
proc nxMkFileSelect {w args} {
    set opt(-command)  {}
    set opt(-entry)    $w:entry
    set opt(-label)    $w:label
    set opt(-text)     {}

    tixHandleOptions opt {-command -entry -label -text} $args

    frame $w
    frame $w:in -bd 2 -relief sunken
    label $opt(-label) -text $opt(-text)
    entry $opt(-entry) -bd 0\
	-highlightthickness 0
    button $w:btn -bitmap [tix getbitmap sel_file] -command $opt(-command)\
	-highlightthickness 0
    label $w:hint -bitmap [tix getbitmap carriage_return]

    pack $w:btn -in $w:in -side right
    pack $opt(-entry) -in $w:in -side left -expand yes -fill both

    pack $opt(-label) -in $w -side left
    pack $w:in -in $w -side left -expand yes -fill both
    pack $w:hint -in $w -side left
}

#----------------------------------------------------------------------
# Align an entry to its end
#----------------------------------------------------------------------
proc nxEntryAlignEnd {entry} {
    tixDoWhenIdle $entry xview end
}

#----------------------------------------------------------------------
# Load a network list
#----------------------------------------------------------------------
set nx_NetworkLists {}

proc nxLoadNetworkNames {slistbox} {
    global nx_NetworkLists

    if {[lsearch $nx_NetworkLists $slistbox] == -1} {
	lappend nx_NetworkLists $slistbox
    }

    $slistbox subwidget listbox delete 0 end
    TkNx_GetNetworkNames [$slistbox subwidget listbox]
    $slistbox subwidget listbox activate 0
}

proc nxReloadNetworkNames {} {
    global nx_NetworkLists

    foreach slistbox $nx_NetworkLists {
	if [winfo exists $slistbox] {
	    $slistbox subwidget listbox delete 0 end
	    TkNx_GetNetworkNames [$slistbox subwidget listbox]
	    $slistbox subwidget listbox activate 0
	}
    }
}

#----------------------------------------------------------------------
# Put up an error dialog
#----------------------------------------------------------------------
proc nxErrorMsg {message} {
    nxYesNoPrompt -message "Error: $message" -yestext "OK" -useno false
}

proc nxCallCommand {command args} {
    set result [eval $command $args]

    if {[lindex $result 0] == "error"} {
	nxErrorMsg [lindex $result 1]
    }
}

proc nxSelectNet {w var net} {
    global $var

    if {$net != ""} {
	set $var $net
    }
}

#----------------------------------------------------------------------
# nxYesNoPrompt
#
#----------------------------------------------------------------------
proc nxYesNoPrompt {args} {
    set w .nxprompt

    if [winfo exists $w] {
	destroy $w
    }

    set opt(-message)  {}
    set opt(-yescmd)  "{;}"
    set opt(-yestext)  Yes
    set opt(-nocmd)   "{;}"
    set opt(-notext)   No
    set opt(-useyes)   true
    set opt(-useno)    true

    tixHandleOptions opt {
	-message -yescmd -nocmd -yestext -notext -useyes -useno
    } $args

    toplevel $w
    wm title $w "Confirmation"
    frame $w.f1 -bd 1 -relief raised
    frame $w.f2 -bd 1 -relief raised
    pack $w.f1 -side top -expand yes -fill both
    pack $w.f2 -side top -fill both

    message $w.f1.message -text $opt(-message) -width 200

    if {$opt(-useyes)} {
	button $w.f2.yes -width 6\
	    -text $opt(-yestext)\
	    -command [list nxYesNo:Done $w $opt(-yescmd)]
	pack $w.f2.yes -side left -expand yes -padx 15 -pady 10
   }
    if {$opt(-useno)} {
	button $w.f2.no -width 6\
	    -text $opt(-notext)\
	    -command [list nxYesNo:Done $w $opt(-nocmd)]
	pack $w.f2.no -side left -expand yes -padx 15 -pady 10
    }
    pack $w.f1.message -expand yes -fill both

    wm protocol $w WM_DELETE_WINDOW "$w.f2.no invoke"

    tkwait visibility $w
    grab $w
}

proc nxYesNo:Done {w command} {
    grab release $w
    destroy $w

    if {$command != "{;}"} {
	eval $command
    }
}

#----------------------------------------------------------------------
# nxQueryYesNo
#
#----------------------------------------------------------------------
proc nxQueryYesNo:Answer {ans} {
    global queryAnswer

    set queryAnswer $ans
}

proc nxQueryYesNo {s} {
    global queryAnswer
    set queryAnswer 0

    nxYesNoPrompt -message $s\
	-yescmd "nxQueryYesNo:Answer 1" \
	-nocmd  "nxQueryYesNo:Answer 0"

    tkwait variable queryAnswer

    return $queryAnswer
}

