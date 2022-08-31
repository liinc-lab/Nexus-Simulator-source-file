# nexus.tcl
#
# Author Ioi Kim Lam, OCT 1994.
#
# This file creates and maintains the man window of the Nexus program.
#

# nxCreateMainWindow
#
# Create the main window. This is the window that exerts the highest
# level control on Nexus. It contains the main wenu and main buttons
#
# This window exists for the whole lifetime of the Nexus program. When
# the user deletes this window, the Nexus program will terminate.
#
proc nxCreateMainWindow {title} {
    set w .main

    # Create the toplevel window and set its title
    #
    toplevel $w
    wm title $w $title
    wm minsize $w 10 10

    option add *[string range $w 1 end]*Button.anchor w
    option add *[string range $w 1 end]*Button.padX 10
    option add *[string range $w 1 end]*Menubutton.padX 10

    # (1) The Simulation Control
    #
    label  $w.lab1 -text "Simulation Control"
    entry $w.file -width 15 -bd 2 -relief sunken \
	-bg [lindex [$w.lab1 config -background] 4] -state disabled \
	-textvariable $w.filename
    button $w.loadsim  -text "Load Simulation ..."  -command nxMainLoadSim
    button $w.buildsim -text "Build Simulation ..." -command nxMainBuildSim
    button $w.savesim  -text "Save Simulation ..."  -command nxMainSaveSim
    button $w.sim      -text "Simulate ..."         -command nxMainSim
    button $w.quit     -text "Quit"  	 	    -command nxMainQuit

    pack $w.lab1 -side top -padx 5 -pady 3 -fill x
    pack $w.file -side top -padx 5 -pady 1 -fill x
    pack $w.loadsim $w.buildsim $w.savesim $w.sim $w.quit \
	-side top -padx 5  -fill x

    # (2) The Electrode  Control
    #
    label $w.lab2 -text "Electrode Control"
    set menu [tk_optionMenu $w.elec main_elec \
	"Connectivity" \
	"Activity" \
	"View"]

    $w.elec config -width 14 -padx 8 -font [$w.savesim cget -font] -anchor w
    $menu config -font [$w.savesim cget -font]
    pack $w.lab2 -side top -padx 5 -pady 3 -fill x
    pack $w.elec -side top -padx 5 -fill x

    uplevel #0 [list set main_elec "Activity"]
    uplevel #0 trace variable main_elec w nxMain:ElecTraceProc

    # (3) The Modification Control
    #
    label  $w.lab3 -text "Modification Control"
    button $w.ranact  -text "Randomize Activity ..." -command nxMainRanAct
    button $w.editcon -text "Edit Connections ..."   -command nxMainEditCon
    button $w.loadmap -text "Load Map Activity ..."  -command nxMainLoadMap
    button $w.setpara -text "Set Parameters ..."     -command nxMainSetParam

    pack $w.lab3 -side top -padx 5 -pady 3 -fill x
    pack $w.ranact $w.editcon $w.loadmap $w.setpara \
        -side top -padx 5  -fill x

    # (3) Learning Menus
    #
    label $w.labx -text "Learning Control"
    set menu [tk_optionMenu $w.learn main_learn  \
	"Back Propagation" \
	"Hebb Rule" \
	"RBF Learning" \
	"Network Inversion"]

    $w.learn config -width 14 -padx 8 -font [$w.loadmap cget -font] -anchor w
    $menu config -font [$w.loadmap cget -font]
    pack $w.labx -side top -padx 5 -pady 3 -fill x
    pack $w.learn -side top -padx 5 -fill x

    uplevel #0 [list set main_learn "Back Propagation"]
    uplevel #0 trace variable main_learn w nxMain:LearnTraceProc

    # (4) The Utilities Control
    #
    label  $w.lab4 -text "Utilities Control"
#   button $w.print   -text "Print Display ..."   -command nxMainPrintDis
    button $w.memory  -text "Memory"    	  -command nxMainMemory
    button $w.script  -text "Script"    	  -command nxMainScript
    button $w.about   -text "About"    	          -command nxMainAbout

    pack $w.lab4 -side top -padx 5 -pady 3 -fill x
    pack $w.memory $w.script $w.about -side top -padx 5  \
	    -fill x

    after 10 {nxMain:SetFilename "no file"}

    return $w
}

proc nxMainQuit {} {
    nxYesNoPrompt -yescmd TkNx_Cleanup -message "Are you sure you want to quit"
}

proc nxMainCellAct {} {

}

proc nxMain:LearnTraceProc {args} {
    global main_learn

    case [lindex $main_learn 0] in {
	"Back" {
	    nxMain:Learn:Back
	}
	"Hebb"  {
	    nxMain:Learn:Hebb
	}
	"RBF"  {
	    nxMain:Learn:RBF
	}
	"Network" {
	    nxMain:Learn:NetInv
	}
    }
}

proc nxMain:ElecTraceProc {args} {
    global main_elec
 
    case $main_elec in {
	"Connectivity" {
	    nxMainConnect
	}
	"Activity" {
	    nxMainActivity
	}
	"View" {
	    nxMainView
	}
    }
}

proc nxMainPrintDis {} {

}

proc nxMainMemory {} {
    TkNx_Memory
}

proc nxMain:SetFilename {filename} {
    set w .main
    global $w.filename

    set $w.filename $filename
    nxEntryAlignEnd $w.file
}
