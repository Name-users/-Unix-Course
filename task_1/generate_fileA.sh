#!/bin/bash
echo -ne '\001'
for i in {1..9999}
do
echo -ne '\000'
done
echo -ne '\001'
for i in {10001..4194303}
do
echo -ne '\000'
done
echo -ne '\001'
