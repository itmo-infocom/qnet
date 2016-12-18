#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/scrypt-n2.log
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`


/usr/bin/stunnel $SCRIPTPATH/../stunnel/stunnel-n2_o.conf &

/usr/bin/ncat --sh-exec "tee /tmp/scrypt-n2.txt|ncat 10.0.0.2 1000" -l 10.0.0.2 1003 --keep-open
