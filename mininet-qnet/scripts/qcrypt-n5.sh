#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/qbcrypt-n5.log

SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`


/usr/bin/qcrypt $SCRIPTPATH/../conf/qcrypt-n5_o.cfg 2>>${QCRYPTLOG}&
/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n5.txt|ncat 10.0.0.5 1012" -l 10.0.0.5 1002 --keep-open
