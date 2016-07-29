#!/bin/bash
QCRYPTLOG=/dev/null
#QCRYPTLOG=/tmp/qbcrypt-n2.log


/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n2.txt| bin/qbuncrypt share/qbcrypt _Bob.key 2>>$QCRYPTLOG|ncat 10.0.0.2 1000" -l 10.0.0.2 1004 --keep-open
