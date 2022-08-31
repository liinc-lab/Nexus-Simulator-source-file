# nxSim.tcl
#
# The Simulate dialog
#
#

#----------------------------------------------------------------------
#
#			Simulate Dialog
#
#----------------------------------------------------------------------

# The "Simulate ..." button in the main window
#
proc nxMainSim {} {
    set w .simulate

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    nxOpenDialog $w "Simulate"

    frame $w.f1
    button $w.go     -text "Go"     -width 5 -command "nxSim:Go $w" \
	-highlightthickness 0
    button $w.stop   -text "Stop"   -width 5 -command "nxSim:Stop $w" \
	-highlightthickness 0
    button $w.cancel -text "Close"  -width 5 -command "nxSim:Cancel $w" \
	-highlightthickness 0

    pack $w.go $w.stop $w.cancel -in $w.f1 -side left -fill y -expand yes \
	 -padx 2

    pack $w.f1 -side top -fill x\
	-padx 4 -pady 2

    #----------------------------------------
    # The Updating group
    #----------------------------------------
    label $w.updating -text "Updating"
    radiobutton $w.sequential -text "Sequential" -variable simulate.updating \
	-value sequential
    radiobutton $w.random     -text "Random"     -variable simulate.updating\
	-value random

    pack $w.updating $w.sequential $w.random -side top -fill x\
	    -padx 4

    #----------------------------------------
    # The "Swap Firing Rate" group
    #----------------------------------------
    label $w.swap -text "Swap Firing Rate"
    frame $w.fr1
    radiobutton $w.fr1.off -text "OFF" -variable simulate.swap \
	-value off
    radiobutton $w.fr1.on  -text "ON"  -variable simulate.swap \
	-value on

    pack $w.swap $w.fr1 -side top -fill x\
	    -padx 4

    pack $w.fr1.off $w.fr1.on -side top -fill x\
	    -padx 4

    #----------------------------------------
    # The "Batch Learning" group
    #----------------------------------------
    label $w.batch -text "Batch Learning"
    frame $w.fr2
    radiobutton $w.fr2.off -text "OFF" -variable simulate.batch \
	-value off
    radiobutton $w.fr2.on  -text "ON"  -variable simulate.batch \
	-value on

    pack $w.batch $w.fr2 -side top -fill x\
	    -padx 4

    pack $w.fr2.off $w.fr2.on -side top -fill x\
	    -padx 4

    #----------------------------------------
    # The "Realtime Display" group
    #----------------------------------------
    label $w.realtime -text "Realtime Display"
    frame $w.fr3
    radiobutton $w.fr3.off     -text "OFF"\
	-variable simulate.realtime \
	-value off
    radiobutton $w.fr3.cell    -text "CELL by CELL"\
	-variable simulate.realtime \
	-value cell
    radiobutton $w.fr3.network -text "NETWORK by NETWORK"\
	-variable simulate.realtime \
	-value network

    pack $w.realtime $w.fr3 -side top -fill x\
	    -padx 4
    pack $w.fr3.off $w.fr3.cell $w.fr3.network -side top -fill x\
	    -padx 4

    #----------------------------------------
    # The "cycles" entry
    #----------------------------------------
    frame $w.f2
    label $w.f2.lab -text "# Cycles: " -width 7 -anchor e
    tixControl $w.cycles -variable simulate.cycles
    label $w.f2.hint -bitmap [tix getbitmap carriage_return]
    pack $w.f2.lab -in $w.f2 -fill y -padx 4 -pady 2 -side left
    pack $w.cycles -in $w.f2 -fill both -padx 4 -pady 2 -side left -expand yes
    pack $w.f2.hint -in $w.f2 -fill y -padx 4 -pady 2 -side left

    pack $w.f2 -fill x -padx 4 -pady 2

    #----------------------------------------
    # The "# cells" entry
    #----------------------------------------
    label $w.ranupdate -text "Random updating:"
    frame $w.f3
    label $w.f3.lab -text "# Cells: " -width 7 -anchor e
    tixControl $w.cells -variable simulate.cells
    label $w.f3.hint -bitmap [tix getbitmap carriage_return]
    pack $w.f3.lab -in $w.f3 -fill y -padx 4 -pady 2 -side left
    pack $w.cells  -in $w.f3 -fill both -padx 4 -pady 2 -side left -expand yes
    pack $w.f3.hint -in $w.f3 -fill y -padx 4 -pady 2 -side left

    pack $w.ranupdate $w.f3 -fill x -padx 4 -pady 2

    #----------------------------------------
    # Interface Created!
    #----------------------------------------
    # The "Random update" entry is off if "sequential" is selected.
    #
    global simulate.updating
    trace variable simulate.updating w "nxSim:UpdateTrace $w"
    nxSim:UpdateTrace  $w simulate.updating "" [set simulate.updating]
}

proc nxSim:Cancel {w} {
    wm withdraw $w
}

proc nxSim:UpdateTrace {w name1 name2 op} {
    global simulate.updating

    set control $w.cells
    set label $w.f3.lab

    if {[set simulate.updating] == "sequential"} {
	$control config -state disabled
	$label   config -fg gray40
    } else {
	$control config -state normal
	$label   config -fg black
    }
}

proc nxSim:Go {w} {
    set last [focus -lastfor $w]
    focus $w.stop
    nxSim:SetWidgetStates $w disabled
    update
    TkNx_DoSimulate go
    update
    nxSim:SetWidgetStates $w normal
    focus $last
}

proc nxSim:Stop {w} {
    TkNx_DoSimulate stop
    nxSim:SetWidgetStates $w normal
}

proc nxSim:SetWidgetStates {w state} {
    if {$state == "disabled"} {
	grab $w
    } else {
	grab release $w
    }

    set buttons [list $w.go $w.cancel $w.sequential $w.random \
	$w.fr1.off $w.fr1.on $w.fr1.off $w.fr2.on $w.fr2.off \
	$w.cycles $w.cells]

    foreach button $buttons {
	$button config -state $state
    }

    set labels [list $w.f2.lab $w.f3.lab]

    foreach label $labels {
	if {$state == "disabled"} {
	    $label config -fg gray40
	} else {
	    $label config -fg black
	}
    }

    if {$state == "normal"} {
	global simulate.updating
	nxSim:UpdateTrace  $w simulate.updating "" [set simulate.updating]
    }
}
