#!/bin/bash


doit() {
   realName=`basename $1`
   cmake-build-debug/pathfinder2 "requests/$realName" "results/$realName.pfres"
}
export -f doit


parallel --joblog calc.log --bar -j8 doit ::: requests/*.pfreq
#parallel --joblog calc.log --retry-failed
#parallel  -a list.txt --bar -j16 doit