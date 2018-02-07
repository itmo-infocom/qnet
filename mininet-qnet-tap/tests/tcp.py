#!/usr/bin/python3

import re
import sys
from statistics import mean, pstdev

pattern = re.compile('.* ([0-9.]*) (.bits/sec)')

bw = []

for s in sys.stdin.readlines():
    m = pattern.match(s)
    if m:
        if m.group(2) == "Mbits/sec":
            bw.append(float(m.group(1))*1000)
        else:
            bw.append(float(m.group(1)))
#        print("%f" % bw[-1])

#print("Bandwidth: %f±%f" % (mean(bw), pstdev(bw)))
print("%s\t%f\t%f" % (sys.argv[1], mean(bw), pstdev(bw)))


        
