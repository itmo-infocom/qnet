#!/bin/sh
python run_daemon.py stop qcrypt1 /usr/bin/qcrypt coder.cfg 
python run_daemon.py stop qcrypt2 /usr/bin/qcrypt decoder.cfg 
python run_daemon.py stop mux1 /usr/bin/tcpmux 8888 127.0.0.1:7777
python run_daemon.py stop mux2 /usr/bin/tcpmux --demux 3333 127.0.0.1:3128

