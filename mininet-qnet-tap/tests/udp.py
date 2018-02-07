#!/usr/bin/python3

import re
import sys
from statistics import mean, pstdev

pattern = re.compile('.* ([0-9.]*) (.bits/sec) *([0-9.]*) ms *.*\(([0-9.]*)%\)')

bw = []
jitter = []
lost = []

for s in sys.stdin.readlines():
    m = pattern.match(s)
    if m:
        if m.group(2) == "Mbits/sec":
            bw.append(float(m.group(1))*1000)
        else:
            bw.append(float(m.group(1)))
        jitter.append(float(m.group(3)))
        lost.append(float(m.group(4)))
#        print("%f %f %f" % (bw[-1], jitter[-1], lost[-1]))

#print("Bandwidth: %f±%f" % (mean(bw), pstdev(bw)))
#print("Jitter: %f±%f" % (mean(jitter), pstdev(jitter)))
#print("Lost: %f±%f%%" % (mean(lost), pstdev(lost)))
print("%s\t%f\t%f\t%f\t%f\t%f\t%f" % (sys.argv[1],
                                      mean(bw), pstdev(bw),
                                      mean(jitter), pstdev(jitter),
                                      mean(lost), pstdev(lost)))



        
