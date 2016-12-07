#!/bin/sh
python run_daemon.py start mux1t /usr/bin/tcpmux 8888 127.0.0.1:7777
python run_daemon.py start mux2t /usr/bin/tcpmux --demux 7777 127.0.0.1:3128

