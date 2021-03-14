#!/bin/sh

OUTDIR=rndkeen1/

/usr/local/djgpp/bin/i586-pc-msdosdjgpp-gcc -o RANDKEEN.EXE randkeen.c
cp RNDKEEN1.BAT $OUTDIR
cp RANDKEEN.EXE $OUTDIR
cp CWSDPMI.EXE $OUTDIR
cp CK1PATCH.EXE $OUTDIR
cp randkeen.c $OUTDIR
cp levels/* $OUTDIR
cp README.TXT $OUTDIR
cp MAPINFO.TXT $OUTDIR
