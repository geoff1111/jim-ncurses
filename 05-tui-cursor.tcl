#! ../jimsh

load ../wtui.so

set tui [newtui]
catch {$tui cursor} err
$tui delete
puts WRONG_ARGS=$err

set tui [newtui]
$tui cursor normal
sleep 2
$tui cursor invisible
sleep 2
$tui cursor hi-vis
sleep 2
$tui delete
