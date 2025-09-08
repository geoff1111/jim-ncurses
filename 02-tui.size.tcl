#! ../jimsh

load ../wtui.so

set tui [newtui]
$tui delete
catch {$tui size} err
puts UNITIALIZED=$err

set tui [newtui]
set size [$tui size]
$tui delete
puts size=$size

puts "resize the tty and try again"
