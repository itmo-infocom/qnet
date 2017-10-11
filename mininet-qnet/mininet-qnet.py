#!/usr/bin/python
import traceback
from mininet_common import sshd, checkIntf
from mininet.log import setLogLevel, info
from mininet.node import RemoteController, OVSSwitch, Controller
from mininet.net import Mininet
from mininet.link import Intf
from mininet.cli import CLI

from os import path
import sys
from subprocess import call
prefix = path.dirname(sys.argv[0]) if len(path.dirname(sys.argv[0]))!=0 else '.'

setLogLevel( 'info' )
#
#setLogLevel( 'debug' )

try:
    exec(open(sys.argv[1]).read())
    conf = config['conf'] if config['conf'][0]=='/' else path.join(prefix, config['conf'])
    controllers={}
    for c in config['controllers']:
        controller_ip,controller_port = config['controllers'][c]
        if controller_ip is None:
            c0 = Controller( c, port=controller_port )
        else:
            c0 = RemoteController( c, ip=controller_ip, port=controller_port )
        controllers[c]=c0
    for intfName in config['phys']:
        checkIntf(intfName)
    cmap={}
    for s in config['switches']:
         cmap[s]=controllers[config['switches'][s]]    
   
    class MultiSwitch( OVSSwitch ):
        "Custom Switch() subclass that connects to different controllers"
        def start( self, controllers ):
            return OVSSwitch.start( self, [ cmap[ self.name ] ] )

    net = Mininet( switch=MultiSwitch, build=False )

    for c in config['controllers']:
        controller_ip,controller_port = config['controllers'][c]
        if controller_ip is None:
            net.addController(controllers[c])

    switches = {}
    for  s in config['switches']:
        switches[s]=net.addSwitch(s)

    hosts = {}
    for h in config['hosts']:
         n = int(h[1:])
         hosts[h] = net.addHost(h,ip='10.0.0.%s'%n, mac='00:00:00:00:00:%s'%(format(n,'02x')))

    for n1,n2 in config['links']:
        if n1 in switches and n2 in config['phys']:
             _intf = Intf(n2, switches[n1])
        elif n2 in switches and n1 in config['phys']:
             _intf = Intf(n1, switches[n2])
        else:
             net.addLink(n1,n2)

    for s,rootns_ip in config['rootns'].items():
         sshd(network=net, ip=rootns_ip, switch=switches[s], host=s)


    net.build()
    net.start()

    #run services on n2,n3,n4,n5
    for host in net.hosts:
        i=int(host.name[1:])
        if i in (2,3,4,5):
	    host.cmd('/usr/bin/stunnel %s/stunnel-n%s.conf  & '%(conf,i))   
            host.cmd('python %s/run_daemon.py start qcrypt_h%s /usr/bin/pythoncrypt %s/qcrypt-n%s.cfg'%(prefix,i,conf,i))
            host.cmd('python %s/run_daemon.py start transparent_h%s /bin/sh %s/transparent_n%s.cfg'%(prefix,i,conf,i))
    CLI(net)
except:
    traceback.print_exc(file=sys.stdout)
finally:
    for host in net.hosts:
        i = int(host.name[1:])
        if i in (2,3,4,5):
             host.cmd('python %s/run_daemon.py stop qcrypt_h%s'%(prefix,i))
             host.cmd('python %s/run_daemon.py stop transparent_h%s'%(prefix,i))
             host.cmd('python %s/run_daemon.py stop muxer_h%s'%(prefix,i))

    for host in net.hosts:
        host.cmd( "kill `ss -nlp|grep LISTEN|sed 's/.*pid=\\(.*\\),.*/\\1/'`" )
    call(["pkill", "controller"])
    net.stop()

     
