#!/usr/bin/python

DIR=$(pwd)
action=$1

ssh 10.0.0.5 "python ${DIR}/run_daemon.py $action reciever_h5 /usr/bin/ncat --sh-exec 'cat /tmp/secret.dat' -l 10.0.0.5 1010 --keep-open"
