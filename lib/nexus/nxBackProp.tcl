#----------------------------------------------------------------------
# nxBackProp.tcl
#
#	The "Back Propagation" dialog
#
#
#----------------------------------------------------------------------


#----------------------------------------------------------------------
#
#			Back Propagation Dialog
#
#----------------------------------------------------------------------
proc nxMain:Learn:Back {} {
    set w .backprop

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    nxOpenDialog $w "Back Propagation"

    option add *backprop*TixComboBox*Label.width 19

    #----------------------------------------
    # The "Back Prop. Learning" group
    #----------------------------------------
    frame $w.f1
    label $w.backlearn -text "Back Prop. Learning: " -width 19 -anchor e
    radiobutton $w.f1.off -text "OFF" -variable $w.learning \
	-value off
    radiobutton $w.f1.on  -text "ON"  -variable $w.learning \
	-value on

    pack $w.backlearn $w.f1.off $w.f1.on -in $w.f1 -side left -fill y -padx 4
    pack $w.f1 -fill x -padx 6 -pady 6

    #----------------------------------------
    # The "Output File" group
    #----------------------------------------
    nxMkFileSelect $w.f2 -entry $w.outfile \
	    -label $w.f2.lab -text "Training Batch File:  "\
	    -command "nxBack:GetOutFile $w"
    $w.outfile config -textvariable $w.outfile
    $w.f2.lab config  -width 19 -anchor e
    pack $w.f2 -fill x -padx 4 -pady 6 -anchor w
    bind $w.outfile <Return> {;}

    #----------------------------------------
    # The "Hidden Network Name" group
    #----------------------------------------
    tixCombobox $w.hidnet -label "Hidden Network: " -labelside left\
	-dropdown false -editable false \
	-browsecmd "nxSelectNet $w $w.hidnet_name" \
	-command   "nxSelectNet $w $w.hidnet_name" \
	-options {
	    label.anchor ne
	    slistbox.scrollbar y
	}
    $w.hidnet subwidget listbox config -height 4
    pack $w.hidnet -side top -pady 1 -padx 4 -anchor w

    #----------------------------------------
    # The "Output Network Name" group
    #----------------------------------------
    tixCombobox $w.outnet -label "Output Network: " -labelside left \
	-dropdown false -editable false \
	-browsecmd "nxSelectNet $w $w.outnet_name" \
	-command   "nxSelectNet $w $w.outnet_name" \
	-options {
	    label.anchor ne
	    slistbox.scrollbar y
	}
    $w.outnet subwidget listbox config -height 4
    pack $w.outnet -side top -pady 1 -padx 4 -anchor w

    
    #----------------------------------------
    # The "Learning rate" group
    #----------------------------------------
    frame $w.f5
    label $w.f5.lab -text "Learning rate: " -width 19 -anchor e
    tixControl $w.learnrate -variable $w.learnrate_value\
	-ulimit 1.0 -llimit 0.0 -step 0.1\
	-options {entry.width 10}
    label $w.f5.hint -bitmap [tix getbitmap carriage_return]

    pack $w.f5.lab -in $w.f5 -fill y -side left
    pack $w.learnrate  -in $w.f5 -fill both \
	    -side left -expand yes
    pack $w.f5.hint -in $w.f5 -fill y -padx 4 -pady 4 -side left

    pack $w.f5 -padx 0 -pady 6 -anchor w

    #----------------------------------------
    # The "Momentum" group
    #----------------------------------------
    frame $w.f6
    label $w.f6.lab -text "Momentum: " -width 19 -anchor e
    tixControl $w.momentum -variable $w.momentum_value\
	-ulimit 1.0 -llimit 0.0 -step 0.1\
	-options {entry.width 10}
    label $w.f6.hint -bitmap [tix getbitmap carriage_return]

    pack $w.f6.lab -in $w.f6 -fill y -side left
    pack $w.momentum  -in $w.f6 -fill both \
	    -side left -expand yes
    pack $w.f6.hint -in $w.f6 -fill y -padx 4 -pady 4 -side left

    pack $w.f6 -padx 0 -pady 6 -anchor w

    #----------------------------------------
    # The "Save Error" group
    #----------------------------------------
    frame $w.sep0 -height 2 -bd 1 -relief sunken
    pack  $w.sep0 -fill x -padx 3 -pady 6
    frame $w.f7
    label $w.error -text "Save Error: "
    radiobutton $w.f7.off -text "OFF" -variable $w.saveerr \
	-value off
    radiobutton $w.f7.on  -text "ON"  -variable $w.saveerr \
	-value on

    pack $w.error $w.f7.off $w.f7.on -in $w.f7 -side left -fill y -padx 4
    pack $w.f7 -fill x -padx 4 -pady 6

    # This "trace" will config the behavior of the file entry properly
    #
    global $w.saveerr
    trace variable $w.saveerr w "nxBack:SaveErrTrace $w"

    #----------------------------------------
    # The Error File entry
    #----------------------------------------
    nxMkFileSelect $w.f10 -entry $w.errfile \
	-label $w.f10.lab -text "Filename:  "\
	    -command "nxBack:GetErrFile $w"
    $w.errfile config -textvariable $w.filename_err
    pack $w.f10 -fill x -padx 8 -pady 2 -anchor w
    bind $w.errfile <Return> {;}

    #----------------------------------------
    # The separator
    #----------------------------------------
    frame $w.sep1 -height 2 -bd 1 -relief sunken
    pack  $w.sep1 -fill x -padx 3 -pady 6

    #----------------------------------------
    # Buttons
    #----------------------------------------
    frame $w.f8
    button $w.alloc -text "Allocate Parameters" \
	-width 17 -command "nxBack:AllocParam $w"
    button $w.cancel -text "Close" \
	-width 17 -command "nxBack:Cancel $w"

    pack $w.alloc $w.cancel -in $w.f8 -fill y -padx 4 -side left \
	    -expand yes
    pack $w.f8 -fill x -padx 4 -pady 6

    #----------------------------------------
    # Dialog Creation FINISHED!
    #----------------------------------------
    # Initialize the network lists
    #
    nxLoadNetworkNames [$w.hidnet subwidget slistbox]
    nxLoadNetworkNames [$w.outnet subwidget slistbox]

    # Initialize the saveerr file entry
    #
    nxBack:SaveErrTrace $w "" "" w
}

