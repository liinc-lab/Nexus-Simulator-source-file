proc nxMain:Learn:RBF {} {
    set w .rbf

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    nxOpenDialog $w "Radial Basis Functions"

    set f1 [nxRBF:MkFrame1 $w]
    set f2 [nxRBF:MkFrame2 $w]
    frame $w.sep -width 2 -bd 1 -relief sunken

    pack $f1 $w.sep $f2 -side left -fill y

    #----------------------------------------
    # Dialog Creation FINISHED!
    #----------------------------------------
    # Initialize the network lists
    #
    nxLoadNetworkNames [$f1.innet subwidget slistbox]
    nxLoadNetworkNames [$f1.hidnet subwidget slistbox]
    nxLoadNetworkNames [$f1.outnet subwidget slistbox]

    # Initialize the saveerr file entry
    #
    nxRBF:SaveErrTrace $w "" "" w
}

proc nxRBF:MkFrame1 {w} {
    set f [frame $w.frame1]

    pack $f -side left -fill y

    #----------------------------------------
    # The "RBF Learning:" group
    #----------------------------------------
    frame $f.f1
    label $f.f1.learn -text "RBF Learning: " -width 15 -anchor e
    radiobutton $f.f1.off -text "OFF" -variable $w.learning -value off
    radiobutton $f.f1.on  -text "ON"  -variable $w.learning -value on
    pack $f.f1.learn  $f.f1.off $f.f1.on -in $f.f1 -side left
    pack $f.f1 -side top -padx 4 -anchor w

    #----------------------------------------
    # The "Input Network" group
    #----------------------------------------
    tixCombobox $f.innet -label "Input Network: " -labelside left \
	-dropdown false -editable false \
	-browsecmd "nxSelectNet $w $w.innet_name" \
	-command   "nxSelectNet $w $w.innet_name" \
	-options {
	    slistbox.scrollbar y
	    entry.width  15
	    label.anchor ne
	    label.padY   4
	    label.width  15
	}
    $f.innet subwidget listbox config -height 4
    pack $f.innet -side top -pady 3 -padx 4 -anchor w -fill x

    #----------------------------------------
    # The "Hidden Network" group
    #----------------------------------------
    tixCombobox $f.hidnet -label "Hidden Network: " -labelside left \
	-dropdown false -editable false \
	-browsecmd "nxSelectNet $w $w.hidnet_name" \
	-command   "nxSelectNet $w $w.hidnet_name" \
	-options {
	    slistbox.scrollbar y
	    entry.width  15
	    label.anchor ne
	    label.padY   4
	    label.width  15
	}
    $f.hidnet subwidget listbox config -height 4
    pack $f.hidnet -side top -pady 3 -padx 4 -anchor w -fill x

    #----------------------------------------
    # The "Output Network" group
    #----------------------------------------
    tixCombobox $f.outnet -label "Output Network: " -labelside left \
	-dropdown false -editable false \
	-browsecmd "nxSelectNet $w $w.outnet_name" \
	-command   "nxSelectNet $w $w.outnet_name" \
	-options {
	    slistbox.scrollbar y
	    entry.width  15
	    label.anchor ne
	    label.padY   4
	    label.width  15
	}
    $f.outnet subwidget listbox config -height 4
    pack $f.outnet -side top -pady 3 -padx 4 -anchor w -fill x

    #----------------------------------------
    # The Buttons
    #----------------------------------------
    frame $f.f2
    button $f.init -text "Initialize" -width 10 \
	    -command "nxRBF:Initialize $w"
    button $f.cancel -text "Close" -width 10 \
	    -command "nxRBF:Cancel $w"

    pack $f.init $f.cancel -in $f.f2 -padx 4 -pady 4 -expand yes -side left
    pack $f.f2 -side top -fill x -padx 4 -pady 4

    return $f
}

