#! ../jimsh

load ../wtui.so

set menuaction [dict create {*}{
  KEY_DOWN	REQ_DOWN_ITEM
  KEY_UP	REQ_UP_ITEM
  KEY_RIGHT	REQ_RIGHT_ITEM
  KEY_LEFT	REQ_LEFT_ITEM
  KEY_NPAGE	REQ_SCR_DPAGE
  KEY_PPAGE	REQ_SCR_UPAGE
  KEY_HOME	REQ_FIRST_ITEM
  KEY_END	REQ_LAST_ITEM
  KEY_BACKSPACE	REQ_BACK_PATTERN
  KEY_MOUSE	KEY_MOUSE
}]

if {[catch {
  set tui [newtui]
  set clrdetail [$tui colordetail]
  set menu1 [$tui newmenu \
	  opt1 desc1 \
	  opt2 desc2 \
	  opt2 desc3 \
	  Exit ""]
  $menu1 mark " * " 
  $menu1 format 4 1
  set panel1 [$menu1 newpanel 10 10 1 1 1 1]
  $panel1 border
  $menu1 post
  while 1 {
    $tui update
    lassign [$panel1 getch] iscode ch keyname
    if {[dict exists $menuaction $keyname]} {
      $menu1 driver $menuaction($keyname)
    }
    if {$keyname eq "^J" || $keyname eq "DC1" || $keyname eq "x"} break
  } 
  set selects [$menu1 getchoice]
  $menu1 unpost
  $panel1 delete
  $menu1 delete

  set menu2 [$tui newmenu \
	  "Oky doky" "the description" \
	  "yessir" "description 2" \
	  "abracadabra" "description 3" \
	  Exit ""]
  $menu2 mark "*"
  $menu2 format 2 2
  $menu2 option off O_ONEVALUE O_ROWMAJOR
  $menu2 option on O_SHOWDESC 
  dict set menuaction { } REQ_TOGGLE_ITEM
  set panel2 [$menu2 newpanel 10 10 1 1 1 1]
  $panel2 border
  $menu2 post
  while 1 {
    $tui update
    lassign [$panel2 getch] iscode ch keyname
    if {[dict exists $menuaction $keyname]} {
      $menu2 driver $menuaction($keyname)
    }
    if {$keyname eq "^J" || $keyname eq "DC1" || $keyname eq "x"} break
  } 
  set selects2 [$menu2 getchoice]
  $menu2 unpost
  $panel2 delete
  $menu2 delete
} err]} {
  $tui delete
  puts $err
  exit 1
}
$tui delete
puts selects=$selects
puts selects2=$selects2
