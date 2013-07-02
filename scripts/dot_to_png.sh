#!/bin/bash 

for i in *.gv
do
  echo $i
  outputpng=`echo $i | sed -e 's/gv/png/'`; 
  echo $outputpng
  dot -v -Tpng:cairo -o $outputpng $i;
done
