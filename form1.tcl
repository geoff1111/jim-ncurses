#! ../jimsh

load ../wtui.so

if {[catch {
  set tui [newtui]
  $tui colorpair 1 COLOR_YELLOW BRIGHT_YELLOW
  set style1 [$tui style 1 WA_UNDERLINE]
  set form [$tui newform \
	  1 20 1 1 0 \
	  1 10 2 1 0 \
	  1 10 3 1 0 \
	  1 10 4 1 0 \
	  1 10 5 1 0 \
	  1 10 6 1 0 \
	  1 10 7 1 0 \
	  1 10 8 1 0 \
	  1 10 9 1 0 \
	  1 10 10 1 0]
  # field functions: form.create (back|field|fore|just|opt|pad|type)set 
  # form functions: setopt post unpost delete driver mkpanel
  for {set i 0} {$i < 10} {incr i} {
    $form background $i $style1 
  }
  # TODO: change typeset to validate
  $form validate 0 TYPE_IPV4
  $form validate 1 TYPE_ALNUM 2 ;# min field length
  $form validate 2 TYPE_ALPHA 2 ;# min field length
  $form validate 3 TYPE_REGEXP {abc[A-K]*45}
  $form validate 4 TYPE_NUMERIC 10 1 100 ;# precision min max 
  $form validate 5 TYPE_INTEGER 5 10 90  ;# prec min max
  $form validate 6 TYPE_ENUM 0 1 FIRST SECOND THIRD FOURTH FIFTH SIXTH SEVENTH EIGHTH NINTH TENTH ;# case-sensitive partial-unique-match ...
  set panel [$form newpanel 10 10 1 1 1 1]
  $panel border
  $form post

  $tui update
  $panel getch
  $form unpost
  $form delete
  $panel delete
} err errtrace]} {
  $tui delete
  puts $err\n$errtrace
  exit 1
}
$tui delete
