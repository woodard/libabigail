#!/bin/bash 

for i in *.svg; 
  do inkscape $i --export-png=`echo $i | sed -e 's/svg$/png/'` --export-pdf=`echo $i | sed -e 's/svg$/pdf/'`; 
done
