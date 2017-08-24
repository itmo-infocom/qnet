#!/bin/bash

/usr/bin/ncat --sh-exec "tee /tmp/trans-n2.txt | ncat 10.0.0.3 1000" -l 10.0.0.2 1000 --keep-open

