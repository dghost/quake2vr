#!/bin/bash

IFS=
if [ $# -ge 1 ]; then
	outdir=$1
else
	outdir=../../build
fi

echo "Generating $outdir/vrquake2.pk3..."
cd vrquake2.pk3/
if [ ! -d $outdir ]; then
  mkdir -p $outdir
fi
zip -q -9 -r $outdir/vrquake2.pk3 *

