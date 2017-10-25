#!/bin/sh

PREFIX=$(dirname $(realpath $0))
. ${PREFIX}/mininet-functions.sh

set_debug_output 0

trap stop_muxers SIGINT

if [ -n "$1" ];
then 
   TESTDATA="$1"
else
   TESTDATA="http://192.168.122.1/1G.dat"
fi 


RYUHOST=localhost


test (){
	STATUS="${GREEN}OK${NC}"
        echo  start 1>$flow1 2>$flow2
        res=$(check_channels_passive $2 $3)
        if [ $? -ne 0 ]
             then
             STATUS="${RED}BAD${NC}"
        fi
	echo -e $res 
        echo -e STATUS=$STATUS
	echo
}


echo Flow tables clearing
curl http://${RYUHOST}:8080/qchannel/1/4 1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/4 1>$flow1 2>$flow2


echo Loading keys
sh load_keys-python.sh 1>$flow1 2>$flow2

echo Starting muxers
sh run_muxers.sh start 1>$flow1 2>$flow2 

echo Running tests
curl http://${RYUHOST}:8080/qchannel/1/3  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/3  1>$flow1 2>$flow2

ssh 10.0.0.1 "rm -f data; curl --proxy 10.0.0.1:1000 $TESTDATA -o data --limit-rate 100k" 1>$flow1 2>$flow2 &
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

ssh $SSHOPT 10.0.0.1 "killall curl" 1>$flow1 2>$flow2

sh run_muxers.sh stop 1>$flow1 2>$flow2

echo Flow tables clearing
curl http://${RYUHOST}:8080/qchannel/1/4  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/4  1>$flow1 2>$flow2



