#!/bin/bash
declare -a keys
ipcs -m | grep 666 > tmp
keys=( $(awk '{ print $2 } ' tmp) )
rm tmp
for s in ${keys[@]}; do
    ipcrm -m ${s}  
    echo "${s}"
done
