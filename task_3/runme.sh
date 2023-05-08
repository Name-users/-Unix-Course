mkdir -p /tmp/myinit
rm -rf result
mkdir result
rm -f /tmp/myinit/*
for i in {1..3}
do
touch /tmp/myinit/in_${i}
touch /tmp/myinit/out_${i}
echo "/bin/sleep ${i}s /tmp/myinit/in_${i} /tmp/myinit/out_${i}" >> /tmp/myinit/myinit.cfg
done
make > /dev/null
./main /tmp/myinit/myinit.cfg /tmp/myinit/myinit.log

# Test 1
sleep 3s
echo "Проверяем, что все 3 задачи запущенны" > result/result.txt
echo "$ ps -ef | grep /bin/sleep | head -n 3" >> result/result.txt
ps -ef | grep /bin/sleep | head -n 3 >> result/result.txt
pkill -f "/bin/sleep 2s"
sleep 1s
echo "Проверяем, что все 3 задачи запущенны, после принудительного завершения 2ой" >> result/result.txt
echo "$ ps -ef | grep /bin/sleep | head -n 3" >> result/result.txt
ps -ef | grep /bin/sleep | head -n 3 >> result/result.txt

# Test 2
touch /tmp/myinit/in_4
touch /tmp/myinit/out_4
echo "Проверяем, что конфиг обновился и запущена только одна задача" >> result/result.txt
echo "/bin/sleep 4s /tmp/myinit/in_4 /tmp/myinit/out_4" > /tmp/myinit/myinit.cfg
killall -SIGHUP main
sleep 5s
echo "$ ps -ef | grep /bin/sleep | head -n 1" >> result/result.txt
ps -ef | grep /bin/sleep | head -n 1 >> result/result.txt

# Shutdown myinit
killall -SIGINT main
sleep 5s
cp /tmp/myinit/myinit.log result/
rm -rf /tmp/myinit
rm -f ./main
