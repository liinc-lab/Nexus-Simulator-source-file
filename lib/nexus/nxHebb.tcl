proc  nxMain:Learn:Hebb {} {
    set w .hebb

    if [winfo exists $w] {
	wm deiconify $w
	TkNx_RaiseWindow $w
	return
    }

    nxOpenDialog $w "Hebb Plasticity"

    frame $w.f1
    frame $w.f1a
    frame $w.sepx -width 2 -bd 1 -relief sunken
    frame $w.f1b
    frame $w.f1c
    pack $w.f1a -side right -anchor nw -fill both
    pack $w.sepx -side right -anchor nw -fill y
    pack $w.f1b $w.f1c -in $w.f1 -side top -anchor nw -fill both
    pack $w.f1 -side top -anchor nw -fill both
    #----------------------------------------
    # The Uppe Left Side: f1a
    #----------------------------------------
       #----------------------------------------
       # The Network: List
       #----------------------------------------
       tixCombobox $w.net -label "Network: " -labelside top \
	   -dropdown false -editable false\
	   -browsecmd "nxHebb:SelectNet $w" \
	   -command   "nxHebb:SelectNet $w" \
	   -options {
	       slistbox.scrollbar y
	       entry.width  15
	       label.anchor nw
	       label.padY   4
	       label.width  15
	   }
       $w.net subwidget listbox config -height 10
       pack $w.net -in $w.f1a -side top -pady 3 -padx 4 -anchor w -fill x

       #----------------------------------------
       # The Connection List
       #----------------------------------------
       nxHebb:MkConList $w
    #----------------------------------------
    # The Uppe Right Side: f1b
    #----------------------------------------
    label $w.lab1 -text "Hebb Rule: " -width 20 -anchor e
    radiobutton $w.off -text OFF -variable $w.learning -width 10\
	-value off
    radiobutton $w.on  -text ON  -variable $w.learning \
	-value on

    label $w.lab2 -text "Using Post-synaptic :" -width  20 -anchor e
    radiobutton $w.rate    -text "Firing Rate" -variable $w.type -width 10 \
	-value rate
    radiobutton $w.voltage -text "Voltage"     -variable $w.type \
	-value voltage

    pack $w.lab1 -in $w.f1b -side left -pady 4 -padx 8 -anchor w
    pack $w.off $w.on -in $w.f1b -side left -padx 2 -anchor w

    pack $w.lab2 -in $w.f1c -side left -pady 4 -padx 8 -anchor w
    pack $w.rate $w.voltage -in $w.f1c -side left -padx 2 -anchor w

    #----------------------------------------
    # The Controls
    #----------------------------------------
    label $w.lab3 -text "Pre-Synaptic" -anchor w
    frame $w.cf1
    frame $w.cf2
    frame $w.cf3

    label $w.cf1.hint -bitmap [tix getbitmap carriage_return]
    label $w.cf2.hint -bitmap [tix getbitmap carriage_return]
    label $w.cf3.hint -bitmap [tix getbitmap carriage_return]

    tixControl $w.thresh -label "Threshold:  " \
	-llimit 0 -ulimit 100\
	-variable $w.thresh_value

    label $w.lab4 -text "Post-Synatic threshold if" -anchor w

    tixControl $w.pre_plus  -label "(Pre +): " \
	-llimit 0 -ulimit 100\
	-variable $w.pre_plus_value

    tixControl $w.pre_minus -label "(Pre -): " \
	-llimit 0 -ulimit 100\
	-variable $w.pre_minus_value

    pack $w.lab3 -side top -anchor w -padx 4 -pady 4
    pack $w.cf1  -side top -anchor w -padx 4
    pack $w.lab4 -side top -anchor w -padx 4 -pady 4
    pack $w.cf2 $w.cf3 -side top -anchor w -padx 4 -pady 1

    pack $w.thresh    $w.cf1.hint -in $w.cf1 -side left -anchor w -padx 4
    pack $w.pre_plus  $w.cf2.hint -in $w.cf2 -side left -anchor w -padx 4
    pack $w.pre_minus $w.cf3.hint -in $w.cf3 -side left -anchor w -padx 4

    #----------------------------------------
    # The Radiobuttons
    #----------------------------------------
    frame $w.f2
    label $w.lab5 -text "Scaling Factor:  "
    radiobutton $w.1 -text "0.01"    -variable $w.scale \
	-value 1
    radiobutton $w.2 -text "0.001"   -variable $w.scale \
	-value 2
    radiobutton $w.3 -text "0.0001"  -variable $w.scale \
	-value 3
    radiobutton $w.4 -text "0.00001" -variable $w.scale \
	-value 4
    pack $w.lab5  -in $w.f2 -padx 4 -side left
    pack $w.1 $w.2 $w.3 $w.4 -in $w.f2 -side left -padx 0
    pack $w.f2 -side top -anchor w -pady 0

    nxMkScaleSelect $w.f3
    nxMkScaleSelect $w.f4
    nxMkScaleSelect $w.f5
    nxMkScaleSelect $w.f6

    pack $w.f3 $w.f4 $w.f5 $w.f6 -side top -padx 8 -pady 3 -anchor w

    $w.f3.label config -text "Pre + Post + " -width 20 -anchor e
    $w.f4.label config -text "Pre + Post - " -width 20 -anchor e
    $w.f5.label config -text "Pre - Post + " -width 20 -anchor e
    $w.f6.label config -text "Pre - Post - " -width 20 -anchor e

    $w.f3.scale config -command "nxHebb:ValueChange $w f3 plus_plus"
    $w.f4.scale config -command "nxHebb:ValueChange $w f4 plus_minus"
    $w.f5.scale config -command "nxHebb:ValueChange $w f5 minus_plus"
    $w.f6.scale config -command "nxHebb:ValueChange $w f6 minus_minus"

    # Initialize the scale trace variables
    #
    global $w.plus_plus
    global $w.plus_minus
    global $w.minus_plus
    global $w.minus_minus

    trace variable $w.plus_plus   w "nxHebb:ScaleTrace $w f3"
    trace variable $w.plus_minus  w "nxHebb:ScaleTrace $w f4"
    trace variable $w.minus_plus  w "nxHebb:ScaleTrace $w f5"
    trace variable $w.minus_minus w "nxHebb:ScaleTrace $w f6"

    # Initialize the scales
    #
    nxHebb:ScaleTrace $w f3 $w.plus_plus   "" w
    nxHebb:ScaleTrace $w f4 $w.plus_minus  "" w
    nxHebb:ScaleTrace $w f5 $w.minus_plus  "" w
    nxHebb:ScaleTrace $w f6 $w.minus_minus "" w

    #----------------------------------------
    # The Cancel Button
    #----------------------------------------
    frame $w.sep1 -height 2 -relief sunken -bd 1
    pack $w.sep1 -side top -fill both

    button $w.cancel -text "Close" -width 8 -command "wm withdraw $w"
    pack $w.cancel -side top -anchor c -pady 10

    #----------------------------------------
    # Dialog Creation FINISHED!
    #----------------------------------------
    # Initialize the network lists
    #
    nxLoadNetworkNames [$w.net subwidget slistbox]

}

