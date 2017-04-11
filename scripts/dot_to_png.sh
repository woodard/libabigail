#!/bin/bash

for i in *.gv
do
  echo "$i"
  outputpng=${i//gv/png}
  echo "$outputpng"
  dot -v -Tpng:cairo -o "$outputpng" "$i"
done
