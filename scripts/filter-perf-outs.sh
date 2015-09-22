#!/bin/bash

# Column numbers for even strings ($11 and $13) are for the files starting with 'Iterations='
# usage: bash filter-perf-outs.sh < filename

i=1;
while read -r line; do
    if [ $((i++ % 2)) -eq 1 ]; then
      echo $line | awk '{printf("%s",$2)}'
    else
      echo $line | awk '{print " " $11 " " $13}'
    fi
done
