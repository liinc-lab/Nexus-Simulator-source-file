# nxActivity.tcl
#
# 	Inplementing the "Modify Cell Activity" dialog
#

proc nxMainActivity {} {
    nxElectrodeNoteBook act
    }

proc nxMakeActPage {nb page} {
    set w [$nb subwidget $page]

    option add *act*TixControl*entry.width  12
    option add *act*TixControl*label.width  14
    option add *act*TixControl*label.padX   10
    option add *act*TixControl*label.anchor e

    #----------------------------------------
    # The "Electrode" group
    #----------------------------------------

    frame $w.f1

    label $w.mode -text "Mode : "
    radiobutton $w.change  -text "Change"  -variable activity.change \
	-value change 
    radiobutton $w.examine -text "Examine" -variable activity.change \
	-value examine
    pack $w.mode $w.change $w.examine -in $w.f1 -side left -padx 0 -fill x

    pack $w.f1 -side top -fill x -padx 4 -pady 1 -anchor w

    #----------------------------------------
    # Separator
    #----------------------------------------
    frame $w.sep2 -height 2 -bd 1 -relief sunken
    pack  $w.sep2 -fill x -padx 3 -pady 2


    #-----------------------------------------
    #  The "Display Depth" Group, DW 95.03.16
    #-----------------------------------------

    frame $w.f04
    label $w.colour -text "Activity Display Depth:"
    radiobutton $w.grey -text "GreyScale" -variable activity.colour \
	    -value 0
    radiobutton $w.colr -text "Colour" -variable activity.colour \
	    -value 1
    pack $w.colour $w.grey $w.colr -in $w.f04 -side left -padx 0 -pady 0
    pack $w.f04 -side top -pady 1 -padx 8 -anchor w


    #----------------------------------------
    # The "Display Range" Group
    #----------------------------------------
    label $w.range -text "Activity Display Range"
    pack $w.range -side top -padx 4 -pady 2 -anchor w

    nxMkControl $w.f2 $w.min
    $w.min config -label "  Min. Value:  " \
	-step 1 \
	-variable activity.min

    nxMkControl $w.f3 $w.max
    $w.max config -label "  Max. Value:  " \
	-step 1.0 \
	-variable activity.max

    pack $w.f2 $w.f3 -side top -padx 4 -pady 2 -anchor w


    #----------------------------------------
    # The "Firing Rate Adjustment" Group
    #----------------------------------------
    label $w.rate -text "Firing Rate Adjustment"
    pack $w.rate -side top -padx 4 -pady 0 -anchor w

    nxMkControl $w.f4 $w.percent 
    $w.percent config -label "  Percentage:  " \
	    -step 1 -ulimit 100 -llimit 0\
	    -variable activity.rate
    pack $w.f4 -side top -padx 4 -pady 0 -anchor w

    #----------------------------------------
    # Separator
    #----------------------------------------
    frame $w.sep1 -height 2 -bd 1 -relief sunken
    pack  $w.sep1 -fill x -padx 3 -pady 2
    label $w.lab3 -text "Output Activity"
    pack  $w.lab3 -fill x -padx 3 -pady 0

    #----------------------------------------
    # The "Cycle Record" group
    #----------------------------------------
    frame $w.f01
    label $w.cycle -text "Cycle Record: " -width 15 -anchor e
    radiobutton $w.f01.off -text "OFF" -variable activity.cycle \
	-value off
    radiobutton $w.f01.on  -text "ON"  -variable activity.cycle \
	-value on
    pack $w.cycle $w.f01.off $w.f01.on -in $w.f01 -side left -padx 2 -pady 0

    pack $w.f01 -side top -pady 1 -padx 8 -anchor w

    #----------------------------------------
    # The "Network" group
    #----------------------------------------
    tixCombobox $w.net -label "Network:" -labelside left \
	-dropdown false -editable false \
	-browsecmd "nxAct:SelectNet $w" \
	-command   "nxAct:SelectNet $w" \
	-options {
	    label.anchor ne
	    label.width  15
    }
    $w.net subwidget listbox config -height 4 -width 20
    pack $w.net -side top -pady 1 -padx 8 -anchor w

    #----------------------------------------
    # The "Precision" group
    #----------------------------------------
    frame $w.f02
    label $w.precis -text "Precision: " -width 15 -anchor e
    radiobutton $w.1 -text "x.x"      -variable activity.precis \
	-value 1
    radiobutton $w.3 -text "x.xxx"    -variable activity.precis \
	-value 3
    radiobutton $w.6 -text "x.xxxxxx" -variable activity.precis \
	-value 6
    pack $w.precis $w.1 $w.3 $w.6 -in $w.f02 -side left -padx 2 -pady 0

    pack $w.f02 -side top -pady 1 -padx 8 -anchor w

    #----------------------------------------
    # The "Select file" group
    #----------------------------------------
    nxMkFileSelect $w.f03 -entry $w.filename \
	-label $w.f03.lab -text "Filename:"\
	-command "nxAct:GetFile $w"
    $w.filename config -textvariable activity.filename
    $w.f03.lab config -width 15 -anchor e
    pack $w.f03 -fill x -padx 8 -pady 2 -anchor w
    bind $w.filename <Return> "$w.write invoke"

    frame $w.f7
    button $w.write  -text "Write One File" -command "nxAct:Write $w"

    pack $w.write -in $w.f7 -side left -padx 8 -pady 6 -expand yes
    pack $w.f7 -fill x -side top -padx 2

    #----------------------------------------
    # Dialog Creation FINISHED!
    #----------------------------------------
    # Initialize the network list
    #
    nxLoadNetworkNames [$w.net subwidget slistbox]

    #----------------------------------------
    # Set up the trace variables
    #
    global activity.change
    trace variable activity.change w "nxAct:TraceChange $w"
    set activity.change [set activity.change]

    global activity.cycle
    set activity.cycle [set activity.cycle]

    global activity.colour
    set activity.colour [set activity.colour]

    # Need to comment out. We don't want to disable anything so that
    # user can "write one file" even when cycle is off
    #
    #    trace variable activity.cycle w "nxAct:TraceCycle $w"

}



