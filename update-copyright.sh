#!/bin/sh

oldyear=2018
newyear=2019

for dir in src include tools tests; do
    for ext in cc h; do
	find $dir -maxdepth 1 -name *.$ext \
	     -exec sed -i -r \
	     "s/(Copyright \(C\) .*?-)$oldyear Red Hat, Inc/\1$newyear Red Hat, Inc/" \
	     {} \; \
	     -exec sed -i -r \
	     "s/(Copyright \(C\)) ($oldyear) (Red Hat, Inc)/\1 $oldyear-$newyear \3/" \
	     {} \;
    done
done
