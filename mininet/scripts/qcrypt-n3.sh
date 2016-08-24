#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/qbcrypt-n3.log

if [ -z "$QCDIR" ]; then QCDIR=share/qbcrypt; fi
if [ -z "$QCSUFFIX" ]; then QCSUFFIX=_Bob.key; fi
echo $QCDIR $QCSUFFIX

/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n3.txt| bin/qbcrypt $QCDIR $QCSUFFIX 2>>${QCRYPTLOG}|ncat 10.0.0.4 1004" -l 10.0.0.3 1002 --keep-open
