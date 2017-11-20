#!/bin/sh

./pythoncrypt.py coder0.cfg >/dev/null 2>&1 &
pr1=$!
./pythoncrypt.py decoder0.cfg >/dev/null 2>&1 &
pr2=$!
echo -n "" > dump1.log
echo -n "" > dump2.log
sleep 1
pw=$(cat /dev/urandom | tr -dc A-E0-9 | fold -w 1000 | head -n 1)
curl -s --silent --data $pw http://127.0.0.1:8888 --output /dev/null &
curl -s --silent --data $pw http://127.0.0.1:9999  --output /dev/null &
socat TCP-L:2222,reuseaddr,fork FILE:dump1.log &
pr3=$!
socat TCP-L:6666,reuseaddr,fork FILE:dump2.log &
pr4=$!
sleep 3
data=$(cat /dev/urandom | tr -dc A-E0-9 | fold -w 300 | head -n 1)
sleep 1
echo $data | nc 127.0.0.1 5555 >/dev/null &
sleep 2
recv=$(cat dump1.log)
echo "CODED"
echo $recv
cat dump1.log | nc 127.0.0.1 3333 >/dev/null &
sleep 3
#tail --pid=$pr3 -f /dev/null
#sleep 5
recv=$(cat dump2.log)
echo "RES"
echo $recv
echo "ORIG"
echo $data
if test "$data" = "$recv"; then
  echo "variables are the same"
else
  echo "variables are different"
fi
kill "$pr4"
kill "$pr3"
kill "$pr2"
kill "$pr1"
rm dump1.log
rm dump2.log
