#!/usr/bin/python
import time
from daemon import runner
import subprocess
import signal
from daemon_tool import App, programCleanup
from functools import partial
import sys
import os

global daemon_runner

data = None
if len(sys.argv)>2:
    data = sys.argv[2:]

app = App(data[1:], pidfile='/tmp/%s.pid'%data[0], logfile='/tmp/%s.log'%data[0])
#app = App(['/usr/bin/echo', '200'], pidfile='/tmp/foo.pid', logfile='/tmp/foo.log')
#app = App(['/usr/bin/sleep', '200'], pidfile='/tmp/foo.pid', logfile='/tmp/foo.log')
daemon_runner = runner.DaemonRunner(app)
daemon_runner.daemon_context.signal_map = { signal.SIGTERM: partial(programCleanup, app, daemon_runner) }
daemon_runner.daemon_context.working_directory = os.getcwd()
daemon_runner.do_action()