proc nxRBF:MkFrame2 {w} {
    set f [frame $w.frame2]

    option add *rbf.frame2*Label.width  16
    option add *rbf.frame2*Label.anchor e
    option add *rbf.frame2*TixControl*entry.width 10
    option add *rbf.frame2*TixControl*ulimit 1.0
    option add *rbf.frame2*TixControl*llimit 0.0
    option add *rbf.frame2*TixControl*step   0.1

    #----------------------------------------
    # The "Output File"
    #----------------------------------------
    nxMkFileSelect $f.f1 -entry $f.outfile \
	    -label $f.f1.lab -text "Training Batch File: "\
	    -command "nxRBF:GetOutFile $w"
    $f.f1.lab config -width 18
    $f.outfile config -textvariable $w.outfile
    pack $f.f1 -fill x -padx 4 -pady 2 -anchor e

    #----------------------------------------
    # The "Initialization ..."
    #----------------------------------------
    frame $f.sep0 -height 2 -bd 1 -relief sunken
    pack $f.sep0 -fill x -side top -padx 2 -pady 7

    label $f.lab0 -text "Initialization: " -width 20 -anchor w
    pack $f.lab0 -side top -anchor w -padx 2

    nxMkControl $f.f01 $f.i_bias
    $f.i_bias config -label "Bias: " \
	    -variable $w.i_bias_value

    nxMkControl $f.f02 $f.smooth
    $f.smooth config -label "Smoothness: " \
	    -variable $w.smooth_value

    pack $f.f01 $f.f02 -side top -padx 4 -anchor w

    #----------------------------------------
    # The "Learning Rates"
    #----------------------------------------
    frame $f.sep1 -height 2 -bd 1 -relief sunken
    pack $f.sep1 -fill x -side top -padx 2 -pady 7
    label $f.learnrate -text "Learning Rates: " -anchor w
    pack $f.learnrate -side top -padx 4 -anchor w

    nxMkControl $f.f03 $f.centers
    $f.centers config -label "Centers: " \
	    -variable $w.centers_value
    nxMkControl $f.f04 $f.l_bias
    $f.l_bias config -label "Bias: " \
	    -variable $w.l_bias_value
    nxMkControl $f.f05 $f.weights
    $f.weights config -label "Weights: " \
	    -variable $w.weights_value

    pack $f.f03 $f.f04 $f.f05 -side top -padx 4 -anchor w

    #----------------------------------------
    # The "Minimum Change ..."
    #----------------------------------------
    frame $f.sep2 -height 2 -bd 1 -relief sunken
    pack $f.sep2 -fill x -side top -padx 2 -pady 7

    nxMkControl $f.f06 $f.min
    $f.min config -label "Minimum Change: " \
	    -variable $w.min_value

    nxMkControl $f.f07 $f.moment
    $f.moment config -label "Momentum: " \
	    -variable $w.moment_value

    pack  $f.f06 $f.f07 -side top -padx 4 -anchor w

    #----------------------------------------
    # The "Save Error"
    #----------------------------------------
    frame $f.sep3 -height 2 -bd 1 -relief sunken
    pack $f.sep3 -fill x -side top -padx 2 -pady 7

    frame $f.f4
    label $f.error -text "Save Error: " -width 15 -anchor e
    radiobutton $f.f4.off -text "OFF" -variable $w.saveerr -value off
    radiobutton $f.f4.on  -text "ON"  -variable $w.saveerr -value on
    pack $f.error  $f.f4.off $f.f4.on -in $f.f4 -side left
    pack $f.f4 -side top -padx 4 -anchor w

    nxMkFileSelect $f.f5 -entry $f.errfile \
	-label $f.f5.lab -text "Filename: "\
	    -command "nxRBF:GetErrFile $w"
    $f.f5.lab config -width 18
    $f.errfile config -textvariable $w.filename_err
    pack $f.f5 -fill x -padx 4 -pady 2 -anchor e


    # This "trace" will config the behavior of the file entry properly
    #
    global $w.saveerr
    trace variable $w.saveerr w "nxRBF:SaveErrTrace $w"

    return $f
}

proc nxRBF:Initialize {w} {
    nxCallCommand TkNx_RBFAllocParam
}

proc nxRBF:Cancel {w} {
    wm withdraw $w
}

# Save output file
#
proc nxRBF:GetOutFile {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select RBF Output File"
    $open config -command "nxRBF:GetOutFile:CB $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.out"
    wm deiconify $open
    $filebox filter
}

proc nxRBF:GetOutFile:CB {w filename} {
    upvar #0 $w.outfile outfilename
    set f $w.frame2

    set outfilename $filename
    nxEntryAlignEnd $f.outfile
}

# Save Err file
#
proc nxRBF:GetErrFile {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select RBF Error File"
    $open config -command "nxRBF:GetErrFile:CB $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.error"
    wm deiconify $open
    $filebox filter
}

proc nxRBF:GetErrFile:CB {w filename} {
    upvar #0 $w.filename_err errfilename

    set errfilename $filename
    nxEntryAlignEnd $w.errfile
}

# Disable the file entry if "save error file" is set to off
#
#
proc nxRBF:SaveErrTrace {w name1 name2 op} {
    global $w.saveerr

    if {[set $w.saveerr] == "on"} {
	# Enable the file-entry
	$w.frame2.f5.lab config -fg black
	$w.frame2.errfile config -fg black -state normal
	$w.frame2.f5:btn config -fg black -state normal
    } else {
	# Disable the file-entry
	#
	$w.frame2.f5.lab config -fg gray40
	$w.frame2.errfile config -fg gray40 -state disabled
	$w.frame2.f5:btn config -fg gray40 -state disabled
    }
}
