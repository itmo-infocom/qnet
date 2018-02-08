# Testing measurements

## Latency

pings size -- measure latency by sending ICMP packages with chosen size (keep in mind 8-byte ICMP header).

pings.py -- creates gnuplot data-file from ping's logs.

pings.plot -- gnuplot script for graph drawing.

## TCP

iperf_tcp_clt load_traffic[KM] -- client part of iperf utility which sending load traffic for measuring of bandwidth.

iperf_tcp_srv -- server part of iperf utility.

tcp.py load_traffic -- creates gnuplot data-file from iperf_tcp_srv's logs for load traffic in Kibits/sec.

tcp.plot -- gnuplot script for bandwidth graph drawing.

## UDP

iperf_udp_clt load_traffic[KM] -- client part of iperf utility which sending load traffic for measuring of bandwidth, jitter and latency.

iperf_udp_srv -- server part of iperf utility.

udp.py load_traffic -- creates gnuplot data-file from iperf_udp_srv's logs for load traffic in Kibits/sec.

udp-bw.plot -- gnuplot script for bandwidth graph drawing.

udp-jitter.plot -- gnuplot script for jitter graph drawing.

udp-lost.plot -- gnuplot script for latency graph drawing.
