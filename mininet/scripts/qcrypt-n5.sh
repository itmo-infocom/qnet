#!/bin/bash
QCRYPTLOG=/dev/null
#QCRYPTLOG=/tmp/qbcrypt-n5.log


/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n5.txt| bin/qbcrypt share/qbcrypt _Bob.key 2>>${QCRYPTLOG}|ncat 10.0.0.2 1004" -l 10.0.0.5 1002 --keep-open
