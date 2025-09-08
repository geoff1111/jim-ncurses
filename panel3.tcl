#! ../../jimtcl/jimsh

load ../wtui.so

if {[catch {
  set tui [newtui]
  set tuisz [$tui size]
  $tui colorpair \
	 1 COLOR_GREEN BRIGHT_BLACK \
	 2 COLOR_RED COLOR_BLACK \
	 3 COLOR_CYAN COLOR_BLUE
  set style0 [$tui style 0]
  set style1 [$tui style 1]
  set style2 [$tui style 2]
  set style3 [$tui style 3]
  set style4 [$tui style 2 WA_UNDERLINE WA_ITALIC]
  set style5 [$tui style 3 WA_REVERSE]
  set panel [$tui newpanel 10 40 10 10 1 1 1 1]
  $panel border 9474 9474 9472 9472 9581 9582 9584 9583
  $panel mvcursor 1 1
  $panel style $style1
  $panel print "a symbol \u{3C0}"
  $tui update
  $panel getch

  $panel mvcursor 2 1
  $panel print "a quick brown fox jumps"
  $tui update
  $panel getch

  $panel style $style2
  $panel printalt WACS_LTEE 
  $panel printalt WACS_RTEE
  $tui update
  $panel getch

  $panel mvcursor 2 3
  $panel chgat 5 $style4
  $tui update
  $panel getch

  $panel scroll 1
  $tui update
  $panel getch

  $panel scroll -1
  $tui update
  $panel getch

  $panel scroll -1
  $tui update
  $panel getch

  # $panel drawline 10 ...

  $tui cursor invisible
  $panel mvcursor 2 3
  $panel chgat 4 $style5
  $tui update

  lassign [$panel getch] iscode ch keyname

  $panel delete
  $tui delete
  puts keyname=\"$keyname\"
} err]} {
  $tui delete
  puts $err
}
