# nxDialog.tcl
#
# This file has various small dialogs
#
#

# nxOpenFileDialog -
#
#	A utility function for creating and reusing a file selection dialog
# box.
#

proc nxOpenFileDialog {{parent {}}} {
    if {![winfo exists .open]} {
	tixFileSelectDialog .open
	.open subwidget btns subwidget cancel config -text Close
    } else {
	wm deiconify .open
	TkNx_RaiseWindow .open
    }

#    if {$parent != {}} {
#	wm transient .open $parent
#    } else {
#	wm transient .open .main
#    }

    return .open
}

# Set the Current Access Directory to the directory thar contains $filename
#
proc nxSetCAD {filename} {
#    puts "Current Access Directory = [file dir $filename]"
    TkNx_SetCAD [file dir $filename]
}

# nxOpenDialog -- 
#
#
# Open a dialog with a title, also set some standard behavior
#
proc nxOpenDialog {w title} {
    toplevel $w
    wm minsize $w 10 10
    wm title $w $title
    wm protocol $w WM_DELETE_WINDOW "wm withdraw $w"
}

proc nxCloseDialog {w} {
    wm withdraw $w
}

#----------------------------------------------------------------------
#
#			Load Simulation Dialog
#
#----------------------------------------------------------------------

proc nxMainLoadSim {} {
    set w [nxOpenFileDialog]
    wm title $w "Load Simulation"
    $w config -command nxMainLoadSimCallback

    set filebox [$w subwidget fsbox]
    $filebox config -pattern "*.save"
    wm deiconify $w
    $filebox filter
}

proc nxMainLoadSimCallback {filename} {
    global nexus

    if {![file exists $filename]} {
	nxErrorPrompt "Cannot open file \"$filename\""
    } elseif {[file extension $filename] != ".save"} {
        nxErrorPrompt "Wrong suffix for simulation files. Should be \".save\""
    } else {
	nxSetCAD $filename
	TkNx_ReadFile L $filename
	nxMain:SetFilename $filename
	nxRemoveGraphAboutWin
	nxGraphRedraw $nexus(glx)
    }
}

#----------------------------------------------------------------------
#
#			Build Simulation Dialog
#
#----------------------------------------------------------------------
proc nxMainBuildSim {} {
    set w [nxOpenFileDialog]
    wm title $w "Build Simulation"
    $w config -command nxMainBuildSimCallback

    set filebox [$w subwidget fsbox]
    $filebox config -pattern "*.nx"
    wm deiconify $w
    $filebox filter
}

proc nxMainBuildSimCallback {filename} {
    global nexus

    if {![file exists $filename]} {
	nxErrorPrompt "Cannot open file \"$filename\""
    } elseif {[file extension $filename] != ".nx"} {
	nxErrorPrompt "Wrong suffix for build files. Should be \".nx\""
    } else {
	nxSetCAD $filename
        TkNx_ReadFile B $filename
	nxMain:SetFilename $filename
	nxRemoveGraphAboutWin
	nxGraphRedraw $nexus(glx)
    }
}

proc nxErrorPrompt {string} {
    puts $string
}
