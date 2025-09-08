#! ../jimsh

load ../wtui.so

catch {newtui nodebug} err
puts WRONG_ARG=$err

catch {newtui 1 2} err
puts EXCESS_ARGS=$err

set name [newtui]
$name delete
puts name=$name

set name [newtui ndelay]
$name delete
puts name.ndelay=$name

set tui [newtui]
catch {newtui ndelay} err
$tui delete
puts REINIT_ERR=$err
