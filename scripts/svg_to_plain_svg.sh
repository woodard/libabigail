#!/bin/bash 

for i in *.svg; 
  do inkscape $i --export-plain-svg=`echo $i | sed -e 's/svg$/plain.svg/'`; 
done
