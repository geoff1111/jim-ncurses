#! ../jimsh

load ../wtui.so

set tui [newtui]
set result [$tui delete]
puts EMPTY=\"$result\"
