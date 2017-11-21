#!/bin/sh
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"

echo -n "" > $SCRIPTPATH/decoder_test0.cfg
cat >>$SCRIPTPATH/decoder_test0.cfg <<EOF
[default]
# mode (int) :
#       0 - чтение данных по curl запросу (./KeyByCURL.out)
#       1 - чтение данных посимвольно
#       2 - plug&play
mode : 0

# coder (int) :
#       0 - Декодер
#       1 - Кодер
coder : 0

#port (int) - слушающий порт
port : 3333

#IP (string) - IP подключения
ip : "127.0.0.1"
#ip : "192.168.1.199"
#portDest (int) - порт подключения
#portDest : 2222
#portDest : 3128
portDest : 6666
#portCtrl (int) - порт контроля
portCtrl : 9999
isFpga : 0
packetSize : 768
EOF

echo -n "" > $SCRIPTPATH/coder_test0.cfg
cat >>$SCRIPTPATH/coder_test0.cfg <<EOF
[default]
# mode (int) :
#       0 - чтение данных по curl запросу (./KeyByCURL.out)
#       1 - чтение данных посимвольно
#       2 - plug&play
mode : 0

# coder (int) :
#       0 - Декодер
#       1 - Кодер
coder : 1

#port (int) - слушающий порт
port : 5555

#IP (string) - IP подключения
ip : "127.0.0.1"
#ip : "192.168.1.199"
#portDest (int) - порт подключения
portDest : 2222
#portDest : 3128
#portCtrl (int) - порт контроля
portCtrl : 8888
isFpga : 0
packetSize : 768
EOF
echo "Testing"
python $SCRIPTPATH/pythoncrypt.py $SCRIPTPATH/coder_test0.cfg >/dev/null 2>&1 &
pr1=$!
python $SCRIPTPATH/pythoncrypt.py $SCRIPTPATH/decoder_test0.cfg >/dev/null 2>&1 &
pr2=$!
echo -n "" > $SCRIPTPATH/dump1.log
echo -n "" > $SCRIPTPATH/dump2.log
sleep 8
pw=$(cat /dev/urandom | tr -dc A-E0-9 | fold -w 1000 | head -n 1)
curl -s --silent --data $pw http://127.0.0.1:8888 --output /dev/null
curl -s --silent --data $pw http://127.0.0.1:9999  --output /dev/null
socat TCP-L:2222,reuseaddr,fork FILE:$SCRIPTPATH/dump1.log &
pr3=$!
socat TCP-L:6666,reuseaddr,fork FILE:$SCRIPTPATH/dump2.log &
pr4=$!
sleep 5
data=$(cat /dev/urandom | tr -dc A-E0-9 | fold -w 300 | head -n 1)
sleep 1
echo $data | nc 127.0.0.1 5555 >/dev/null &
sleep 3
recv=$(cat $SCRIPTPATH/dump1.log)
echo "Coded data in dump1.log"
cat $SCRIPTPATH/dump1.log | nc 127.0.0.1 3333 >/dev/null &
sleep 4
recv=$(cat $SCRIPTPATH/dump2.log)
echo "RES"
echo $recv
echo "ORIG"
echo $data
if test "$data" = "$recv"; then
  echo "variables are the same"
else
  echo "variables are different"
fi
kill $pr4
wait $pr4 2>/dev/null
kill $pr3
wait $pr3 2>/dev/null
kill $pr2 
wait $pr2 2>/dev/null
kill $pr1 
wait $pr1 2>/dev/null

rm $SCRIPTPATH/dump2.log
rm $SCRIPTPATH/decoder_test0.cfg
rm $SCRIPTPATH/coder_test0.cfg
