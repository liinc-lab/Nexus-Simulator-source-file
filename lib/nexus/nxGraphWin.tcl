# nxGraphWin.tcl
#
# Author Ioi Kim Lam, OCT 1994.
#
# This file creates and maintains the graphics window in Nexus.
#

set direct_graphics 1
TkNx_SetBackingStore [expr 1 - $direct_graphics]


# nxCreateGraphWindow
#
# Create the graph window window. This window contains the nexus graph
#
# This window exists for the whole lifetime of the Nexus program. When
# the user deleted this window, the Nexus program will terminate.
#
proc nxCreateGraphWindow {w title} {
    # Create the toplevel window and set its title
    #
    toplevel $w -borderwidth 2
    wm title $w $title
    wm minsize $w 10 10

    # Create the OpenGL grahics window
    #  size defined in nexus_graphics.h (V_WIN_HEIGHT, V_WIN_WIDTH)
    tixScrolledWindow $w.scr -expandmode expand
    set f [$w.scr subwidget window]

    GLxwin $f.glx -width 750 -height 750
    frame $f.about
    nxAboutWindow $f.about.f
    pack $f.about.f -expand yes -anchor c

    pack $f.about -expand yes -fill both
    pack $w.scr -expand yes -fill both
    # We'll pack the glx widget later.

    # Record the names of the widgets
    uplevel #0 set nexus(about) $f.about
    uplevel #0 set nexus(glx)   $f.glx

    # Bind the behaviors
    #
    bind $f.glx <Expose>    "tixDoWhenIdle \
	    nxGraphExpose $f.glx %x %y %w %h %c"
    bind $f.glx <Configure> "tixDoWhenIdle \
	    nxGraphConfigure $f.glx"

    bind $f.glx <Any-ButtonPress>   "nxMousePress   %W %b %x %y"
    bind $f.glx <Any-ButtonRelease> "nxMouseRelease %W %b %x %y"
    bind $f.glx <Any-Motion>        "nxMouseMotion  %W %x %y"

    # Create the pixmap to do off screen rendering
    #

    global direct_graphics

    if {$direct_graphics} {
	$f.glx link
    } else {
	$f.glx pmcreate 750 750
	$f.glx pmlink
    }

    return $w
}


# Forces the window to redraw.
#
proc nxGraphRedraw {w} {
    global direct_graphics

    if {$direct_graphics} {
	$w link
	tixDoWhenIdle \
		TkNx_RedrawGraphWin $w [winfo width $w] [winfo height $w]
    } else {
	$w pmlink
	TkNx_RedrawGraphWin $w 800 800
    }
}

# Copy from the backing store.
#
proc nxGraphExpose {w xpx xpy xpwidth xpheight count} {
    global direct_graphics

    if {$direct_graphics} {
	if {$count == 0} {
	    $w link
	    tixDoWhenIdle \
		    TkNx_RedrawGraphWin $w [winfo width $w] [winfo height $w] 
	}
    } else {
	$w pmcopy $xpx $xpy $xpwidth $xpheight
    }
}

# Resize the window.
#
proc nxGraphConfigure {w} {
    global nexus
    global direct_graphics

    set width [winfo width $w]
    set height [winfo height $w]

    if [winfo exists $nexus(about)] {
	$nexus(about) config -width $width -height $height
    }

    if {$width <= 750 } {
        set width 750
    }
    if {$height <= 750} {
        set height 750
    }

    $nexus(glx) config -width $width -height $height
    $w config -width $width -height $height

    TkNx_ConfigureGraphWin $w $width $height

    if {$direct_graphics == 0} {
	$nexus(glx) pmcreate $width $height
	$nexus(glx) pmlink
	TkNx_RedrawGraphWin $w $width $height
    }

}

proc nxRemoveGraphAboutWin {} {
    global nexus

    if [winfo exists $nexus(about)] {
	destroy $nexus(about)
	pack $nexus(glx) -expand yes -fill both
    }
}
#----------------------------------------------------------------------
#			Handling Mouse Events
#----------------------------------------------------------------------

set nxButton(1) 0
set nxButton(2) 0
set nxButton(3) 0

proc nxMousePress {w b x y} {
    global nxButton

    set nxButton($b) 1
    nxSendMouseEvent B $x $y
}

proc nxMouseRelease {w b x y} {
    global nxButton

    set nxButton($b) 0
    nxSendMouseEvent R $x $y
}

proc nxMouseMotion {w x y} {
    global nxButton

    if {$nxButton(1) || $nxButton(2) || $nxButton(3)} {
	nxSendMouseEvent M $x $y
    }
}

# This sends mouse events to the Nexus "C" module.
#
#
proc nxSendMouseEvent {type x y} {
    global nxButton

    set buttons [expr $nxButton(1)+$nxButton(2)*2+$nxButton(3)*4]

    TkNx_MouseEvent $type $x $y $buttons
}

