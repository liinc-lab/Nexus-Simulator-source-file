# nxScript.tcl
#
# 	Inplementing the "Script" dialog
#
proc nxMainScript {} {
    set w .script
    set v script

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    nxOpenDialog $w "Script"

    option add *$v*Checkbutton*borderWidth 2
    option add *$v*Checkbutton*relief flat

    frame $w.f1
    pack $w.f1 -fill x -anchor nw

    label $w.lexp -text "Explore Script Variables:"
    radiobutton $w.expoff -text "Off" \
	    -variable $v.explore -value off
    radiobutton $w.expon -text "On" \
	    -variable $v.explore -value on
    pack $w.lexp $w.expoff $w.expon -in $w.f1 -side left -padx 4 -pady 4

    global $v.explore
    trace variable $v.explore w "nxScript:Exp $v"


    frame $w.f9
    button $w.f9.close -text " Close " -command "wm withdraw $w"
    pack $w.f9.close -pady 10 -anchor c
    pack $w.f9 -fill x -expand yes -anchor sw

    frame $w.sep1 -relief sunken -height 2 -bd 1 
    pack $w.sep1 -padx 2 -pady 2 -fill x  -anchor sw
}

proc nxScript:Exp {v name1 name2 op} {
    global $v.explore

    puts [set $v.explore]
}

proc nxLoadSimulation {filename} {
    TkNx_ReadFile L $filename
}

proc nxBuildSimulation {filename} {
    TkNx_ReadFile B $filename
}