proc nxAct:Write {w} {
    global activity.filename

    if {[set activity.filename] != ""} {
	TkNx_WriteActivityFile
    }
}

proc nxAct:SelectNet {w net} {
    global activity.network

    if {$net != ""} {
	set activity.network $net
    }
}

#----------------------------------------------------------------------
# Getting Output filename
#----------------------------------------------------------------------
proc nxAct:GetFile {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select Activity Output File"
    $open config -command "nxAct:GetFile:CB $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.act"
    wm deiconify $open
    $filebox filter
}

# This func gets called when user select a file from "Output file"
proc nxAct:GetFile:CB {w filename} {
    global activity.filename

    set activity.filename $filename
    nxEntryAlignEnd $w.filename
}

# Need to comment out. We don't want to disable anything so that
# user can "write one file" even when cycle is off
#
# This func gets called when user toggles the "Cycle Record" buttons
#
#proc nxAct:TraceCycle {w name1 name2 op} {
#    global activity.cycle
#
#    set label  $w.f03.lab
#    set entry  $w.filename
#    set button $w.f03:btn
#    set buttons [list $w.1 $w.3 $w.6]
#
#    if {[set activity.cycle] == "on"} {
#	# Enable the file-entry
#	#
#	$label config -fg black
#	$entry config -fg black -state normal
#	$button config -fg black -state normal
#
#	$w.precis config -fg black
#	foreach b $buttons {
#	    $b config -fg black -state normal
#	}
#    } else {
#	# Disable the file-entry
#	#
#	$label config -fg gray40
#	$entry config -fg gray40 -state disabled
#	$button config -fg gray40 -state disabled
#
#	$w.precis config -fg gray40
#	foreach b $buttons {
#	    $b config -fg gray40 -state disabled
#	}
#    }
#
#}



# This func gets called when user toggles the "Mode" buttons
proc nxAct:TraceChange {w name1 name2 op} {
    global activity.change

    set control $w.percent

    if {[set activity.change] == "change"} {
	$control config -state normal
    } else {
	$control config -state disabled
    }
}
