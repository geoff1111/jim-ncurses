#! ../jimsh

load ../wtui.so

set tui [newtui]
catch {$tui colorpair} err
$tui delete
puts WRONG_ARGS=$err

set tui [newtui]
set result [$tui colorpair \
	1 COLOR_WHITE COLOR_GREEN \
	2 COLOR_MAGENTA COLOR_CYAN \
	3 COLOR_BLUE COLOR_RED \
	4 COLOR_YELLOW COLOR_BLACK \
	5 BRIGHT_RED BRIGHT_YELLOW]
$tui delete
puts RESULT=\"$result\"
