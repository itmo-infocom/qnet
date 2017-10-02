#!/usr/bin/python

DIR=$(pwd)
action=$1

ssh 10.0.0.5 "python ${DIR}/run_daemon.py $action muxer_h5 /usr/bin/tcpmux --demux 10.0.0.5:1010 10.0.0.102:3128"
sleep 3
ssh 10.0.0.1 "python ${DIR}/run_daemon.py $action muxer_h1 /usr/bin/tcpmux  10.0.0.1:1000 10.0.0.5:1000"
