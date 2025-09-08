#! ../../jimtcl/jimsh

load ../wtui.so

stdin readable {
  set str {}
  foreach {iscode ch keyname} [$panel getch] {
    if {$keyname ne {}} {
      if {$keyname eq "x"} {
        set now finish
      }
      append str \"$keyname\" " "
    }
  }
  append str [string repeat " " [expr {30 - [string length $str]}]]
  $panel mvcursor 4 2
  $panel print $str 
  $tui update
}

set tui [newtui ndelay]

if {[catch {
  $tui cursor invisible

  set panel [$tui newpanel 12 42 14 10 1 1 1 1]
  $panel border
  for {set i 0} {$i < 10} {incr i} {
    $panel mvcursor $i 0
    $panel print $i
  }
  for {set i 0} {$i < 9} {incr i} {
    $panel mvcursor $i 39
    $panel print $i
  }
  for {set i 0} {$i < 40} {incr i} {
    $panel mvcursor 0 $i
    $panel print [expr {$i % 10}]
  }
  for {set i 0} {$i < 39} {incr i} {
    $panel mvcursor 9 $i
    $panel print [expr {$i % 10}]
  }
  $panel focus border
  $panel mvcursor 0 10
  $panel print header
  $panel mvcursor 11 10
  $panel print footer
  $panel focus interior
  $panel mvcursor 2 10
  $panel print "x\U2714x\U274Cx\U2611x\U263Ax\U2605x\U2665x\U2615"
  $panel mvcursor 3 10
  $panel print "\U2190\U2191\U2192\U2193\U21D0\U2194\U2794\U25B6\U25FC\U25CF\U275D\U275E\U261E\U26A0"

  set panel2 [$tui newpanel 10 40 0 10]
  for {set i 0} {$i < 10} {incr i} {
    $panel2 mvcursor $i 0
    $panel2 print $i
  }
  for {set i 0} {$i < 9} {incr i} {
    $panel2 mvcursor $i 39
    $panel2 print $i
  }
  for {set i 0} {$i < 40} {incr i} {
    $panel2 mvcursor 0 $i
    $panel2 print [expr {$i % 10}]
  }
  for {set i 0} {$i < 39} {incr i} {
    $panel2 mvcursor 9 $i
    $panel2 print [expr {$i % 10}]
  }
  $tui update
} err errtrace]} {
  $tui delete
  puts $err\n$errtrace
  exit 1
}

vwait now

$panel delete
$panel2 delete
$tui delete
