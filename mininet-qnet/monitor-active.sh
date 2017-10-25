#!/bin/sh
PREFIX=$(dirname $(realpath $0))
. ${PREFIX}/mininet-functions.sh

set_debug_output 0

if [ -n "$1" ];
then 
   TESTDATA="$1"
else
   TESTDATA="http://192.168.122.1/100k.dat"
fi 


ssh $SSHOPT 10.0.0.1 "curl -o /tmp/data.orig --proxy 10.0.0.102:3128 $TESTDATA" 1>$flow1 2>$flow2

RYUHOST=localhost


test (){
	STATUS="${GREEN}OK${NC}"
        res=$(check_channels_active $TESTDATA)
        ssh $SSHOPT 10.0.0.1 "diff /tmp/data /tmp/data.orig" 1>$flow1 2>$flow2
        if [ $? -ne 0 ]
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