proc nxBack:Cancel {w} {
    wm withdraw $w
}

proc nxBack:AllocParam {w} {
    nxCallCommand TkNx_BpAllocParam
}

#----------------------------------------------------------------------
# Getting Output filename
#----------------------------------------------------------------------

proc nxBack:GetOutFile {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select Back Propagation Output File"
    $open config -command "nxBack:GetOutFile:CB $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.out"
    wm deiconify $open
    $filebox filter
}

# This func gets called when user select a file from "Output file"
proc nxBack:GetOutFile:CB {w filename} {
    upvar #0 $w.outfile outfilename

    set outfilename $filename
    nxEntryAlignEnd $w.outfile
}

#----------------------------------------------------------------------
# Save Err file
#----------------------------------------------------------------------
proc nxBack:SaveErrTrace {w name1 name2 op} {
    global $w.saveerr

    set label  $w.f10.lab
    set entry  $w.errfile
    set button $w.f10:btn

    if {[set $w.saveerr] == "on"} {
	# Enable the file-entry
	#
	$label config -fg black
	$entry config -fg black -state normal
	$button config -fg black -state normal
    } else {
	# Disable the file-entry
	#
	$label config -fg gray40
	$entry config -fg gray40 -state disabled
	$button config -fg gray40 -state disabled
    }
}
proc nxBack:GetErrFile {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select Back Propagation Error File"
    $open config -command "nxBack:GetErrFile:CB $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.error"
    wm deiconify $open
    $filebox filter
}

proc nxBack:GetErrFile:CB {w filename} {
    upvar #0 $w.filename_err errfilename

    set errfilename $filename
    nxEntryAlignEnd $w.errfile
}
