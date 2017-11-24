#!/bin/sh
PREFIX=$(dirname $(realpath $0))
. ${PREFIX}/mininet-functions.sh

trap stop_muxers SIGINT

set_debug_output 0

if [ -n "$1" ];
then 
   TESTDATA="$1"
else
   TESTDATA="http://192.168.122.1/100k.dat"
fi 

SSHOPT="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"

ssh $SSHOPT 10.0.0.1 "curl -o /tmp/data.orig --proxy 10.0.0.102:3128 $TESTDATA" 1>$flow1 2>$flow2

RYUHOST=localhost



test (){
        sleep 5
	STATUS="${GREEN}OK${NC}"
        echo  start 1>$flow1 2>$flow2
        ret=1
        for i in $(seq 10); do

#            res=$(check_channels_active $TESTDATA $1 $2) 1>$flow1 2>$flow2
            check_channels_active $TESTDATA $1 $2 1>/tmp/res.txt 2>$flow2
            ret1=$?
            echo ret1  $ret1 1>$flow1 2>$flow2
            ssh $SSHOPT 10.0.0.1 "diff /tmp/data /tmp/data.orig" 1>$flow1 2>$flow2
            ret2=$?
            echo ret2 $ret2 1>$flow1 2>$flow2
            if [ $ret1 -eq 0 -a $ret2 -eq 0 ]; then ret=0;break; fi
        done


        if [ $ret -ne 0 ]
             then
             STATUS="${RED}BAD${NC}"
        fi

        ssh $SSHOPT 10.0.0.1 "killall curl" 1>$flow1 2>$flow2
#        echo_date
        cat /tmp/res.txt
#        echo -e $res
        echo
       	echo -e STATUS=$STATUS
}


echo Flow tables clearing
curl http://${RYUHOST}:8080/qchannel/1/4 1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/4 1>$flow1 2>$flow2

echo Loading keys
sh $PREFIX/load_keys-python.sh 1>$flow1 2>$flow2

echo Starting muxers
sh $PREFIX/run_muxers.sh start 1>$flow1 2>$flow2

echo Running tests
# test 1
echo Uncrypted channels
curl http://${RYUHOST}:8080/qchannel/1/3  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/3  1>$flow1 2>$flow2
test transparent transparent

echo Quantum crypted channels
curl http://${RYUHOST}:8080/qchannel/1/1  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/1  1>$flow1 2>$flow2
test qcrypt qcrypt


echo SSL crypted channels
curl http://${RYUHOST}:8080/qchannel/1/0  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/0  1>$flow1 2>$flow2
test scrypt scrypt
echo Mix...
echo Transparent -- SSL
curl http://${RYUHOST}:8080/qchannel/1/3  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/0  1>$flow1 2>$flow2
test transparent scrypt
echo SSL -- Transparent 
curl http://${RYUHOST}:8080/qchannel/1/0  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/3  1>$flow1 2>$flow2
test scrypt transparent
echo Transparent -- Quantum
curl http://${RYUHOST}:8080/qchannel/1/3  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/1  1>$flow1 2>$flow2
test transparent qcrypt
echo Quantum -- Transparent 
curl http://${RYUHOST}:8080/qchannel/1/1  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/3  1>$flow1 2>$flow2
test qcrypt transparent
echo SSL -- Quantum 
curl http://${RYUHOST}:8080/qchannel/1/0  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/1  1>$flow1 2>$flow2
test scrypt qcrypt
echo Quantum -- SSL 
curl http://${RYUHOST}:8080/qchannel/1/1  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/0  1>$flow1 2>$flow2
test qcrypt scrypt

sh $PREFIX/run_muxers.sh stop 1>$flow1 2>$flow2

echo Flow tables clearing
curl http://${RYUHOST}:8080/qchannel/1/4  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/4  1>$flow1 2>$flow2



