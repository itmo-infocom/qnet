#!/bin/sh
if [ -n "$1" ];
then 
   TESTDATA="$1"
else
   TESTDATA=/usr/share/mininet-qnet/test
fi 




RYUHOST=localhost

test "$1" = '-h' -o "$1" = '-help' -o "$1" = '--help' && echo "Usage: $0 [-h|-help|--help|-i|i]" && exit

MODE=$1

check_channel (){
        c1="UNKNOWN"
        c2="UNKNOWN"
        if [ -f /tmp/trans-n3.txt -a -f /tmp/trans-n4.txt ]
        then 
            c1='transparent'
        else
            if [ -f /tmp/qcrypt-n3.txt -a -f /tmp/qcrypt-n4.txt ]
            then
                c1='qcrypt'
            else
                if [ -f /tmp/scrypt-n3.txt -a -f /tmp/scrypt-n4.txt ]
                then
                    c1='scrypt'
                fi
            fi
        fi

        if [ -f /tmp/trans-n5.txt -a -f /tmp/trans-n2.txt ]
        then 
            c2='transparent'
        else
            if [ -f /tmp/qcrypt-n5.txt -a -f /tmp/qcrypt-n2.txt ]
            then
                c2='qcrypt'

            else
                if [ -f /tmp/scrypt-n5.txt -a -f /tmp/scrypt-n2.txt ]
                then
                    c2='scrypt'
                fi
            fi
        fi
        echo channel 1: $c1, channel 2: $c2

}

test (){
	STATUS=OK
	curl http://${RYUHOST}:8080/qchannel/1/5
        rm -rf /tmp/trans-n*.txt /tmp/qcrypt-n*.txt /tmp/scrypt-n*.txt /tmp/top-secret.txt 
	echo
	ssh 10.0.0.1 "nc 10.0.0.2 1000 < $TESTDATA/test$1"
        sleep 1
#        ls -lt /tmp|head
	diff /tmp/top-secret.txt $TESTDATA/test$1 || STATUS=BAD
	echo
	echo STATUS=$STATUS
#	curl http://${RYUHOST}:8080/qchannel/1/6
        check_channel
	echo
	if [ "$MODE" = i -o "$MODE" = '-i' ]
	then
		echo Press ENTER...
		read
	fi
}

echo Flow tables clearing
curl http://${RYUHOST}:8080/qchannel/1/4
curl http://${RYUHOST}:8080/qchannel/2/4
# test 1
echo Uncrypted channels
curl http://${RYUHOST}:8080/qchannel/1/3
curl http://${RYUHOST}:8080/qchannel/2/3
test 2
echo Quantum crypted channels
curl http://${RYUHOST}:8080/qchannel/1/1
curl http://${RYUHOST}:8080/qchannel/2/1
test 3
echo SSL crypted channels
curl http://${RYUHOST}:8080/qchannel/1/0
curl http://${RYUHOST}:8080/qchannel/2/0
test 1
echo Mix...
echo Transparent -- SSL
curl http://${RYUHOST}:8080/qchannel/1/3
curl http://${RYUHOST}:8080/qchannel/2/0
test 2
echo SSL -- Transparent 
curl http://${RYUHOST}:8080/qchannel/1/0
curl http://${RYUHOST}:8080/qchannel/2/3
test 3
echo Transparent -- Quantum
curl http://${RYUHOST}:8080/qchannel/1/3
curl http://${RYUHOST}:8080/qchannel/2/1
test 1
echo Quantum -- Transparent 
curl http://${RYUHOST}:8080/qchannel/1/1
curl http://${RYUHOST}:8080/qchannel/2/3
test 2
echo SSL -- Quantum 
curl http://${RYUHOST}:8080/qchannel/1/0
curl http://${RYUHOST}:8080/qchannel/2/1
test 3
echo Quantum -- SSL 
curl http://${RYUHOST}:8080/qchannel/1/1
curl http://${RYUHOST}:8080/qchannel/2/0
test 1
