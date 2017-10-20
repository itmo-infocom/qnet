#!/bin/sh
PREFIX=$(dirname $(realpath $0))

debug=0
if [ $debug -ne 0 ]; then
flow1=/dev/stdout
flow2=/dev/stderr
else
flow1=/dev/null
flow2=/dev/null
fi

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

if [ -n "$1" ];
then 
   TESTDATA="$1"
else
   TESTDATA="http://192.168.122.1/100k.dat"
fi 

SSHOPT="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"

ssh $SSHOPT 10.0.0.1 "curl -o /tmp/data.orig --proxy 10.0.0.102:3128 $TESTDATA" 1>$flow1 2>$flow2

RYUHOST=localhost
DIR=`pwd`

test "$1" = '-h' -o "$1" = '-help' -o "$1" = '--help' && echo "Usage: $0 [-h|-help|--help|-i|i]" && exit

MODE=$1

check_channel_mux (){
        c1="UNKNOWN"
        c2="UNKNOWN"
        rm -f /tmp/tcpdump_h?.log.copy
        port1=$(ssh $SSHOPT 10.0.0.2 "cat /tmp/tcpdump-h2.log | sed 's/.* > 10.0.0.3.\(10[01][034]\).*/\1/' |uniq" 2>/$flow2)
        echo $port1 1>$flow1 2>$flow2
        port2=$(ssh $SSHOPT 10.0.0.4 "cat /tmp/tcpdump-h4.log | sed 's/.* > 10.0.0.5.\(10[01][034]\).*/\1/' |uniq" 2>/$flow2) 
        echo  $port2 1>$flow1 2>$flow2

        case $port1 in
            1000)
                     c1=transparent
                     ;;
            1003)
                     c1=scrypt
                     ;;
            1004)
                     c1=qcrypt
                     ;;
            *)
                     c1=UNKNOWN
                     ;;
        esac
        case $port2 in
            1000)
                     c2=transparent
                     ;;
            1003)
                     c2=scrypt
                     ;;
            1004)
                     c2=qcrypt
                     ;;
            *)
                     c2=UNKNOWN
                     ;;
        esac



        echo -n $(date "+%Y-%m-%d %H:%M:%S ")
        echo -n " channel 1: $c1, channel 2: $c2"   

}

test (){
	STATUS="${GREEN}OK${NC}"
        echo  start 1>$flow1 2>$flow2
        ssh $SSHOPT 10.0.0.2 "tcpdump -i h2-eth0 -nn 2>/dev/null|grep ' > 10.0.0.3.10[01][034]' >/tmp/tcpdump-h2.log" 1>$flow1 2>$flow2 & 
        ssh $SSHOPT 10.0.0.4 "tcpdump -i h4-eth0 -nn 2>/dev/null|grep ' > 10.0.0.5.10[01][034]' >/tmp/tcpdump-h4.log" 1>$flow1 2>$flow2 &
 
        echo  start1 1>$flow1 2>$flow2
        ssh $SSHOPT 10.0.0.1 "rm -f /tmp/data; curl --proxy 10.0.0.1:1000 $TESTDATA -o /tmp/data " 1>$flow1 2>$flow2 
        ssh $SSHOPT 10.0.0.2 "killall tcpdump" 1>$flow1 2>$flow2
        ssh $SSHOPT 10.0.0.4 "killall tcpdump" 1>$flow1 2>$flow2
#        kill %1 %2 1>$flow1 2>$flow2
        check_channel_mux 
        ssh $SSHOPT 10.0.0.1 "diff /tmp/data /tmp/data.orig" 1>$flow1 2>$flow2
        if [ $? -ne 0 ]
             then
             STATUS="${RED}BAD${NC}"
        fi

        ssh $SSHOPT 10.0.0.1 "killall curl" 1>$flow1 2>$flow2
	echo -e " DATA=$STATUS"
}

while true; do
test 
done




