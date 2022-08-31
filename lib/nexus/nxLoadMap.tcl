#  nxLoadMap.tcl
#
#	The "Load Map Activity" dialog box.
#

proc nxMainLoadMap {} {
    set w .loadmap

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow  $w
	return
    }

    nxOpenDialog $w "Load Map Activity"

    option add *loadmap*Radiobutton*width 3

    #----------------------------------------
    # The buttons on the right side
    #----------------------------------------
    frame $w.f0 -bd 3 -relief flat
    pack $w.f0 -side bottom -fill both
    frame $w.sep0 -height 2 -bd 1 -relief sunken
    pack $w.sep0 -side bottom -fill x

    #----------------------------------------
    # The radio buttons
    #----------------------------------------
    frame $w.f1
    frame $w.sep1 -bd 1 -height 2 -relief sunken
    frame $w.f1a
    frame $w.f1b
    pack $w.f1a $w.f1b -side left -in $w.f1 -anchor w -padx 10
    pack $w.f1 -side top -anchor w
    pack $w.sep1 -side top -fill x -padx 2

    label $w.load -text "Cycle Load"
    radiobutton $w.loadon    -text "ON"  \
	    -variable $w.load -value on
    radiobutton $w.loadoff   -text "OFF" \
	    -variable $w.load -value off
    pack $w.load  $w.loadoff $w.loadon -in $w.f1a -side top -pady 0 -anchor w

    label $w.order -text "Randomize Order"
    radiobutton $w.orderon    -text "ON"  \
	    -variable $w.order -value on
    radiobutton $w.orderoff   -text "OFF" \
	    -variable $w.order -value off
    pack $w.order $w.orderoff $w.orderon -in $w.f1b -side top -pady 0 -anchor w

    #----------------------------------------
    # The "Selected Network" group
    #----------------------------------------
    tixCombobox $w.net -label "Network:" -labelside left \
 	    -dropdown false -editable false \
	    -browsecmd "nxLoadMap:SelectNet $w" \
	    -command "nxLoadMap:SelectNet $w" \
	    -options {
	slistbox.scrollbar y
	entry.width  15
	label.padY   4
	label.anchor ne
	label.width  15
    }
    $w.net subwidget listbox config -height 4
    pack $w.net -side top -pady 3 -padx 4 -anchor w -fill x

    #----------------------------------------
    # The "Batch Filename:" group
    #----------------------------------------
    nxMkFileSelect $w.f2 -entry $w.filename_b \
	    -label $w.f2.lab -text "Batch Filename: "\
	    -command "nxLoadMap:GetFile_B $w"
    $w.filename_b config -textvariable $w.filename_b
    $w.f2.lab config -width 15 -anchor e
    bind $w.filename_b <Return> "nxLoadMap:LoadFile_B $w"

    pack $w.f2 -fill x -padx 4 -pady 2 -anchor w

    #----------------------------------------
    # The "Activity File" group
    #----------------------------------------
    nxMkFileSelect $w.f3 -entry $w.filename_a \
	    -label $w.f3.lab -text "Activity File: "\
	    -command "nxLoadMap:GetFile_A $w"
    $w.filename_a config -textvariable $w.filename_a
    $w.f3.lab config -width 15 -anchor e

    #  Changed to Select file (from Load file).  DW 95.03.16
    #  bind $w.filename_a <Return> "nxLoadMap:LoadFile_A $w"


    pack $w.f3 -fill x -padx 4 -pady 2 -anchor w

    #----------------------------------------
    # The list of activities
    #----------------------------------------
    frame $w.f4
    label $w.f4.lab -width 15
    tixScrolledListbox $w.actlist \
	-command "nxLoadMap:SelectActFile $w; nxLoadMap:LoadFile_A $w" \
	-browsecmd "nxLoadMap:SelectActFile $w" \
	-options {
	    slistbox.scrollbar y
	}
    $w.actlist subwidget listbox config -height 4
    pack $w.f4.lab  -in $w.f4 -side left
    pack $w.actlist -in $w.f4 -side left -expand yes -fill both
    pack $w.f4 -side top -pady 3 -padx 4 -anchor w -fill x

    #----------------------------------------
    # Create and pack the buttons
    #----------------------------------------
    button $w.loadfile_b -width 10 -text "Load Batch" \
	-command "nxLoadMap:LoadFile_B $w" -padx 4
    button $w.loadfile_a -width 10 -text "Load Activity" \
	-command "nxLoadMap:LoadFile_A $w" -padx 4
    button $w.cancel  -width 10 -text "Close" \
	-command "nxLoadMap:Cancel $w"     -padx 4
    pack $w.cancel $w.loadfile_a $w.loadfile_b -in $w.f0 \
	    -side right -expand yes -anchor c -padx 4 -pady 8

    #----------------------------------------
    # Dialog Creation FINISHED!
    #----------------------------------------
    # Initialize the network list
    #
    nxLoadNetworkNames [$w.net subwidget slistbox]
}

