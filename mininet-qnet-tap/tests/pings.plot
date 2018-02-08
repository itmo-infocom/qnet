#!/usr/bin/gnuplot

set terminal png size 800,600
set grid
set title "Package Latency"
show title
set output 'pings.png'
set xlabel "Package size (bytes)"
set ylabel "Latency (ms)"
set xrange [60:70000]
set logscale x
plot "pings-raw_udp.dat" using 1:2:3 with errorlines, "pings-udp.dat" using 1:2:3 with errorlines, "pings-raw_tcp.dat" using 1:2:3 with errorlines, "pings-tcp.dat" using 1:2:3 with errorlines

