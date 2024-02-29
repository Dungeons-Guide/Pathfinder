#!/bin/bash


doit() {
   realName=`basename $1`
   cmake-build-release/cli/pathfinder-cli "requests/342efd7e-036b-45ef-9d08-defb6b0c39e3.pfreq/$realName" "results/$realName.pfres"
}
export -f doit


parallel --joblog calc.log --bar -j8 doit ::: requests/342efd7e-036b-45ef-9d08-defb6b0c39e3.pfreq/*.pfreq
#parallel --joblog calc.log --retry-failed
#parallel  -a list.txt --bar -j16 doit