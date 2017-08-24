#!/bin/bash

/usr/bin/ncat --sh-exec "tee /tmp/trans-n3.txt| ncat 10.0.0.5 1000" -l 10.0.0.3 1000 --keep-open
