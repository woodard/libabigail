#!/bin/bash

for i in *.svg;
  do inkscape "$i" --export-plain-svg="${i//svg/plain.svg}"
done
