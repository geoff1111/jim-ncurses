#!./jimsh
# requires `testhash`, compile `testhash.c` with `make testhash`

proc hash {str} {
  return [exec ./testhash $str]
}

if {$argv in {-h -H -? --help} || $argc != 4} {
  puts "Usage:"
  puts "cat FILE | $argv0 stats|code|define|hashes|bestArray REGEXP minRowFactor maxRowFactor"
  exit
}
# GET ALL MACROS
set txt [read stdin]
lassign $argv action pattern minRowFactor maxRowFactor
if {$maxRowFactor < 1.0} {puts "maxRowFactor must be >1.0"; exit}
foreach {all m} [regexp -all -inline $pattern $txt] {
  lappend macros $m
}

# KEEP ONLY UNIQUE MACROS
set macros [lsort $macros]
set last ""
foreach item $macros {
  if {$item eq $last} {
    continue
  } else {
    lappend unique $item
    set last $item
  }
}
set macros $unique

set N [llength $macros]

# PRINT DEFINES
if {$action eq "define"} {
  foreach name $macros {
    puts "#define $name [incr Z]"
  }
  exit 
}

# CREATE HASHES
set hashes [dict create]
foreach word $macros {
  dict set hashes $word [hash $word]
}

# PRINT HASHES
if {$action eq "hashes"} {
  puts $hashes
  exit
}

set maxRows [expr {int(ceil($N * $maxRowFactor))}]
set minRows [expr {int(floor($N * $minRowFactor))}]
set minScore 1e9
set checks 0

# CALCULATE bestArray
for {set rows $maxRows} {$rows >= $minRows} {incr rows -1} {
  set results [dict create]
  dict for {name hash} $hashes {
    set idx [expr {$hash % $rows}]
    dict lappend results $idx $name
  }
  set minCols 1
  foreach key [dict keys $results] {
    set cols [llength [dict get $results $key]]
    if {$cols > $minCols} {set minCols $cols}
  }
  set score [expr {$minCols * $minCols * $minCols * $minCols + $rows}]
  if {$score < $minScore} {
    set minScore $score
    set nRows $rows
    set bestArray $results
  }
  incr checks
}

if {$action eq "bestArray"} {
  puts $bestArray
  exit
}

# GET NCOLS and sum
set nCols 0
dict for {idx nlist} $bestArray {
  set len [llength $nlist]
  incr sum $len
  if {$len > $nCols} {set nCols $len}
}

# STATS
if {$action eq "stats"} {
  puts "MaxRows:  $maxRows"
  puts "MinRows:  $minRows"
  puts "MinScore: $minScore"
  puts "Checks:   $checks\n"
  puts "N macros: $N"
  puts "nRows: $nRows"
  puts "nCols: $nCols"
  puts "Average strcmp checks: [expr {$sum * 1.0 / [llength [dict keys $bestArray]]}]"
  puts "Empty elements: [expr {$nRows * $nCols - $N}]\n"
}

# CODE (fills out the dictionary and prints it)
if {$action eq "code"} {
  for {set i 0} {$i < $nRows} {incr i} {
    if {[dict exists $bestArray $i]} {
      while {[llength [dict get $bestArray $i]] < $nCols} {
	# fill empty cols with {}
        dict lappend bestArray $i {}
      }
      set result {}
      foreach element [dict get $bestArray $i] {
        if {$element ne {}} {
	  append result " {\"$element\", $element},"
        } else {
	  append result " {NULL, 0}, "
        }
      }
      puts "   $result"
    } else {
      dict lappend bestArray $i {}
      while {[llength [dict get $bestArray $i]] < $nCols} {
        dict lappend bestArray $i {}
      }
      set result {}
      foreach element [dict get $bestArray $i] {
        if {$element ne {}} {
	  append result " {\"$element\", $element},"
        } else {
	  append result " {NULL, 0},"
        }
      }
      puts "   $result"
    }
  }
}
