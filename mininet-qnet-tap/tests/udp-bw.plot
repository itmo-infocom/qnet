#!/usr/bin/gnuplot

set terminal png size 800,600
set grid
set title "UDP Bandwidth"
show title
set output 'udp-bw.png'
set xlabel "Load Traffic (Mbits/sec)"
set ylabel "Bandwidth (Kbits/sec)"
set xrange [0.9:11000]
set logscale x
plot "udp-raw_udp.dat" using 1:2:3 with errorlines, "udp-udp.dat" using 1:2:3 with errorlines, "udp-raw_tcp.dat" using 1:2:3 with errorlines, "udp-tcp.dat" using 1:2:3 with errorlines, "udp-direct.dat" using 1:2:3 with errorlines

