#----------------------------------------------------------------------
#
#			Randomize Activity
#
#----------------------------------------------------------------------
proc nxMainRanAct {} {
    set w .randomize

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    toplevel $w
    wm minsize $w 10 10
    wm title $w "Randomize Cell Activity"

    option add *randomize*entry.width 12

    #----------------------------------------
    # Min firing rate
    #----------------------------------------
    frame $w.f1
    label $w.f1.lab -text " Min Firing Rate" -width 14
    tixControl $w.minfire -variable $w:minfire
    label $w.f1.hint -bitmap [tix getbitmap carriage_return]
    pack $w.f1.lab -in $w.f1 -fill y -padx 4 -side left
    pack $w.minfire  -in $w.f1 -fill both -padx 4 \
	    -side left -expand yes
    pack $w.f1.hint -in $w.f1 -fill y -padx 4 -side left

    pack $w.f1 -fill x -padx 4 -pady 4

    #----------------------------------------
    # Max firing rate
    #----------------------------------------

    frame $w.f2
    label $w.f2.lab -text " Max Firing Rate" -width 14
    tixControl $w.maxfire -variable $w:maxfire
    label $w.f2.hint -bitmap [tix getbitmap carriage_return]
    pack $w.f2.lab -in $w.f2 -fill y -padx 4 -side left
    pack $w.maxfire  -in $w.f2 -fill both -padx 4 \
	    -side left -expand yes
    pack $w.f2.hint -in $w.f2 -fill y -padx 4 -side left

    pack $w.f2 -fill x -padx 4 -pady 4

    #----------------------------------------
    # The separator
    #----------------------------------------
    frame $w.sep1 -height 2 -bd 1 -relief sunken
    pack  $w.sep1 -fill x -padx 3 -pady 6

    #----------------------------------------
    # Buttons
    #----------------------------------------
    frame $w.f3
    button $w.loadcell -text "Load Cells" -width 10 \
	    -command "nxMainRanAct:Load $w"
    button $w.cancel   -text "Close"     -width 10 \
	    -command "nxMainRanAct:Cancel $w"
    pack $w.loadcell $w.cancel -in $w.f3 -fill y -padx 4 -side left \
	    -expand yes
    pack $w.f3 -fill x -padx 4 -pady 6
}

proc nxMainRanAct:Load {w} {
    TkNx_LoadCells
}

proc nxMainRanAct:Cancel {w} {
    wm withdraw $w
}
