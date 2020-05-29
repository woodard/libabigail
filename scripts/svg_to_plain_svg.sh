#!/bin/bash
# SPDX-License-Identifier: LGPL-3.0-or-later

for i in *.svg;
  do inkscape "$i" --export-plain-svg="${i//svg/plain.svg}"
done
