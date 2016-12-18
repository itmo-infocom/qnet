#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/scrypt-n4.log
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`

/usr/bin/stunnel $SCRIPTPATH/../stunnel/stunnel-n4_o.conf &

/usr/bin/ncat --sh-exec "tee /tmp/scrypt-n4.txt|ncat 10.0.0.2 1000" -l 10.0.0.4 1003 --keep-open
