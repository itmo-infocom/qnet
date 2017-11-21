#!/bin/sh
PREFIX=$(dirname $(realpath $0))
. ${PREFIX}/mininet-functions.sh

set_debug_output 0

if [ -n "$1" ];
then 
   TESTDATA="$1"
else
   TESTDATA="http://192.168.122.1/10k.dat"
fi 


ssh $SSHOPT 10.0.0.1 "curl -o /tmp/data.orig --proxy 10.0.0.102:3128 $TESTDATA" 1>$flow1 2>$flow2

RYUHOST=localhost


test (){
	STATUS="${GREEN}OK${NC}"
        ret1=0
        ret2=0
        cnt=0
#        check_channels_active $TESTDATA
        for i in $(seq 10); do

            res=$(check_channels_active $TESTDATA) 1>$flow1 2>$flow2
            echo $res|grep UNKNOWN >/dev/null
            ret1=$?
            ssh $SSHOPT 10.0.0.1 "diff /tmp/data /tmp/data.orig" 1>$flow1 2>$flow2
            ret2=$?
            cnt=$(($cnt+1))
#            echo $cnt $ret1 $ret2
            if [ $ret1 -ne 0 -a $ret2 -eq 0 ]; then break; fi
        done
        if [ $ret2 -ne 0 ]
             then
             STATUS="${RED}BAD${NC}"
        fi

        echo_date
        echo -ne $res
	echo -e " DATA=$STATUS"
}

while true; do
test 
done




