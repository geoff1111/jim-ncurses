#! ../jimsh

load ../wtui.so

set tui [newtui]
catch {$tui colwidth} err
$tui delete
puts WRONG_ARGS=$err

set tui [newtui]
set result [$tui colwidth "abc"]
$tui delete
puts WIDTH=\"$result\"

set tui [newtui]
set result [$tui colwidth \U8868]
$tui delete
puts WIDTH=\"$result\"
