#!/bin/sh
PREFIX=$(dirname $(realpath $0))
. ${PREFIX}/mininet-functions.sh

set_debug_output 0 

RYUHOST=localhost

test (){
        echo  start 1>$flow1 2>$flow2
        res=$(check_channels_passive )
        echo_date
        echo $res
}


while true; do
 test 
done



