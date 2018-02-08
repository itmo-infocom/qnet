#!/usr/bin/gnuplot

set terminal png size 800,600
set grid
set title "TCP Bandwidth"
show title
set output 'tcp.png'
set xlabel "Load Traffic (Kbits/sec)"
set ylabel "Bandwidth (Kbits/sec)"
set xrange [0.9:1100]
set logscale x
plot "tcp-raw_udp.dat" using 1:2:3 with errorlines, "tcp-udp.dat" using 1:2:3 with errorlines, "tcp-raw_tcp.dat" using 1:2:3 with errorlines, "tcp-tcp.dat" using 1:2:3 with errorlines

