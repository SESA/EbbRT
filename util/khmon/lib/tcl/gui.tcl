#####################################
#
#  IBM Coporation 
#  Project kittyhawk
#
#  khmon
#  A crude visualization tool for 
#  demonstrating kittyhawk state
#  
#####################################

proc GraphicsSupport {} {
    proc setupGraphics {} {
        global objLabelFont objImgHeight objImgWidth xinc yinc lw  env
        package require Tk

        set objLabelFont "*-times-medium-r-normal--*-90-*-*-*-*-*-*"
        image create photo testimg -file "$env(KHMON_LIBDIR)/images/greenled.gif"
        set objImgHeight [image height testimg]
        set objImgWidth [image width testimg]
        set xinc [expr $objImgWidth / 2]
        set yinc [expr $objImgHeight / 2]
        set lw [expr $objImgWidth + 10]
        image delete testimg
    }

    proc getViewer { key } {
        global viewerTbl
        if {[info exists viewerTbl($key)]==0} {
            createViewer "$key" "Network Id: $key" blueled.gif 200 100
            set viewerTbl($key) $key
        }
        return "$key"
    }

    proc printViewerVars {} {
        global canvasWidth canvasHeight defImage instanceCount instanceStr filterRegex i x y viewerTbl

        puts "canvasWidth : [array names canvasWidth]"
        puts "canvasHeight : [array names canvasHeight]"
        puts "defImage: [array names defImage]"
        puts "instanceCount : [array names instanceCount]"
        puts "instanceStr : [array names instanceStr]"
        puts "filterRegex : [array names filterRegex]"
        puts "i : [array names i]"
        puts "x : [array names x]"
        puts "y : [array names y]"
        puts "viewerTbl: [array names viewerTbl]"
    }

    proc destroyViewer { name } {
        global canvasWidth canvasHeight defImage instanceCount instanceStr filterRegex i x y viewerTbl
        
        if { $name != "free" } {
            # must destroy frame first to free references to variables
            destroy .$name
            unset canvasWidth($name) canvasHeight($name) defImage($name) instanceStr($name) instanceCount($name) \
                filterRegex($name) x($name) y($name) i($name) viewerTbl($name)
        } else {
            .ob.objcan delete Object ObjectLabel
            set instanceCount(free) 0
        }
    }

    proc createViewer { name title img w h} { 
        global canvasWidth canvasHeight defImage instanceCount instanceStr filterRegex i x y env

        set canvasWidth($name) $w
        set canvasHeight($name) $h
        set defImage($name) $img
        set instanceStr($name) ""
        set instanceCount($name) 0
        set filterRegex($name) "/(\\d*)$"
        set x($name) 0
        set y($name) 0
        set i($name) 0

        if { $name == "free" } {
            set wpath ""
            wm title . "$title"
        } else {
            set wpath .${name}
            toplevel $wpath
            wm title $wpath "$title"
        }

        if {0} {
            frame $wpath.re
            label $wpath.re.regexlabel -text "Filter:"
            entry $wpath.re.regex -width 40 -relief sunken -bd 2 \
                -textvariable filterRegex($name)
            pack $wpath.re.regexlabel $wpath.re.regex -side left -padx 1m \
                -pady 2m -fill x -expand 1
        }

        frame $wpath.ob    
        canvas $wpath.ob.objcan -height $canvasHeight($name) -width $canvasWidth($name) \
            -background white -yscrollcommand [list $wpath.ob.yscroll set] \
            -borderwidth 0 -scrollregion {0 0 0 100000}
        scrollbar $wpath.ob.yscroll -orient vertical -command [list $wpath.ob.objcan yview]
        grid $wpath.ob.objcan $wpath.ob.yscroll -sticky news
        grid columnconfigure $wpath.ob 0 -weight 1
        grid rowconfigure $wpath.ob 0 -weight 1

        frame $wpath.info
        label $wpath.info.title -text "${title}"
#        label $wpath.info.inlbl -text "${title}"
        label $wpath.info.incntlbl -justify left -text "  \#"
        label $wpath.info.incnt -justify left -textvariable instanceCount($name)
#        label $wpath.info.indatalbl -justify left -text "node:"
#        label $wpath.info.indata -justify left -textvariable instanceStr($name)
        pack $wpath.info.title  $wpath.info.incntlbl $wpath.info.incnt -side left \
            -padx 0 -pady 0 -in $wpath.info -fill x
#        pack $wpath.info.title $wpath.info.inlbl $wpath.info.incnt $wpath.info.indatalbl $wpath.info.indata -side left \
\#            -padx 0 -pady 0 -in $wpath.info -fill x

        if {0} { 
            pack $wpath.re -side top -fill x -expand 1
        }

        pack $wpath.ob -fill both -expand 1 -side top
        pack $wpath.info -fill x -side bottom
        
#        $wpath.ob.objcan bind Object <Any-Enter>  "
#            set data \[$wpath.ob.objcan gettags current\]
#            set instanceStr($name) \"\[lindex \$data 2\]\"
#        " 
        
#        $wpath.ob.objcan bind Object <Any-Leave> "
#            set instanceStr($name) \"\"
#        "
                
        $wpath.ob.objcan bind Object <Button-1> "
            set data \[$wpath.ob.objcan gettags current\]
            displayObjInfo \$data
        "
        
        proc ${name}_config {w h} "
            global canvasWidth canvasWidth
            set canvasWidth($name) \$w
            set canvasHeight($name) \$h     
        "
        
        bind $wpath.ob.objcan <Configure> "${name}_config %w %h"
        
        return 1
    }

    proc loadimages {} {
        global env images
        foreach imgfile [glob -nocomplain -directory $env(KHMON_LIBDIR)/images -types f *.gif] {
            set img [file tail $imgfile]
            image create photo $img -file $imgfile
            set images($img) $imgfile
        }
    }
    proc displayObj {viewer group obj node owner pnets inet xnet con} {
        global curStates images canvasWidth defImage objImgWidth objImgHeight objLabelFont xinc yinc lw x y i

        if { $viewer == "free" } {
            set wpath ""
        } else {
            set wpath .${viewer}
        }
        
        if { $i($viewer) == 0 } {
            set x($viewer) $xinc
            set y($viewer) $yinc
            $wpath.ob.objcan delete Object ObjectLabel
        }

        set i($viewer) [expr $i($viewer) + 1]
        set yl [expr $y($viewer) + $objImgHeight/2 + 1]

        set img $defImage($viewer)        
        if {$group != "free" } { 
            if {$inet == 1 || $xnet == 1 || [string is digit -strict $pnets] == 0} {
                set img gateway.gif
            }
            if {[info exists curStates($node)] && [info exists images($curStates($node).gif)]} {
                set img $curStates($node).gif
            } 
        }

        $wpath.ob.objcan create image $x($viewer) $y($viewer) -image $img -tags [list $group $obj Object]
        $wpath.ob.objcan create text $x($viewer) $yl  -anchor n -text "$node" \
              -width $lw -tags [list $group $obj ObjectLabel]
        
        set x($viewer) [expr $x($viewer) + $objImgWidth + 10]
        if { $x($viewer) >= $canvasWidth($viewer) } {
            set x($viewer) $xinc
            set y($viewer) [expr $y($viewer) + $objImgHeight + $objImgHeight]
        }
    }

    proc updateState {} {
        global curStates stateDir
        array unset curStates
        array set curStates [join [glob -tails -nocomplain -directory $stateDir -types f *]]
    }

    proc displayObjs {} {
        global instanceCount i

        updateState
        ###  Node nodeid:owner:pnetids:inet:xnet:consid  ###
        foreach obj [getObjects] {
            set objinfo  [split $obj :] 
            set node [lindex $objinfo 0]
            set owner [lindex $objinfo 1]
            set pnets [lindex $objinfo 2]
            set inet [lindex $objinfo 3]
            set xnet [lindex $objinfo 4]
            set con  [lindex $objinfo 5]          
            if { $owner == "_FREE_" } { 
                displayObj free "free" $obj $node $owner $pnets $inet $xnet $con
            } else {
                foreach pnet [split $pnets ,] {
                    displayObj [getViewer $pnet] $pnet $obj $node $owner $pnets $inet $xnet $con 
                }
            }       
        }
        foreach n [array names instanceCount] {
            if { $i($n) != 0 } {
                set instanceCount($n) $i($n)
                set i($n) 0
            } else {
                destroyViewer $n
            }
        }
    }

    proc displayObjInfo {objdata} {
        global curStates
        set group [lindex $objdata  0]
        set objinfo [split [lindex $objdata 1] :]
        set node [lindex $objinfo 0]
        set owner [lindex $objinfo 1]
        set pnets [lindex $objinfo 2]
        set inet [lindex $objinfo 3]
        set xnet [lindex $objinfo 4]
        set con  [lindex $objinfo 5]          

        if { $inet == 0 } { set inet NO } else { set inet YES }
        if { $xnet == 0 } { set xnet NO } else { set xnet YES }

        set winName ".${group}-${objinfo}-InfoWin"
        toplevel $winName 
        if { $owner == "_FREE_" } {
               message $winName.msg -text "Node: $node is Free\n"
        } else {
            message $winName.msg -text "Node: $node\nOwner: $owner\nExternal Net: $xnet\nInternal Net: $inet\nPrivate Networks: $pnets\nConsole: $con\n"
        }
        label $winName.state -justify left -textvariable curStates($node) -width 10
        button $winName.but -text close -command [list destroy $winName ]
        pack $winName.msg $winName.but $winName.state -side top -in $winName    
   }

    proc redisplay {} {
        global filterRegex
        after 250 { 
            displayObjs
            redisplay
        }
    }

    proc startGraphicsInterface {} {
        loadimages
        createViewer free "FREE" greenled.gif 1000 100
        displayObjs
        redisplay
    }

    setupGraphics
}

GraphicsSupport
