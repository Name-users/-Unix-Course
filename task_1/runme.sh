#!/bin/bash
bash generate_fileA.sh > fileA
make
./main fileA fileB
gzip -k fileA fileB
gzip -cd fileB.gz | ./main fileC
./main fileA fileD -b 100
stat file? file?.gz > result.txt
rm -f file? file?.gz ./main