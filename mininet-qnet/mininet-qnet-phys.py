#!/usr/bin/python

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
# setLogLevel( 'debug' )

try:
    exec(open(sys.argv[1]).read())
    hostn = config['hostn']
    hosts = config['hosts']
    intfName = config['intf']
    controller_ip = config['controller_ip']
    controller_port = config['controller_port']
    conf = config['conf'] if config['conf'][0]=='/' else path.join(prefix, conf)
    checkIntf(intfName)
    if controller_ip is None:
        c0 = Controller( 'c%s'%hostn, port=controller_port )
    else:
        c0 = RemoteController( 'c%s'%hostn, ip=controller_ip, port=controller_port )

    cmap = { 's%s'%hostn: c0 }
    class MultiSwitch( OVSSwitch ):
        "Custom Switch() subclass that connects to different controllers"
        def start( self, controllers ):
            return OVSSwitch.start( self, [ cmap[ self.name ] ] )

    net = Mininet( switch=MultiSwitch, build=False )
    if controller_ip is None:
        net.addController(c0)


    s = net.addSwitch('s%s'%hostn)
    info( '*** Adding hardware interface', intfName, 'to switch',
          s.name, '\n' )
    _intf = Intf( intfName, node=s )

    for n in hosts:
        i=int(host.name[1:])
        h =  net.addHost( 'h%d' % n, ip='10.0.0.%s'%n, mac='00:00:00:00:00:%s'%(format(n,'02x')) )
        net.addLink(s,h,port1=portmap[i])
    sshd(network=net, ip='10.0.0.10%s/8'%(hostn-1),switch=s)
    net.build()
    net.start()

    #run services on n2,n3,n4,n5
    for host in net.hosts:
        i=int(host.name[1:])
        if i in (2,3,4,5):
	    host.cmd('/usr/bin/stunnel %s/stunnel-n%s.conf  & '%(conf,i))   
            host.cmd('python %s/run_daemon.py start qcrypt_h%s python %s/pythoncrypt.py %s/qcrypt-n%s.cfg'%(prefix,i,prefix,conf,i))
            host.cmd('python %s/run_daemon.py start transparent_h%s /bin/sh %s/transparent_n%s.cfg'%(prefix,i,conf,i))
    CLI(net)
except Exception as e:
    print 'exception', e
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

     
