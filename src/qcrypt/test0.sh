#!/bin/sh

./qcrypt coder0.cfg >/dev/null &
pr1=$!
./qcrypt decoder0.cfg >/dev/null &
pr2=$!
> dump.log
socat TCP-L:3333,reuseaddr,fork FILE:dump.log &
pr3=$!
sleep 1
pw=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c 64)
curl -s --data "$pw" http://127.0.0.1:77 >/dev/null &
curl -s --data "$pw" http://127.0.0.1:78  >/dev/null &
sleep 1
data=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c 1024)
echo "$data" | nc 127.0.0.1 7777 >/dev/null &
sleep 1
recv=$(cat dump.log)
if test "$data" = "$recv"; then
  echo "variables are the same"
else
  echo "variables are different"
fi
curl -s --data "quit" http://127.0.0.1:77 >/dev/null &
curl -s --data "quit" http://127.0.0.1:78 >/dev/null &
kill "$pr3"
rm dump.log
