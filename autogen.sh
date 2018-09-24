#!/bin/bash


rm -Rf Makefile.in aclocal.m4 ar-lib compile config.guess config.h.in config.sub configure depcomp install-sh ltmain.sh m4 missing src/Makefile.in
autoreconf --install
rm -Rf autom4te.cache
