#!/bin/sh
flow1=/dev/stdout
flow2=/dev/stderr

flow1=/dev/null
flow2=/dev/null


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
        if [ -f /tmp/trans-n2.txt -a -f /tmp/trans-n3.txt ]
        then 
            c1='transparent'
        else
            if [ -f /tmp/qcrypt-n2.txt -a -f /tmp/qcrypt-n3.txt ]
            then
                c1='qcrypt'
            else
                if [ -f /tmp/scrypt-n2.txt -a -f /tmp/scrypt-n3.txt ]
                then
                    c1='scrypt'
                fi
            fi
        fi

        if [ -f /tmp/trans-n4.txt -a -f /tmp/trans-n5.txt ]
        then 
            c2='transparent'
        else
            if [ -f /tmp/qcrypt-n4.txt -a -f /tmp/qcrypt-n5.txt ]
            then
                c2='qcrypt'

            else
                if [ -f /tmp/scrypt-n4.txt -a -f /tmp/scrypt-n5.txt ]
                then
                    c2='scrypt'
                fi
            fi
        fi
        result=1
        if [ "x$c1" = "x$1" -a "x$c2" = "x$2" ]
             then
                 result=0
        fi

        echo channel 1: $c1, channel 2: $c2   
        return $result

}

test (){
	STATUS=OK
#	curl http://${RYUHOST}:8080/qchannel/1/5 
        rm -rf /tmp/trans-n*.txt /tmp/qcrypt-n*.txt /tmp/scrypt-n*.txt /tmp/top-secret.txt* 
        for i in `seq 1 5`; do
           ssh root@10.0.0.${i}  'rm -rf /tmp/trans-n*.txt /tmp/qcrypt-n*.txt /tmp/scrypt-n*.txt /tmp/top-secret.txt'  1>$flow1 2>$flow2
        done

        echo  start 1>$flow1 2>$flow2
	ssh 10.0.0.1 "nc 10.0.0.5 1000 < $TESTDATA/test$1"
        sleep 1
#        ls -lt /tmp|head
        for i in `seq 2 5`; do
            scp root@10.0.0.${i}:/tmp/trans-n${i}.txt /tmp/  1>$flow1 2>$flow2
            scp root@10.0.0.${i}:/tmp/qcrypt-n${i}.txt /tmp/  1>$flow1 2>$flow2
            scp root@10.0.0.${i}:/tmp/scrypt-n${i}.txt /tmp/  1>$flow1 2>$flow2
        done
        scp root@10.0.0.5:/tmp/top-secret.txt /tmp/top-secret.txt.copy  1>$flow1 2>$flow2

	diff /tmp/top-secret.txt.copy $TESTDATA/test$1 1>$flow1 2>$flow2 || STATUS=BAD  
	echo
#	curl http://${RYUHOST}:8080/qchannel/1/6
        check_channel $2 $3
        if [ $? -ne 0 ]
             then
             STATUS=BAD
        fi
	echo STATUS=$STATUS
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


echo loading keys
KeyByCURL.out 127.0.0.1:8080/qkey/1  1>$flow1 2>$flow2
KeyByCURL.out 127.0.0.1:8080/qkey/2  1>$flow1 2>$flow2
echo


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

echo Flow tables clearing
curl http://${RYUHOST}:8080/qchannel/1/4  1>$flow1 2>$flow2
curl http://${RYUHOST}:8080/qchannel/2/4  1>$flow1 2>$flow2

