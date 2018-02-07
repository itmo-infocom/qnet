#!/usr/bin/gnuplot

set terminal png size 800,600
set grid
set title "UDP Jitter"
show title
set output 'udp-jitter.png'
set xlabel "Load Traffic (Kbits/sec)"
set ylabel "Jitter (ms)"
set xrange [0:1100]
plot "udp-raw_udp.dat" using 1:4:5 with errorlines, "udp-udp.dat" using 1:4:5 with errorlines, "udp-raw_tcp.dat" using 1:4:5 with errorlines, "udp-tcp.dat" using 1:4:5 with errorlines

