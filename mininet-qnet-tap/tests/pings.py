#!/usr/bin/python3

import re
import sys
from statistics import mean, pstdev

pattern = re.compile('([0-9.]*) .* time=([0-9.]*) ms')

latency = {}

for s in sys.stdin.readlines():
    m = pattern.match(s)
    if m:
        size = int(m.group(1))
        if not size in latency:
            latency[size] = []
        latency[size].append(float(m.group(2)))
#        print("%f\t%f" % (size[-1], latency[-1]))

for size in sorted(latency):
    print("%d\t%f\t%f" % (size, mean(latency[size]), pstdev(latency[size])))


        
