#!/bin/bash
echo "test string" > test.txt
./main test.txt -s 1 -e &
rm -f test.txt.lck

./main test.txt -s 1 -e & 
sleep 1
echo 0 > test.txt.lck

rm -f test.txt*