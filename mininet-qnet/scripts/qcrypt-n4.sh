#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/qbcrypt-n4.log
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`


/usr/bin/qcrypt $SCRIPTPATH/../conf/qcrypt-n4_o.cfg 2>>${QCRYPTLOG}&
/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n4.txt|ncat 10.0.0.2 1000" -l 10.0.0.4 1004 --keep-open
