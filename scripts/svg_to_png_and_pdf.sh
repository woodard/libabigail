#!/bin/bash
# SPDX-License-Identifier: LGPL-3.0-or-later

for i in *.svg;
  do inkscape "$i" --export-png="${i//svg/png}" --export-pdf="${i//svg/pdf}"
done
