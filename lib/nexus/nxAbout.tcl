proc nxMainAbout {} {
    set w .about

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    nxOpenDialog $w "About Nexus"

    nxAboutWindow $w.f2
    pack $w.f2 -padx 30

    frame $w.sep1 -relief sunken -height 2 -bd 1
    pack $w.sep1 -padx 2 -pady 2 -fill x

    frame $w.f9
    button $w.f9.close -text " Close " -command "wm withdraw $w"
    pack $w.f9.close -pady 10 -anchor c
    pack $w.f9 -fill both
}

proc nxAboutWindow {w} {
    frame $w

    label $w.bmp1 -bitmap [tix getbitmap nexus.xbm]
#   label $w.bmp2 -bitmap [tix getbitmap nexus3.xbm]
#   label $w.msg1 -text "NEXUS"
    label $w.msg2 -text "(c) 1990, 1995 University of Pennsylvania"
    label $w.msg3 -text "Created by: Paul Sajda and Leif Finkel"
    label $w.msg4 -text "Version 1.0"

    pack $w.bmp1 $w.msg2 $w.msg3 $w.msg4 -padx 10 -pady 6
}
