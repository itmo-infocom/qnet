#!/bin/bash

/usr/bin/ncat --sh-exec "tee /tmp/trans-n5.txt | ncat 10.0.0.2 1000" -l 10.0.0.5 1000 --keep-open
