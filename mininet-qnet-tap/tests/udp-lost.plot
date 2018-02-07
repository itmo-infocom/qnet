#!/usr/bin/gnuplot

set terminal png size 800,600
set grid
set title "UDP Lost Packages"
show title
set output 'udp-lost.png'
set xlabel "Load Traffic (Kbits/sec)"
set ylabel "Lost Packages (%)"
set xrange [0:1100]
plot "udp-raw_udp.dat" using 1:6:7 with errorlines, "udp-udp.dat" using 1:6:7 with errorlines, "udp-raw_tcp.dat" using 1:6:7 with errorlines, "udp-tcp.dat" using 1:6:7 with errorlines

