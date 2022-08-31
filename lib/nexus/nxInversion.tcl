# nxInversion.tcl --
#
#	Implement the "Network Inversion" dialog
#
#

proc nxMain:Learn:NetInv {} {
    set w .inver

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    nxOpenDialog $w "Network Inversion"

    option add *inver*TixControl*entry.width 10
    option add *inver*TixControl*label.width 25
    option add *inver*TixControl*label.anchor e

    #----------------------------------------
    # The "Inversion Learning" group
    #----------------------------------------
    frame $w.f1
    label $w.invlearn -text "Inversion Learning: " -width 18 -anchor e
    radiobutton $w.f1.off -text "OFF" -variable $w.learning -value off
    radiobutton $w.f1.on  -text "ON"  -variable $w.learning -value on

    pack $w.invlearn $w.f1.off $w.f1.on -in $w.f1 -side left -fill y -padx 0
    pack $w.f1 -fill x -padx 4 -pady 0 -anchor e

    #----------------------------------------
    # The "Output File"
    #----------------------------------------
    nxMkFileSelect $w.f2 -entry $w.outfile \
	    -label $w.f2.lab -text "Desired Output File:"\
	    -command "nxInver:GetOutFile $w"
    $w.outfile config -textvariable $w.outfile
    $w.f2.lab config -anchor e -width 18
    pack $w.f2 -fill x -padx 4 -pady 2 -anchor w
    bind $w.outfile <Return> {;}

    #----------------------------------------
    # The "Input Network" group
    #----------------------------------------
    tixCombobox $w.innet -label "Input Network:" -labelside left \
	-dropdown false -editable false \
	-browsecmd "nxSelectNet $w $w.innet_name" \
	-command   "nxSelectNet $w $w.innet_name" \
	-options {
	    slistbox.scrollbar y
	    entry.width  15
	    label.anchor ne
	    label.padY   4
	    label.width  18
	}
    $w.innet subwidget listbox config -height 4
    pack $w.innet -side top -pady 2 -padx 4 -anchor w -fill x

    #----------------------------------------
    # The "Output Network" group
    #----------------------------------------
    tixCombobox $w.outnet -label "Output Network: " -labelside left \
	-dropdown false -editable false \
	-browsecmd "nxSelectNet $w $w.outnet_name" \
	-command   "nxSelectNet $w $w.outnet_name" \
	-options {
	    slistbox.scrollbar y
	    entry.width  15
	    label.anchor ne
	    label.padY   4
	    label.width  18
	}
    $w.outnet subwidget listbox config -height 4
    pack $w.outnet -side top -pady 2 -padx 4 -anchor w -fill x
    
    #----------------------------------------
    # The various spin-button inputs
    #----------------------------------------
    frame $w.sep1 -height 2 -bd 1 -relief sunken
    pack $w.sep1 -fill x -side top -padx 2 -pady 7

    nxMkControl $w.fa $w.rate
    $w.rate config -label "Learning Rate: " \
	    -variable $w.rate_value

    nxMkControl $w.fb $w.approx
    $w.approx config -label "2nd Approximation: " \
	    -variable $w.approx_value

    nxMkControl $w.fc $w.approx2
    $w.approx2 config -label "2nd Approximation Ratio: " \
	    -variable $w.approx2_value

    nxMkControl $w.fd $w.decay
    $w.decay config -label "Decay: " \
	    -variable $w.decay_value

    pack $w.fa $w.fb $w.fc $w.fd \
	-side top -pady 3 -padx 1 -anchor w 

    #----------------------------------------
    # The "Error File" radio buttons
    #----------------------------------------
    frame $w.sep2 -height 2 -bd 1 -relief sunken
    pack $w.sep2 -fill x -side top -padx 2 -pady 7

    frame $w.f4
    label $w.error -text "Save Error: "
    radiobutton $w.f4.off -text "OFF" -variable $w.saveerr -value off
    radiobutton $w.f4.on  -text "ON"  -variable $w.saveerr -value on
    pack $w.error  $w.f4.off $w.f4.on -pady 0 -in $w.f4 -side left
    pack $w.f4 -side top -padx 4 -anchor w

    # This "trace" will config the behavior of the file entry properly
    #
    global $w.saveerr
    trace variable $w.saveerr w "nxInver:SaveErrTrace $w"

    #----------------------------------------
    # The Error File entry
    #----------------------------------------
    nxMkFileSelect $w.f5 -entry $w.errfile \
	    -label $w.f5.lab -text "Filename:"\
	    -command "nxInver:GetErrFile $w"
    $w.errfile config -textvariable $w.filename_err
    pack $w.f5 -fill x -padx 4 -pady 2 -anchor w

    #----------------------------------------
    # The separator
    #----------------------------------------
    frame $w.sep3 -height 2 -bd 1 -relief sunken
    pack  $w.sep3 -fill x -padx 3 -pady 6

    #----------------------------------------
    # Buttons
    #----------------------------------------
    frame $w.f6
    button $w.alloc -text "Allocate Parameters" \
	-width 16 -command "nxInver:AllocParam $w"
    button $w.cancel -text "Close" \
	-width 16 -command "nxInver:Cancel $w"

    pack $w.alloc $w.cancel -in $w.f6 -fill y -padx 4 -side left \
	    -expand yes
    pack $w.f6 -fill x -padx 4 -pady 6

    #----------------------------------------
    # Dialog Creation FINISHED!
    #----------------------------------------
    # Initialize the network lists
    #
    nxLoadNetworkNames [$w.innet  subwidget slistbox]
    nxLoadNetworkNames [$w.outnet subwidget slistbox]

    # Initialize the saveerr file entry
    #
    nxInver:SaveErrTrace $w "" "" w
}

proc nxInver:AllocParam {w} {
    nxCallCommand TkNx_InvAllocParam
}

proc nxInver:Cancel {w} {
    wm withdraw $w
}

#----------------------------------------------------------------------
# Getting Output filename
#----------------------------------------------------------------------

proc nxInver:GetOutFile {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select Inversion Output File"
    $open config -command "nxInver:GetOutFile:CB $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.act"
    wm deiconify $open
    $filebox filter
}

# This func gets called when user select a file from "Output file"
proc nxInver:GetOutFile:CB {w filename} {
    upvar #0 $w.outfile outfilename

    set outfilename $filename
    nxEntryAlignEnd $w.outfile
}

#----------------------------------------------------------------------
#
#		The "Error File" handlers
#
#----------------------------------------------------------------------
proc nxInver:SaveErrTrace {w name1 name2 op} {
    global $w.saveerr

    set label  $w.f5.lab
    set entry  $w.errfile
    set button $w.f5:btn

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

# Save Err file
#
proc nxInver:GetErrFile {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select Inversion Error File"
    $open config -command "nxInver:GetErrFile:CB $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.error"
    wm deiconify $open
    $filebox filter
}

proc nxInver:GetErrFile:CB {w filename} {
    upvar #0 $w.filename_err errfilename

    set errfilename $filename
    nxEntryAlignEnd $w.errfile
}
