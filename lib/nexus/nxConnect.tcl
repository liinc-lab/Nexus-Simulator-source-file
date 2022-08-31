# nxConnect.tcl
#
# 	Inplementing the "Connectivity" dialog
#

proc nxMainConnect {} {
    nxElectrodeNoteBook conn
}

proc nxMakeConnPage {nb page} {
    set w [$nb subwidget $page]

    option add *conn*TixControl*entry.width  10

    #----------------------------------------
    # The "Show" group
    #----------------------------------------
    frame $w.f1

    label $w.show -text "Show:" -anchor w
    radiobutton $w.ante  -text "Anterograde" -variable connect.show \
	-value ante
    radiobutton $w.retro -text "Retrograde"  -variable connect.show \
	-value retro

    # Now we don't support "Anterograde". Let's disable it. It may be
    # enabled in future versions
    $w.ante config -state disabled

    pack $w.show $w.ante $w.retro -in $w.f1 -side left -fill y
    pack $w.f1 -fill x -padx 4 -pady 4

    #-----------------------------------------
    #  Separator
    #-----------------------------------------

    frame $w.sep2 -height 2 -bd 1 -relief sunken
    pack  $w.sep2 -fill x -padx 3 -pady 2

    #-----------------------------------------
    #  The "Display Depth" Group, DW 95.03.16
    #-----------------------------------------

    frame $w.f04
    label $w.colour -text "Connectivity Display Depth:"
    radiobutton $w.grey -text "GreyScale" -variable conduct.colour \
	    -value 0
    radiobutton $w.colr -text "Colour" -variable conduct.colour \
	    -value 1
    pack $w.colour $w.grey $w.colr -in $w.f04 -side left -padx 0 -pady 0
    pack $w.f04 -side top -pady 1 -padx 8 -anchor w

    #----------------------------------------
    # The "Connectivity Range Adjustment" groups
    #----------------------------------------
    label $w.conduct -text "Connectivity Range Adjustment"
    pack  $w.conduct -fill x -padx 4 -pady 4

      #----------------------------------------
      # The "Display min/max" group
      #----------------------------------------
      label $w.disp -text "  Display"
      pack $w.disp -fill x -padx 4 -pady 2

        #----------------------------------------
	# Min
	#----------------------------------------
	frame $w.f5
	label $w.f5.lab -text "    Min. Value" -width 14
	tixControl $w.dispmin -variable connect.dispmin -step 0.01
	label $w.f5.btn -bitmap [tix getbitmap carriage_return]
	pack $w.f5.lab -in $w.f5 -fill y -padx 4 -side left
	pack $w.dispmin  -in $w.f5 -fill both -padx 4 \
		-side left -expand yes
	pack $w.f5.btn -in $w.f5 -fill y -padx 4 -side left

        pack $w.f5 -anchor w -padx 4 -pady 0

        #----------------------------------------
	# Max
	#----------------------------------------
	frame $w.f6
	label $w.f6.lab -text "    Max. Value" -width 14
	tixControl $w.dispmax -variable connect.dispmax -step 0.01
        label $w.f6.btn  -bitmap [tix getbitmap carriage_return]
	pack $w.f6.lab -in $w.f6 -fill y -padx 4 -side left
	pack $w.dispmax  -in $w.f6 -fill both -padx 4 \
		-side left -expand yes
	pack $w.f6.btn -in $w.f6 -fill y -padx 4 -side left

        pack $w.f6 -anchor w -padx 4 -pady 0

      #----------------------------------------
      # The "Computation min/max" group
      #----------------------------------------
      label $w.comp -text "  Computation"
      pack $w.comp -anchor w -padx 4 -pady 2

        #----------------------------------------
	# Min
	#----------------------------------------
	frame $w.f7
	label $w.f7.lab -text "    Min. Value" -width 14
	tixControl $w.compmin  -variable connect.compmin -step 0.01
	label $w.f7.btn -bitmap [tix getbitmap carriage_return]
	pack $w.f7.lab -in $w.f7 -fill y -padx 4 -side left
	pack $w.compmin  -in $w.f7 -fill both -padx 4 \
		-side left -expand yes
	pack $w.f7.btn -in $w.f7 -fill y -padx 4 -side left

        pack $w.f7 -anchor w -padx 4 -pady 0

        #----------------------------------------
	# Max
	#----------------------------------------
	frame $w.f8
	label $w.f8.lab -text "    Max. Value" -width 14
	tixControl $w.compmax  -variable connect.compmax -step 0.01
	label $w.f8.btn -bitmap [tix getbitmap carriage_return]
	pack $w.f8.lab -in $w.f8 -fill y -padx 4 -side left
	pack $w.compmax  -in $w.f8 -fill both -padx 4 \
		-side left -expand yes
	pack $w.f8.btn -in $w.f8 -fill y -padx 4 -side left

        pack $w.f8 -anchor w -padx 4 -pady 0


    #----------------------------------------
    # The "Output Weight" groups
    #----------------------------------------
    frame $w.sep1 -height 2 -bd 1 -relief sunken
    pack  $w.sep1 -fill x -padx 3 -pady 2
    label $w.output -text "Output Weights"
    pack  $w.output -fill x -padx 4 -pady 1

      #----------------------------------------
      # The "To Screen" group
      #----------------------------------------
      frame $w.f3
      label $w.screen -text "    To Screen" -width 16
      radiobutton $w.f3.off -text "OFF" -variable connect.toscreen -width 5 \
	-value off 
      radiobutton $w.f3.on  -text "ON"  -variable connect.toscreen -width 5 \
	-value on

      pack $w.screen $w.f3.off $w.f3.on -in $w.f3 -side left -fill y \
	      -padx 4
      pack $w.f3 -fill x -padx 4 -pady 0

      #----------------------------------------
      # The "To File" group
      #----------------------------------------
      frame $w.f4
      label $w.file -text "    To File" -width 16
      radiobutton $w.f4.off -text "OFF" -variable connect.tofile -width 5 \
	-value off
      radiobutton $w.f4.on  -text "ON"  -variable connect.tofile -width 5 \
	-value on

      pack $w.file $w.f4.off $w.f4.on -in $w.f4  -padx 4\
	      -side left -fill y
      pack $w.f4 -fill x -padx 4 -pady 0


      #----------------------------------------
      # The "Select file" group
      #----------------------------------------

      nxMkFileSelect $w.f9 -entry $w.filename \
	  -label $w.f9.lab -text "Filename: "\
	  -command "nxConn:GetFile $w"
      $w.filename config -textvariable connect.filename
      $w.f9.lab config -width 11 -anchor e
      pack $w.f9 -fill x -padx 4 -pady 1 -anchor w
      bind $w.filename <Return> {;}

    # This "trace" will config the behavior of the file entry properly
    #
    global connect.tofile
    trace variable connect.tofile w "nxConn:SaveFileTrace $w"
    nxConn:SaveFileTrace $w connect.tofile "" [set connect.tofile]
}

proc nxConn:SaveFileTrace {w name1 name2 op} {
    global connect.tofile

    set label  $w.f9.lab
    set entry  $w.filename 
    set button $w.f9:btn

    if {[set connect.tofile] == "on"} {
	# Disable the file-entry
	#
	$label config -fg black
	$entry config -fg black -state normal
	$button config -fg black -state normal
    } else {
	# Enable the file-entry
	#
	$label config -fg gray40
	$entry config -fg gray40 -state disabled
	$button config -fg gray40 -state disabled
    }
    nxEntryAlignEnd $w.filename 
}

proc nxConn:GetFile {w} {
    set open [nxOpenFileDialog $w]
    wm title $open "Select Output Weights File"
    $open config -command "nxConn:GetFile:Callback $w"

    set filebox [$open subwidget fsbox]
    $filebox config -pattern "*.wt"
    wm deiconify $open
    $filebox filter
}

proc nxConn:GetFile:Callback {w filename} {
    global connect.filename

    set connect.filename $filename
    nxEntryAlignEnd $w.filename 
}

