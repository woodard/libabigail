#!/bin/bash
# SPDX-License-Identifier: LGPL-3.0-or-later

for i in *.gv
do
  echo "$i"
  outputpng=${i//gv/png}
  echo "$outputpng"
  dot -v -Tpng:cairo -o "$outputpng" "$i"
done
