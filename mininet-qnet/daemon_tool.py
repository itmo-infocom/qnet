#!/usr/bin/python
import time
from daemon import runner
import subprocess
import signal

class App():
    def __init__(self, command, pidfile, logfile):
        self.stdin_path = '/dev/null'
        self.stdout_path = '/dev/tty'
        self.stderr_path = '/dev/tty'
        self.pidfile_path =  pidfile
        self.pidfile_timeout = 5
        self.command = command
        self.logfile = logfile
    def run(self):
        self.log = open(self.logfile,'a')
        cnt = 0
        while True:
            s = time.time()
            print >>self.log, time.asctime(), 'starting process'
            self.p = subprocess.Popen(self.command, stdout=self.log, stderr=self.log)
            self.p.wait()
            e = time.time()
            print >>self.log, time.asctime(), 'process stopped, %s sec' %(e-s)
            if e-s<5:
                cnt+=1
                wait_time = 0 if cnt<=5 else (4+cnt/10 if cnt<1100 else 120)
                if wait_time >0:

                    print >>self.log, time.asctime(), 'restarting delayed for %s sec' %(wait_time)        
                    time.sleep(wait_time)
            else:
                cnt=0

            self.log.flush()

def programCleanup(app, runner, signal, frame):
    try:
        app.p.terminate()
    except OSError:
        pass
    print >>app.log, time.asctime(), "pythonDaemon STOP"
    runner.daemon_context.terminate(signal,frame)
    app.log.close()

