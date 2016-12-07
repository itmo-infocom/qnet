#!/bin/sh
python run_daemon.py start qcrypt1 /usr/bin/qcrypt conf/coder.cfg 
python run_daemon.py start qcrypt2 /usr/bin/qcrypt conf/decoder.cfg 
python run_daemon.py start mux1 /usr/bin/tcpmux 8888 127.0.0.1:7777
python run_daemon.py start mux2 /usr/bin/tcpmux --demux 3333 127.0.0.1:3128

