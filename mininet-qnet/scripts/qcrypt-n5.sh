#!/bin/bash
QCRYPTLOG=/dev/null
QCRYPTLOG=/tmp/qbcrypt-n5.log

if [ -z "$QCDIR" ]; then QCDIR=share/qbcrypt; fi
if [ -z "$QCSUFFIX" ]; then QCSUFFIX=_Bob.key; fi
echo $QCDIR $QCSUFFIX

/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n5.txt| bin/qbcrypt $QCDIR $QCSUFFIX 2>>${QCRYPTLOG}|ncat 10.0.0.2 1004" -l 10.0.0.5 1002 --keep-open
