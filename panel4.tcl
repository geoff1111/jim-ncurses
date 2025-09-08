#! ../../jimtcl/jimsh

load ../wtui.so
set err {}

catch {
  set tui [newtui]
  $tui colorpair \
	  1 BRIGHT_YELLOW COLOR_YELLOW
  set style1 [$tui style 1]
  set tuisz [$tui size]
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
  $panel focus inside
  $panel mvcursor 2 10
  $panel print "x\U2714x\U274Cx\U2611x\U263Ax\U2605x\U2665x\U2615"
  $panel mvcursor 3 10
  $panel print "\U2190\U2191\U2192\U2193\U21D0\U2194\U2794\U25B6\U25FC\U25CF\U275D\U275E\U261E\U26A0"

  $tui update
  $panel getch

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
  $panel2 style $style1
  $panel2 mvcursor 3 1
  $panel2 drawline right 10
  $panel2 mvcursor 1 15
  $panel2 drawline down 5

  $tui update
  set getc [$panel getch]
  $panel delete
} err errdetails

$tui delete
if {$err ne {}} {
  puts $err
  puts $errdetails
  exit
}
if {[exists getc]} {
  puts $getc
}
