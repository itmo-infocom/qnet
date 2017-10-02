#!/usr/bin/python
import sys

from mininet.net import Mininet
from mininet.node import OVSSwitch, Controller, RemoteController
from mininet.topolib import TreeTopo
from mininet.log import setLogLevel
from mininet.cli import CLI
from mininet.node import Host, Node
from mininet.util import ensureRoot, waitListening
from os import path
from mininet_common import connectToRootNS, sshd
prefix = path.dirname(sys.argv[0]) if len(path.dirname(sys.argv[0]))!=0 else '.'


if len(sys.argv)==3:
    if sys.argv[2] == 'host1':
        hostn = 1
    elif sys.argv[2] == 'host2':
        hostn = 2
    elif sys.argv[2] == 'host3':
        hostn = 3
    else:
        raise NotImplemented("2nd arg: host1|host2|host3")
    config = sys.argv[1]
    exec(open(config).read())
else:
    print 'config host1|host2'
    sys.exit(1)




setLogLevel( 'info' )
#setLogLevel( 'debug' )
try:
    single_host = False
    if 'single_host' in config and config['single_host']:
        hostn_list = filter(lambda x: isinstance(x,int), config.keys())
        single_host = True
    else:
        hostn_list = [hostn,]

    cmap = {}
    for i in range(len(hostn_list)):
        hostn_=hostn_list[i]
        sw = config[hostn_]['sw']
        hosts = config[hostn_]['hosts']
        controller = config[hostn_]['controller']
        port = config[hostn_]['port']
        conf = config['conf'] if config['conf'][0]=='/' else path.join(prefix, config['conf'])

        c0 = RemoteController( 'c%s'%i, ip=controller, port=port )
        cmap[sw] = c0 

 

    class MultiSwitch( OVSSwitch ):
        "Custom Switch() subclass that connects to different controllers"
        def start( self, controllers ):
            return OVSSwitch.start( self, [ cmap[ self.name ] ] )

    net = Mininet( switch=MultiSwitch, build=False )
    switches={}
    for i in range(len(hostn_list)):
        hostn_=hostn_list[i]
        sw = config[hostn_]['sw']
        hosts = config[hostn_]['hosts']    
        s = net.addSwitch(sw)
        switches[hostn_] = s
        for n in hosts:
            h = net.addHost( 'h%d' % n, ip = '10.0.0.%s'%n , mac ='00:00:00:00:00:%s'%(format(n, '02x')))
            net.addLink( s, h)
            h.cmd('ip link set mtu 1400 dev h%s-eth0'%n)
    sshd(network=net, ip='10.0.0.10%s/8'%(3-hostn),switch=s)



    #run services on n2,n3,n4,n5
    for host in net.hosts:
        i = int(host.name[1:])
        if i in (2,3,4,5):
	    host.cmd('/usr/bin/stunnel %s/stunnel-n%s.conf  & '%(conf,i))   
            host.cmd('python %s/run_daemon.py start qcrypt_h%s python %s/pythoncrypt.py %s/qcrypt-n%s.cfg'%(prefix,i,prefix,conf,i))
            host.cmd('python %s/run_daemon.py start transparent_h%s /bin/sh %s/transparent_n%s.cfg'%(prefix,i,conf,i))

    tunnels=config['tunnels']
    if single_host:
        for i in range(0,len(tunnels)):
            tunnel = tunnels[i]
            net.addLink(switches[tunnel[0]], switches[tunnel[1]])       

 
    net.build()
    net.start()
    if not single_host:
        for i in range(0,len(tunnels)):
            tunnel = tunnels[i]
            if hostn not in tunnel:
                continue
            if hostn==tunnel[0]:
                gre_ip = config[tunnel[1]]['ip']
            else:
                gre_ip = config[tunnel[0]]['ip']
            switch = config[hostn]['sw'] 
            gren='gre'+str(i+1)
            s.cmd('ovs-vsctl add-port '+switch+' '+switch+'-'+gren+' -- set interface '+switch+'-'+gren+' type=gre options:remote_ip='+gre_ip)
            s.cmdPrint('ovs-vsctl show')
        


    CLI( net )
except Exception as e:
    print 'exception', e
finally:
    for host in net.hosts:
        i = int(host.name[1:])
        if i in (2,3,4,5):
             host.cmd('python %s/run_daemon.py stop qcrypt_h%s'%(prefix,i))
             host.cmd('python %s/run_daemon.py stop transparent_h%s'%(prefix,i))
             host.cmd('python %s/run_daemon.py stop muxer_h%s'%(prefix,i))
 
    #remove processes on hosts

    for host in net.hosts:
        host.cmd( "kill `ss -nlp|grep LISTEN|sed 's/.*pid=\\(.*\\),.*/\\1/'`" )
    net.stop()
