#!/bin/bash

IFS=
if [ $# -ge 1 ]; then
	outdir=$1
else
	outdir=../../build
fi

pk3file=$outdir/vrquake2.pk3

echo "Generating $pk3file ..."
cd vrquake2.pk3/
if [ ! -d $outdir ]; then
  mkdir -p $outdir
fi
if [ -e $pk3file ]; then
	if [ -z "`find -newer $pk3file -print -quit`" ]; then
		echo "$pk3file up to date."
		exit
	fi
fi
zip -q -9 -r $pk3file *

