#!/bin/bash
outfile=/tmp/top-secret.txt

/usr/bin/ncat --sh-exec "tee /tmp/trans-n2.txt >$outfile" -l 10.0.0.2 1000 --keep-open

