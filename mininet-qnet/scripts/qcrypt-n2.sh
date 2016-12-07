#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/qbcrypt-n2.log

if [ -z "$QCDIR" ]; then QCDIR=share/qbcrypt; fi
if [ -z "$QCSUFFIX" ]; then QCSUFFIX=_Bob.key; fi
echo $QCDIR $QCSUFFIX
/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n2.txt| bin/qbuncrypt $QCDIR $QCSUFFIX 2>>$QCRYPTLOG|ncat 10.0.0.2 1000" -l 10.0.0.2 1004 --keep-open
