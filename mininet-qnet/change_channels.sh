#!/bin/sh

PREFIX=$(dirname $(realpath $0))
. ${PREFIX}/mininet-functions.sh

set_debug_output 0

RYUHOST=localhost

declare -a c1=($1)
declare -a c2=($2)
sleep_time=$3
l1=${#c1[@]}
l2=${#c2[@]}

echo $l1 $l2 1>$flow1 2>$flow2
cnt=0
while true; do
   sleep $sleep_time
   cnt=$(($cnt+1))
   p1=${c1[$(random $l1)]} 
   p2=${c2[$(random $l2)]} 
   echo $p1 $p2 1>$flow1 2>$flow2

   curl http://${RYUHOST}:8080/qchannel/1/$p1 1>$flow1 2>$flow2
   curl http://${RYUHOST}:8080/qchannel/2/$p2 1>$flow1 2>$flow2
   echo_date
   echo -e "channel 1: $(key_to_cname $p1), channel 2: $(key_to_cname $p2) count=$cnt"
done
