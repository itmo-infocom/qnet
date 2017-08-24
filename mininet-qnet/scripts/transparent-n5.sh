#!/bin/bash
outfile=/tmp/top-secret.txt

/usr/bin/ncat --sh-exec "tee /tmp/trans-n5.txt >$outfile" -l 10.0.0.5 1000 --keep-open
