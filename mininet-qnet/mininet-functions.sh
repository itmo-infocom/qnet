#!/bin/sh
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
BROWN='\033[0;33m'

NC='\033[0m'

SSHOPT="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"


port_to_name (){
        local c1;
        case $1 in
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
   echo $c1
}

port_to_cname (){
        local c1;
        case $1 in
            1000)
                     c1=${BROWN}transparent${NC}
                     ;;
            1003)
                     c1=${BLUE}scrypt${NC}
                     ;;
            1004)
                     c1=${GREEN}qcrypt${NC}
                     ;;
            *)
                     c1=${RED}UNKNOWN${NC}
                     ;;
            esac
   echo $c1
}


set_debug_output () {
    if [ $1 -ne 0 ]; then
        flow1=/dev/stdout
        flow2=/dev/stderr
    else
        flow1=/dev/null
        flow2=/dev/null
    fi
}

check_channels_passive (){       
        port1=$(ssh $SSHOPT 10.0.0.2 "tcpdump -i h2-eth0 -nn  -l -c 5 'ip src host 10.0.0.2 and  dst host 10.0.0.3 and (port 1000 or 1003 or 1004) and tcp[tcpflags] & (tcp-syn|tcp-fin) = 0' 2>/dev/null| sed 's/.* > 10.0.0.3.\(10[01][034]\):.*/\1/' |uniq" 2>$flow2)
        echo $port1 1>$flow1 2>$flow2
        port2=$(ssh $SSHOPT 10.0.0.4 "tcpdump -i h4-eth0 -nn  -l -c 5 'ip src host 10.0.0.4 and  dst host 10.0.0.5 and (port 1000 or 1003 or 1004) and tcp[tcpflags] & (tcp-syn|tcp-fin) = 0' 2>/dev/null| sed 's/.* > 10.0.0.5.\(10[01][034]\):.*/\1/' |uniq" 2>$flow2)
        echo  $port2 1>$flow1 2>$flow2

        c1=$(port_to_cname $port1)
        c2=$(port_to_cname $port2)

        echo -ne " channel 1: $c1, channel 2: $c2" 
        if [[ $# -eq 2 ]]; then
            return $(check_ports $1 $2 $port1 $port2)
        else
            return 1
        fi
}
echo_date (){
        echo -n "$(date '+%Y-%m-%d %H:%M:%S ')"
}
check_channels_active () {
        ssh $SSHOPT 10.0.0.2 "tcpdump -i h2-eth0 -nn  -l -c 5 'ip src host 10.0.0.2 and  dst host 10.0.0.3 and (port 1000 or 1003 or 1004) and tcp[tcpflags] & (tcp-syn|tcp-fin) = 0' 2>/dev/null" >/tmp/h2-tcpdump.log 2>$flow2 &
        ssh $SSHOPT 10.0.0.4 "tcpdump -i h4-eth0 -nn  -l -c 5 'ip src host 10.0.0.4 and  dst host 10.0.0.5 and (port 1000 or 1003 or 1004) and tcp[tcpflags] & (tcp-syn|tcp-fin) = 0' 2>/dev/null" >/tmp/h4-tcpdump.log 2>$flow2 &
        ssh $SSHOPT 10.0.0.1 "rm -f /tmp/data; curl -m 10 --proxy 10.0.0.1:1000 $1 -o /tmp/data " 1>$flow1 2>$flow2 
        kill %1 1>$flow1 2>$flow2
        kill %2 1>$flow1 2>$flow2
        port1=$(cat /tmp/h2-tcpdump.log| sed 's/.* > 10.0.0.3.\(10[01][034]\):.*/\1/' |uniq )
        port2=$(cat /tmp/h4-tcpdump.log| sed 's/.* > 10.0.0.5.\(10[01][034]\):.*/\1/' |uniq)
        echo $port1 1>$flow1 2>$flow2
        echo $port2 1>$flow1 2>$flow2
        echo -ne " channel 1: $(port_to_cname $port1), channel 2: $(port_to_cname $port2) " 
        return $(check_ports $2 $3 $port1 $port2)
}

check_ports (){
        c1=$(port_to_name $3)
        c2=$(port_to_name $4)
        result=1
        if [ "x$c1" = "x$1" -a "x$c2" = "x$2" ]
             then
                 result=0
        fi
        return $result
}

random () {
   local number
   number=$RANDOM; 
   let "number %= $1"; 
   echo $number
}

key_to_cname () {
   local res
   case $1 in
     0) 
        res=$(port_to_cname 1003)
        ;;
     1) 
        res=$(port_to_cname 1004)
        ;;
     3) 
        res=$(port_to_cname 1000)
        ;;
     *) 
        res=$(port_to_cname -1)
   esac
   echo $res
}

stop_muxers (){
    ssh $SSHOPT 10.0.0.1 "killall curl" 1>$flow1 2>$flow2

    echo stopping muxers
    sh ${PREFIX}/run_muxers.sh stop 1>$flow1 2>$flow2
    exit
}
