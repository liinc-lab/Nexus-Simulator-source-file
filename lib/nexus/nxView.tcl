# nxView.tcl
#
# 	Inplementing the "Simulation View" dialog
#

#----------------------------------------------------------------------
# The Notebook stuff
#----------------------------------------------------------------------

proc nxElectrodeNoteBook {page} {
    set w .elec_nb
    set nb $w.nb

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
    } else {
	toplevel $w

	option add *TixNoteBook.tagPadX 10
	option add *TixNoteBook.tagPadY 4
	option add *TixNoteBook.borderWidth 2
	option add *TixNoteBook.font\
	    -*-helvetica-bold-o-normal-*-14-*-*-*-*-*-*-*

	# This notebook has three pages
	#
	tixNoteBook $nb -ipadx 5 -ipady 2
	$nb add conn -createcmd "nxMakeConnPage $nb conn" -label Connectivity\
	    -raisecmd "nxENB:Select $w conn Connectivity"
	$nb add act  -createcmd "nxMakeActPage  $nb act"  -label Activity\
	    -raisecmd "nxENB:Select $w act Activity"
	$nb add view -createcmd "nxMakeViewPage $nb view" -label View\
	    -raisecmd "nxENB:Select $w view View"

	# The close button
	#
	frame $w.f2 -bd 2 -relief raised
	button $w.f2.close -text Close -command "nxENB:Close $w"

	pack $w.f2.close -expand yes -padx 2 -pady 2
	pack $w.f2 -side bottom -fill x

	pack $nb -side top -fill both -expand yes
    }
    $nb raise $page
}

proc nxENB:Select {w page name} {
    wm title $w "Electrode -- $name"

    case $page in {
	act {
	    uplevel #0 set activity.electrode on
	    uplevel #0 set main_elec Activity
	}
	conn {
	    uplevel #0 set connect.electrode on
	    uplevel #0 set main_elec Connectivity
	}
	view {
	    uplevel #0 set view.electrode on
	    uplevel #0 set main_elec View
	}
    }
}

proc nxENB:Close {w} {
    wm withdraw $w
}

#----------------------------------------------------------------------
# The View stuff
#----------------------------------------------------------------------

proc nxMainView {} {
    set w .view

    nxElectrodeNoteBook view
    }

proc nxMakeViewPage {nb page} {
    set w [$nb subwidget $page]

    option add *view*TixControl*label.width  18
    option add *view*TixControl*label.anchor e
    option add *view*TixControl*entry.width  12
 
    #----------------------------------------
    # The "Selected Network" group
    #----------------------------------------
    frame $w.f1
    tixCombobox $w.net -label "Network:" -labelside top \
	-dropdown false -editable false \
	-command "nxView:SelectNet $w" \
	-browsecmd "nxView:SelectNet $w" \
	-options {
	    label.anchor w
    }
    $w.net subwidget listbox config -height 4
    pack $w.net -in $w.f1 -side left -pady 1
    pack $w.f1 -padx 4 -pady 4 -anchor w
 
    #----------------------------------------
    # The buttons
    #----------------------------------------
    
    frame $w.f2
    button $w.netdim     -width 13 -text "Change Net Size" \
	    -command "nxView:ChangeDimension $w N"
    button $w.textwidth  -width 10 -text "Text Width" \
	    -command "nxView:ChangeDimension $w W"
    button $w.textheight -width 10 -text "Text Height" \
	    -command "nxView:ChangeDimension $w H"

#    pack  $w.netdim -in $w.f2 -side left \
#	    -expand yes -fill none -padx 4 -pady 4

    pack  $w.netdim $w.textwidth $w.textheight -in $w.f2 -side left \
	    -expand yes -fill none -padx 4 -pady 4

    pack $w.f2 -padx 4 -pady 4 -anchor c -fill x

    #----------------------------------------
    # Scaling factor
    #----------------------------------------
    nxMkControl $w.f8 $w.scale
    $w.scale config -label "Scaling factor:  " \
	-step 0.01 \
	-variable view.scale

    pack $w.f8  -padx 4 -pady 4 -anchor c

    frame $w.sep0 -height 2 -bd 1 -relief sunken
    pack $w.sep0 -fill x -padx 2

    #----------------------------------------
    # Direction buttons
    #----------------------------------------
    frame $w.f3
    frame $w.f4
    frame $w.f5
    button $w.up      -bitmap [tix getbitmap pan_up] \
			-command "nxMoveNets y p"
    button $w.down    -bitmap [tix getbitmap pan_down] \
			-command "nxMoveNets y n"
    button $w.left    -bitmap [tix getbitmap pan_left] \
			-command "nxMoveNets x n"
    button $w.right   -bitmap [tix getbitmap pan_right] \
			-command "nxMoveNets x p"
    button $w.zoomin  -bitmap [tix getbitmap zoom_in] \
			-command "nxMoveNets z p"
    button $w.zoomout -bitmap [tix getbitmap zoom_out] \
			-command "nxMoveNets z n"

    pack $w.up -in $w.f3 -side top -expand yes
    pack $w.left $w.zoomin $w.zoomout $w.right -in $w.f4 -side left -expand yes
    pack $w.down -in $w.f5 -side top -expand yes

    pack $w.f3 $w.f4 $w.f5 -fill x -padx 4 -pady 4

    #----------------------------------------
    # Movement amount
    #----------------------------------------
    nxMkControl $w.f9 $w.move
    $w.move config -label "Movement amount:  " \
	-variable view.movement -llimit 0
    pack $w.f9  -padx 4 -pady 4 -anchor c

    #----------------------------------------
    # Dialog Creation FINISHED!
    #----------------------------------------
    # Initialize the network list
    #
    nxLoadNetworkNames [$w.net subwidget slistbox]
  
}
proc nxView:ChangeDimension {w which} {
    nxCallCommand TkNx_ChangeDimension $which
}

proc nxMoveNets {axis direction} {
    TkNx_MoveNets $axis $direction    
}

proc nxView:SelectNet {w net} {
    global view.network

    set view.network $net
}
