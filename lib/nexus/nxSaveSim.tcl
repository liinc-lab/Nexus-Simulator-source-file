proc nxMainSaveSim {} {
    set w .savesim

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    nxOpenDialog $w "Save Simulation"

    #----------------------------------------
    # The "Output File" group
    #----------------------------------------
    nxMkFileSelect $w.f1 -entry $w.outfile \
	    -label $w.f1.lab -text "Filename:"\
	    -command "nxSSim:GetOutFile $w"
    $w.outfile config -textvariable $w.outfile
    pack $w.f1 -fill x -padx 8 -pady 6 -anchor w
    bind $w.outfile <Return> "$w.save invoke"

    #----------------------------------------
    # The "Save Format" group
    #----------------------------------------
    label $w.lab -text "Save Format:"
    radiobutton $w.ascii      -text "ASCII"      -width 10 -variable $w.format\
	-command "nxSSim:SetFormat $w" \
	-value ascii
    radiobutton $w.compressed -text "Compressed" -width 10 -variable $w.format\
	-command "nxSSim:SetFormat $w"\
	-value compressed 
    pack $w.lab $w.ascii $w.compressed -side top -padx 30 -anchor w

    # Set the default value
    #
    global $w.format; set $w.format "ascii"

    #----------------------------------------
    # The Buttons
    #----------------------------------------
    frame $w.sep1 -height 2 -bd 1 -relief sunken
    pack  $w.sep1 -fill x -padx 3 -pady 6
    
    frame $w.f2
    button $w.save   -text "Save Now" -width 10 -command "nxSSim:Save $w"
    button $w.cancel -text "Close"   -width 10 -command "wm withdraw $w"

    pack $w.save $w.cancel -in $w.f2 -fill y -padx 4 -side left \
	    -expand yes
    
    pack $w.f2 -fill x -padx 4 -pady 10
}

#----------------------------------------------------------------------
# Getting Output filename
#----------------------------------------------------------------------
proc nxSSim:GetOutFile {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select Simulation File"
    $open config -command "nxSSim:GetOutFile:CB $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.save"
    wm deiconify $open
    $filebox filter
}

# This func gets called when user select a file from "Output file"
proc nxSSim:GetOutFile:CB {w filename} {
    upvar #0 $w.outfile outfilename
    upvar #0 $w.format  format

    set outfilename $filename

    if {[file extension $filename] == ".Z"} {
	set format compressed
    } else {
	set format ascii
    }

    nxEntryAlignEnd $w.outfile
}

proc nxSSim:Save {w} {
    upvar #0 $w.outfile outfilename
    upvar #0 $w.format  format

    if {$outfilename == ""} {
	return
    }

    if {[file exists $outfilename] == 0} {
	# This file does not exist yet, don't worry
	#
	TkNx_SaveSimulation $outfilename $format
    } else {
	# Ask for confirmation %%
	#
	TkNx_SaveSimulation $outfilename $format
    }
}

# Change the filename entry according to the format type
#
proc nxSSim:SetFormat {w} {
    upvar #0 $w.outfile outfilename
    upvar #0 $w.format  format

    if {$outfilename == ""} {
	return
    }

    if {$format == "ascii" && [file extension $outfilename] == ".Z"} {
	set outfilename [file root $outfilename]
    }
    if {$format == "compressed" && [file extension $outfilename] != ".Z"} {
	set outfilename $outfilename.Z
    }
    nxEntryAlignEnd $w.outfile 
}