proc nxLoadMap:SelectActFile {w} {
    set file [$w.actlist subwidget listbox get active]
    
    if {$file != ""} {
	global $w.filename_a

	set $w.filename_a $file
    }
}


# Callback that handles the "Load Batch File" button
#
proc nxLoadMap:LoadFile_B {w} {
    global $w.filename_b

    if {[set $w.filename_b] == ""} {
	return
    }
    $w.actlist subwidget listbox delete 0 end

    set result [\
        TkNx_LoadBatchFile [set $w.filename_b] [$w.actlist subwidget listbox]]

    if {[lindex $result 0] == "error"} {
	nxErrorMsg [lindex $result 1]
    }
}

#  Callback that handles the "Load Single File" button
#  Note: C global variable activity_filename is linked with Tcl variable
#    .loadmap.filename_a (see nexus_tcl_var.c), so TkNx_LoadActivity does
#    not need a filename passed in.
#
#  DW 95.02.26  Use the value in the entry widget, and don't bother with
#    the list widget.  This way the user may set the activity file without
#    using a batch file.
#
proc nxLoadMap:LoadFile_A {w} {
    global $w.filename_a

    if {[set $w.filename_a] == ""} {
	return
    }

    nxCallCommand TkNx_LoadActivity

#    set file [$w.actlist subwidget listbox get active]
#    if {$file != ""} {
#	nxCallCommand TkNx_LoadActivity
#    }
}

# Callback that handles the "Cancel" button
#
proc nxLoadMap:Cancel {w} {
    wm withdraw $w
}

#----------------------------------------------------------------------
#
#			ACTIVITY FILES
#
#----------------------------------------------------------------------
#  Callback that handles the "Select" button inside the activity file entry
#
proc nxLoadMap:GetFile_A {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select Activity File"
    $open config -command "nxLoadMap:GetFile_A:Callback $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.act"
    wm deiconify $open
    $filebox filter
}

#  Callback for Return or OK button in File Select Dialog.
#
#  DW 95.02.26  Deselect ALL names in the list when choosing a file
#    manually.  This does not look for the filename in the list, because
#    the list might not be of the same directory, even though basenames might
#    just happen to match (i.e. subdir1/file1.act and subdir2/file1.act are
#    different, but the batch list might just have "file1.act"...)
#  DW 95.04.18  Deselection does not work properly (interaction with
#    tixFileSelectBox return code).  Commented out.
#
proc nxLoadMap:GetFile_A:Callback {w filename} {
    global $w.filename_a

#    $w.actlist subwidget listbox selection clear 0

    set $w.filename_a $filename
    nxEntryAlignEnd $w.filename_a
}

#----------------------------------------------------------------------
#
#			BATCH FILES
#
#----------------------------------------------------------------------
# Callback that handles the "Select" button inside the batch file entry
#
proc nxLoadMap:GetFile_B {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select Batch File"
    $open config -command "nxLoadMap:GetFile_B:Callback $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.in"
    wm deiconify $open
    $filebox filter
}


#  Callback for Return or OK button in File Select Dialog.
#
proc nxLoadMap:GetFile_B:Callback {w filename} {
    global $w.filename_b
    global $w.filename_a

    set $w.filename_b $filename
    nxEntryAlignEnd $w.filename_b

    set $w.filename_a ""
    $w.actlist subwidget listbox delete 0 end

    nxSetCAD $filename
}


proc nxLoadMap:SelectNet {w net} {
    global $w.network

    if {$net != ""} {
	set $w.network $net
    }
}
