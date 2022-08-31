# nexus.tcl
#
# Author Ioi Kim Lam, OCT 1994.
#
# This is the main TCL program for the TK based nexus package.
# It will auto-load in other TCL files of TK-nexus and popup the main windows.
#

# nxInit
#
# This is the first TCL procedure that gets called when TK-Nexus starts up.
#
proc nxInit {} {

    # The root window is troublesome. Don't even consider using it.
    #
    wm withdraw .

    # Initialize the bitmaps
    #
    global nexus_library
    tix addbitmapdir $nexus_library/bitmaps

    # Create the start-up windows
    #
    set mainGeom 166x630
    set graphGeom 800x800
	
    set w [nxCreateMainWindow "Nexus Version 1.0"]
    wm geometry $w $mainGeom 
    wm protocol $w WM_DELETE_WINDOW nxMainQuit
    update

    global env graphics_off

    if [info exists env(NEXUS_NOGRAPHICS)] {
	set graphics_off 1
    } else {
	set graphics_off 0

	set w [nxCreateGraphWindow .graph "Nexus Graphics Window"]
	wm geometry $w $graphGeom 
	wm protocol $w WM_DELETE_WINDOW nxMainQuit
	update
    }
}

option add *background lightgray userDefault
option add *TixComboBox*TixScrolledListBox.scrollbar y
option add *TixControl*entry.width 8
option add *Radiobutton*borderWidth 2
option add *Radiobutton*relief      flat


# Init the Nexus package.
#
nxInit

# debug stuff
#
proc s {file} {
    source $file
}
