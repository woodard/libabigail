#!/bin/bash

for i in *.gv
do
  echo "$i"
  outputsvg=${i//gv/svg}
  echo "$outputsvg"
  dot -v -Tsvg:cairo -o "$outputsvg" "$i"
done
