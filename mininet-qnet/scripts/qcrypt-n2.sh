#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/qbcrypt-n2.log
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`


/usr/bin/qcrypt $SCRIPTPATH/../conf/qcrypt-n2_o.cfg 1>>${QCRYPTLOG} 2>&1  &
/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n2.txt|ncat 10.0.0.2 1012" -l 10.0.0.2 1002 --keep-open
