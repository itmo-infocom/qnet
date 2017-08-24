#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/scrypt-n3.log
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`

/usr/bin/stunnel $SCRIPTPATH/../conf/stunnel-n3_o.conf &

/usr/bin/ncat --sh-exec "tee /tmp/scrypt-n3.txt|ncat 10.0.0.5 1000" -l 10.0.0.3 1003 --keep-open
