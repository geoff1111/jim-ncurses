#! ../../jimtcl/jimsh

load ../wtui.so

set tui [newtui]
set minfo {}
set outcoords {}
if {[catch {
  set tuisz [$tui size]
  $tui colorpair \
	  1 COLOR_GREEN COLOR_BLACK \
	  2 COLOR_RED COLOR_BLACK
  set style0 [$tui style 0]
  set style1 [$tui style 1 WA_ITALIC]
  set style2 [$tui style 2]
  set panel [$tui newpanel 10 40 10 10 1 1 1 1]
  $panel border
  $panel focus inside
  $panel mvcursor 1 1
  $panel style $style1
  $panel print "a symbol \u{3C0}"
  $panel style $style2
  $panel printalt WACS_LTEE 
  $panel printalt WACS_RTEE
  $panel style $style0
  $tui update
  $panel focus border
  lassign [$panel getch] iscode ch keyname
  if {$keyname eq "KEY_MOUSE"} {
    set minfo [$panel mouse]
    if {$minfo(loc) eq "outer"} {
      set outcoords [$panel trafo $minfo(row) $minfo(col)]
    }
  }
  $panel delete
} err]} {
  $tui delete
  puts mouseinfo=$minfo
  puts $err
  exit
}
$tui delete
puts keyname=$keyname
puts minfo=$minfo
puts outcoords(loc,row,col)=$outcoords
