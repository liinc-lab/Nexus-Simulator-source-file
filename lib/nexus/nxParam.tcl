#  nxParam.tcl
#
#	The "Set Parameters" dialog box.
#

proc nxMainSetParam {} {
    set w .setparam

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    nxOpenDialog $w "Set Parameters"

    option add *setparam*TixControl*label.width  15
    option add *setparam*TixControl*label.anchor e
    option add *setparam*TixControl*entry.width  8

    #----------------------------------------
    # The "Set Network:" group
    #----------------------------------------
    frame $w.f1
    frame $w.sep1 -bd 1 -height 2 -relief sunken
    label $w.label -text "Parameters: " -pady 2
    frame $w.f1a
    frame $w.f1b
    frame $w.f1c
    pack $w.label -in $w.f1 -side left -anchor n -pady 2
    pack $w.f1a $w.f1b $w.f1c -side top  -in $w.f1 -expand yes -fill both
    pack $w.f1 -side top -anchor w -padx 4
    pack $w.sep1 -side top -fill x -padx 2

    radiobutton $w.clamped   -text "Fully Clamped"   \
	-variable $w.setnet -width 12 -value clamped
    radiobutton $w.unclamped -text "Unclamped"       \
	-variable $w.setnet -width 9 -value unclamped
    radiobutton $w.activity  -text "Activity"        \
	-variable $w.setnet -width 6 -value activity
    radiobutton $w.transfunc -text "Trans. Function" \
	-variable $w.setnet -width 12 -value transfunc
    radiobutton $w.threshold -text "Threshold"       \
	-variable $w.setnet -width 9 -value threshold
    radiobutton $w.scale     -text "Scale"           \
	-variable $w.setnet -width 6 -value scale
    radiobutton $w.offset    -text "Offset"          \
	-variable $w.setnet -width 12 -value offset
    radiobutton $w.updates   -text "Updates"         \
	-variable $w.setnet -width 9 -value updates
    radiobutton $w.decay     -text "Decay"           \
	-variable $w.setnet -width 6 -value decay

    pack $w.clamped   $w.unclamped $w.activity  -in $w.f1a \
	    -side left -anchor w -padx 1 -pady 0
    pack $w.transfunc $w.threshold $w.scale     -in $w.f1b \
	    -side left -anchor w -padx 1 -pady 0
    pack $w.offset    $w.updates   $w.decay     -in $w.f1c \
	    -side left -anchor w -padx 1 -pady 0

    #----------------------------------------
    # The "Selected Network" group
    #----------------------------------------
    tixCombobox $w.net -label "Network:" -labelside left \
 	    -dropdown false -editable false \
	    -command "nxParam:SelectNet $w" \
	    -browsecmd "nxParam:SelectNet $w" \
	    -options {
	slistbox.scrollbar y
	entry.width  15
	label.anchor ne
	label.width  15
    }
    $w.net subwidget listbox config -height 4
    pack $w.net -side top -pady 3 -padx 4 -anchor w

    #----------------------------------------
    # The "Value" group
    #----------------------------------------
    frame $w.f7
    label $w.f7.lab -text "Value: " -width 15 -anchor e
    label $w.f7.hint -bitmap [tix getbitmap carriage_return]
    entry $w.f7.entry -width 25

    bind $w.f7.entry <Return> "nxParam:SetValue $w %W; $w.setval invoke"

    pack $w.f7.lab  $w.f7.entry $w.f7.hint -side left -anchor w
    pack $w.f7 -padx 4 -pady 4 -anchor w


    frame $w.sep2 -bd 1 -height 2 -relief sunken
    pack $w.sep2 -side top -fill x -padx 2

    #----------------------------------------
    # The Buttons
    #----------------------------------------
    frame $w.f2
    pack $w.f2 -padx 4 -pady 4 -side top -anchor w -fill x

    button $w.setval  -width 13 -text "Change Value" \
	    -command "nxParam:Change $w"
    button $w.showval -width 17 -text "Show Current Values" \
	    -command "nxParam:Show $w"
    button $w.cancel  -text "Close" \
	    -command "nxParam:Cancel $w"
    pack $w.setval $w.showval -in $w.f2 \
	    -side left -expand yes -anchor c -padx 4 -pady 4
    pack $w.cancel -in $w.f2 \
	    -side left -expand yes -anchor c -padx 10 -pady 4

    #----------------------------------------
    # Dialog Creation FINISHED!
    #----------------------------------------
    # Initialize the network list
    #
    nxLoadNetworkNames [$w.net subwidget slistbox]
}

proc nxParam:SelectNet {w net} {
    global $w.network

    set $w.network $net
}

proc nxParam:Change {w} {
    TkNx_ChangeParam
}

proc nxParam:Show {w} {
    TkNx_ShowParam
}


proc nxParam:Cancel {w} {
    wm withdraw $w
}

# Set the "value"
#
proc nxParam:SetValue {w entry_widget} {
    global $w.value

    set $w.value [$entry_widget get]
}
