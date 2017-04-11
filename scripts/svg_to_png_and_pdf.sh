#!/bin/bash

for i in *.svg;
  do inkscape "$i" --export-png="${i//svg/png}" --export-pdf="${i//svg/pdf}"
done
