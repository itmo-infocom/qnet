#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/scrypt-n2.log
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`


/usr/bin/stunnel $SCRIPTPATH/../conf/stunnel-n2_o.conf &

/usr/bin/ncat --sh-exec "tee /tmp/scrypt-n2.txt|ncat 10.0.0.2 1011" -l 10.0.0.2 1001 --keep-open
