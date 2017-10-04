#!/bin/sh
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
   TESTDATA="http://192.168.122.1/1G.dat"
fi 

#curl -o data.orig $TESTDATA 1>$flow1 2>$flow2

RYUHOST=localhost
DIR=`pwd`

test "$1" = '-h' -o "$1" = '-help' -o "$1" = '--help' && echo "Usage: $0 [-h|-help|--help|-i|i]" && exit

MODE=$1

check_channel_mux (){
        c1="UNKNOWN"
        c2="UNKNOWN"
        scp 10.0.0.2:/tmp/tcpdump-h2.log /tmp/tcpdump-h2.log.copy 1>$flow1 2>$flow2
        port1=$(cat /tmp/tcpdump-h2.log.copy | sed 's/.* > 10.0.0.3.\(10[01][034]\).*/\1/' |head -5|uniq)
        echo $port1 1>$flow1 2>$flow2
        scp 10.0.0.4:/tmp/tcpdump-h4.log /tmp/tcpdump-h4.log.copy 1>$flow1 2>$flow2
        port2=$(cat /tmp/tcpdump-h4.log.copy | sed 's/.* > 10.0.0.5.\(10[01][034]\).*/\1/' |head -5|uniq)
        echo $port2 1>$flow1 2>$flow2

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



        result=1
        if [ "x$c1" = "x$1" -a "x$c2" = "x$2" ]
             then
                 result=0
        fi

        echo channel 1: $c1, channel 2: $c2   
        return $result

}

test (){
	STATUS="${GREEN}OK${NC}"
        echo  start 1>$flow1 2>$flow2
        ssh 10.0.0.2 "tcpdump -i h2-eth0 -nn 2>/dev/null|grep ' > 10.0.0.3.10[01][034]' >/tmp/tcpdump-h2.log" &
        ssh 10.0.0.4 "tcpdump -i h4-eth0 -nn 2>/dev/null|grep ' > 10.0.0.5.10[01][034]' >/tmp/tcpdump-h4.log" &
        sleep 5
        echo  start1 1>$flow1 2>$flow2
        killall tcpdump
        check_channel_mux $2 $3
        if [ $? -ne 0 ]
             then
             STATUS="${RED}BAD${NC}"
        fi
#        diff ../data data.orig
#        if [ $? -ne 0 ]
#             then
#             STATUS="${RED}BAD${NC}"
#        fi

#        ssh 10.0.0.1 "killall curl" 1>$flow1 2>$flow2
	echo -e STATUS=$STATUS
	echo
	if [ "$MODE" = i -o "$MODE" = '-i' ]
	then
		echo Press ENTER...
		read
	fi
}


echo Flow tables clearing
curl http://${RYUHOST}:8080/qchannel/1/4 1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/4 1>$flow1 2>$flow2


echo Loading keys
sh load_keys-python.sh 1>$flow1 2>$flow2

echo Starting muxers
sh run_muxers.sh start

echo Running tests
curl http://${RYUHOST}:8080/qchannel/1/3  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/3  1>$flow1 2>$flow2

ssh 10.0.0.1 "rm -f data; curl --proxy 10.0.0.1:1000 $TESTDATA -o data " 1>$flow1 2>$flow2 &
sleep 1

# test 1
echo Uncrypted channels
curl http://${RYUHOST}:8080/qchannel/1/3  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/3  1>$flow1 2>$flow2
test 2 transparent transparent

echo Quantum crypted channels
curl http://${RYUHOST}:8080/qchannel/1/1  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/1  1>$flow1 2>$flow2
test 3 qcrypt qcrypt

echo SSL crypted channels
curl http://${RYUHOST}:8080/qchannel/1/0  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/0  1>$flow1 2>$flow2
test 1 scrypt scrypt
echo Mix...
echo Transparent -- SSL
curl http://${RYUHOST}:8080/qchannel/1/3  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/0  1>$flow1 2>$flow2
test 2 transparent scrypt
echo SSL -- Transparent 
curl http://${RYUHOST}:8080/qchannel/1/0  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/3  1>$flow1 2>$flow2
test 3 scrypt transparent
echo Transparent -- Quantum
curl http://${RYUHOST}:8080/qchannel/1/3  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/1  1>$flow1 2>$flow2
test 1 transparent qcrypt
echo Quantum -- Transparent 
curl http://${RYUHOST}:8080/qchannel/1/1  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/3  1>$flow1 2>$flow2
test 2 qcrypt transparent
echo SSL -- Quantum 
curl http://${RYUHOST}:8080/qchannel/1/0  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/1  1>$flow1 2>$flow2
test 3 scrypt qcrypt
echo Quantum -- SSL 
curl http://${RYUHOST}:8080/qchannel/1/1  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/0  1>$flow1 2>$flow2
test 1 qcrypt scrypt

ssh 10.0.0.1 "killall curl" 1>$flow1 2>$flow2

sh run_muxers.sh stop 1>$flow1 2>$flow2

echo Flow tables clearing
curl http://${RYUHOST}:8080/qchannel/1/4  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/4  1>$flow1 2>$flow2



