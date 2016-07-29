#!/bin/bash
QCRYPTLOG=/dev/null
#QCRYPTLOG=/tmp/qbcrypt-n3.log


/usr/bin/ncat --sh-exec "tee /tmp/qcrypt-n3.txt| bin/qbcrypt share/qbcrypt _Bob.key 2>>${QCRYPTLOG}|ncat 10.0.0.4 1004" -l 10.0.0.3 1002 --keep-open
