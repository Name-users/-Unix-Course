#!/bin/bash
echo "test string" > test.txt
echo -e "PID\tlock\tunlock" > result.txt
for i in {1..10}
do
  ./main test.txt -s 1 &
done

sleep 5m
killall -SIGINT main
rm -f test.txt