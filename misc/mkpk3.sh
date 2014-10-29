#! /bin/bash

echo "Generating vrquake2.pk3..."
cd vrquake2.pk3/
if [ ! -d "../../build/" ]; then
  mkdir ../../build
fi
zip -q -9 -r ../../build/vrquake2.pk3 *