set hebb(list1) {}
set hebb(list2) {}
set hebb(sb)    {}

proc nxHebb:MkConList {w} {
    frame $w.f1a.conlist
    label $w.f1a.conlabel -text "Connection: " -anchor nw -pady 4 -width 15
    pack $w.f1a.conlabel -side top -anchor nw
    pack $w.f1a.conlist -side top -expand yes -fill both

    frame $w.f1a.top
    frame $w.f1a.bot
    pack $w.f1a.top $w.f1a.bot -in $w.f1a.conlist\
	-side top -padx 4 -pady 3 -fill x

    # The entry part
    #
    entry $w.f1a.top.entry
    frame $w.f1a.top.frame -width 19
    pack $w.f1a.top.frame -side right
#   pack $w.f1a.top.entry -side left -expand yes -fill x

    # The two listboxes
    #
    frame $w.f1a.bot.f1 -relief sunken -bd 2
    scrollbar $w.f1a.bot.sb -orient vertical -command "nxHebb:SbView"

    pack $w.f1a.bot.sb -side right -fill y
    pack $w.f1a.bot.f1 -side left -expand yes -fill both

    listbox $w.f1a.bot.list1 -width 4 -height 10  -bd 0 -exportselection false \
	-yscrollcommand nxHebb:ListView -fg #800000 \
	-highlightthickness 0
    listbox $w.f1a.bot.list2 -width 15 -height 10 -bd 0 -exportselection false \
	-yscrollcommand nxHebb:ListView \
	-highlightthickness 0

    frame $w.f1a.bot.sep -width 1 -bg black
    pack $w.f1a.bot.list1 $w.f1a.bot.sep -in $w.f1a.bot.f1 -side left -fill y
    pack $w.f1a.bot.list2 -in $w.f1a.bot.f1 -side left -fill both -expand yes

    # Set the bindings to work right for the two listboxes
    #
    #
    foreach widget "$w.f1a.bot.list1 $w.f1a.bot.list2" {
	bind $widget <B1-Motion> {;}
    }
    bind $w.f1a.bot.list1 <1> {+nxHebb:ListSelect %y}
    bind $w.f1a.bot.list2 <1> {+nxHebb:ListSelect %y}

    # We use these global variables later
    #
    #
    global hebb
    set hebb(list1) $w.f1a.bot.list1
    set hebb(list2) $w.f1a.bot.list2
    set hebb(sb)    $w.f1a.bot.sb
}

proc nxHebb:SbView {y} {
    global hebb
    
    $hebb(list1) yview $y
    $hebb(list2) yview $y
}

proc nxHebb:ListView {first last} {
    global hebb
    
    $hebb(list1) yview $first
    $hebb(list2) yview $first
    $hebb(sb) set $first $last
}

proc nxHebb:ListSelect {y} {
    global hebb

    set index [$hebb(list1) nearest $y]

    if {$index == -1} {
	return
    }

    if {[$hebb(list1) get $index] == "ON"} {
	set new_value 0
    } else {
	set new_value 1
    }

    $hebb(list1) delete $index

    if {$new_value == 1} {
	$hebb(list1) insert $index "ON"
    } else {
	$hebb(list1) insert $index "  "
    }

    $hebb(list1) selection set $index
    $hebb(list2) selection set $index

    TkNx_SetHebbConnection $hebb(net) [$hebb(list2) get $index] $new_value
}


proc nxHebb:SelectNet {w net} {
    global hebb

    if {$net == ""} {
	return
    }

    $hebb(list1) delete 0 end
    $hebb(list2) delete 0 end

    TkNx_GetHebbConnections $net $hebb(list2) $hebb(list1)

    set hebb(net) $net
}

# Monitoring the values of the scales:
#
# Scales don't have -variable option. So we have to use a more cumbersome
# function calling interface
#
#
proc nxHebb:ValueChange {w frame which value} {
    global $w.$which

    $w.$frame.scale set $value

    set $w.$which $value
}

proc nxHebb:ScaleTrace {w frame name1 name2 op} {
    global $name1


    $w.$frame.scale set [set $name1]
}
