#!/bin/bash 

for i in *.gv
do
  echo $i
  outputsvg=`echo $i | sed -e 's/gv/svg/'`; 
  echo $outputsvg
  dot -v -Tsvg:cairo -o $outputsvg $i;
done
