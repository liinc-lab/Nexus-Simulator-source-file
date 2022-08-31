#----------------------------------------------------------------------
# nxEditCon.tcl
#
#	The "Edit Connections" dialog box
#
#----------------------------------------------------------------------

proc nxMainEditCon {} {
    set w .editcon

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    toplevel $w
    wm minsize $w 10 10
    wm title $w "Edit Connections"

    #----------------------------------------
    # Available Networks:
    #----------------------------------------
    tixCombobox $w.net -label "Network:" -labelside left \
	-dropdown false -editable false \
	-browsecmd "nxEditCon:SelectNet $w" \
	-command   "nxEditCon:SelectNet $w" \
	-options {
	    label.width 11
	    label.anchor ne
	    label.padY 5
	    slistbox.scrollbar y
	}
    $w.net subwidget listbox config -height 4
    pack $w.net -padx 8 -pady 4 -anchor w -fill x

    #----------------------------------------
    # Connection:
    #----------------------------------------
    tixCombobox $w.conn -label "Connection:" -labelside left \
	-dropdown false -editable false\
	-browsecmd "nxEditCon:SelectCon $w" \
	-command   "nxEditCon:SelectCon $w" \
	-options {
	    label.width 11
	    label.anchor ne
	    label.padY 5
	    slistbox.scrollbar y
	}
    $w.conn subwidget listbox config -height 4
    pack $w.conn -padx 8 -pady 4 -anchor w -fill x

    #----------------------------------------
    # Connection Function
    #----------------------------------------
    frame $w.f2
    label $w.f2.lab -text "Function:" -width 11 -anchor e
    label $w.f2.hint -bitmap [tix getbitmap carriage_return]
    entry $w.function -width 20

    bind $w.function <Return> "nxEditCon:SetFunction $w %W; $w.ok invoke"

    pack $w.f2.lab -in $w.f2 -fill y -side left
    pack $w.f2.hint -in $w.f2 -fill y -side right -padx 4
    pack $w.function  -in $w.f2 -fill both \
	    -side left -expand yes


    pack $w.f2 -fill x -padx 8 -pady 4

    #----------------------------------------
    # The separator
    #----------------------------------------
    frame $w.sep1 -height 2 -bd 1 -relief sunken
    pack  $w.sep1 -fill x -padx 3 -pady 6

    #----------------------------------------
    # Buttons
    #----------------------------------------
    frame $w.f3
    button $w.ok     -text "Apply" -width 5\
	-command "nxEditCon:Change $w"
    button $w.cancel -text "Close" -width 5\
	-command "nxEditCon:Cancel $w"

    pack $w.ok $w.cancel -in $w.f3 -fill y -padx 4 -side left \
	    -expand yes
    pack $w.f3 -fill x -padx 4 -pady 6

    #----------------------------------------
    # Dialog Creation FINISHED!
    #----------------------------------------
    # Initialize the network list
    #
    nxLoadNetworkNames [$w.net subwidget slistbox]
}

proc nxEditCon:SelectNet {w net} {
    global $w.network
    global $w.idlist

    if {$net != ""} {
	set $w.network $net

	# load in the activities
	[$w.conn subwidget listbox] delete 0 end
	set $w.idlist [TkNx_GetConnections [$w.conn subwidget listbox]]
	#puts "ioi:: The list of ID's is \"[set $w.idlist]\""

	$w.conn subwidget listbox activate 0
    }
}

proc nxEditCon:SelectCon {w connection} {
    global $w.connection
    global $w.idlist

    #  Ioi:
    #
    #  $index is an integer: the position of the currently selected
    #  listbox index
    #
    set index [$w.conn subwidget listbox index active]
    set $w.connection [lindex [set $w.idlist] $index]

    #  puts "ioi:: The Network ID selected is \"[set $w.connection]\""
}

proc nxEditCon:SetFunction {w entry_widget} {
    global $w.function

    set $w.function [$entry_widget get]
}

proc nxEditCon:Change {w} {
    nxCallCommand TkNx_ChangeConnection
}

proc nxEditCon:Cancel {w} {
    wm withdraw $w
}
