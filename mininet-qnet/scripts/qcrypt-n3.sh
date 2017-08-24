#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/qbcrypt-n3.log
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`


/usr/bin/qcrypt $SCRIPTPATH/../conf/qcrypt-n3_o.cfg 2>>${QCRYPTLOG}&
/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n3.txt|ncat 10.0.0.5 1000" -l 10.0.0.3 1004 --keep-open
