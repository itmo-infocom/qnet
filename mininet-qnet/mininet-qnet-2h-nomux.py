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
prefix = path.dirname(sys.argv[0])

cmd='/usr/sbin/sshd'

def connectToRootNS( network, switch, ip, routes ):
    """Connect hosts to root namespace via switch. Starts network.
      network: Mininet() network object
      switch: switch to connect to root namespace
      ip: IP address for root namespace node
      routes: host networks to route to"""
    # Create a node in root namespace and link to switch 0
    root = Node( 'root', inNamespace=False )
    intf = network.addLink( root, switch ).intf1
    root.setIP( ip, intf=intf )
    # Start network that now includes link to root namespace
    network.start()
    # Add routes from root ns to hosts
    for route in routes:
        root.cmd( 'route add -net ' + route + ' dev ' + str( intf ) )

def sshd( network, cmd='/usr/sbin/sshd', opts='-D -o UseDNS=no -u0',
          ip='10.123.123.1/32', routes=None, switch=None ):
    """Start a network, connect it to root ns, and run sshd on all hosts.
       ip: root-eth0 IP address in root namespace (10.123.123.1/32)
       routes: Mininet host networks to route to (10.0/24)
       switch: Mininet switch to connect to root namespace (s1)"""
    if not routes:
        routes = [ '10.0.0.0/24' ]
    if switch is not None:
        print('connect host to mininet network')
        connectToRootNS( network, switch, ip, routes )
    for host in network.hosts:
        host.cmd( cmd + ' ' + opts + '&' )
    print( "*** Waiting for ssh daemons to start" )
    for server in network.hosts:
        waitListening( server=server, port=22, timeout=5 )

    print()
    print( "*** Hosts are running sshd at the following addresses:" )
    print()
    for host in network.hosts:
        print( host.name, host.IP() )
    print()

if len(sys.argv)==4:
    if sys.argv[1] == 'host1':
        hostn = 1
    elif sys.argv[1] == 'host2':
        hostn = 2
    else:
        raise NotImplemented("1st arg: host1|host2")
    ip1 = sys.argv[2]
    ip2 = sys.argv[3]
    controller = ip1
else:
    print 'host1|host2 ip1 ip2'
    sys.exit(1)



setLogLevel( 'info' )
try:
    # two switches with a single remote controller on localhost:6633  
    if hostn == 1:
       c0 = RemoteController( 'c0', ip=controller, port=6633 )
       cmap = { 's1': c0 }
    else:
       c1 = RemoteController( 'c1', ip=controller, port=6633 )
       cmap = { 's2': c1 }
 

    class MultiSwitch( OVSSwitch ):
        "Custom Switch() subclass that connects to different controllers"
        def start( self, controllers ):
            return OVSSwitch.start( self, [ cmap[ self.name ] ] )

    net = Mininet( switch=MultiSwitch, build=False )

    if hostn == 1:
        s = net.addSwitch('s1')
        for n in (1,3):
            h = net.addHost( 'h%d' % n, ip = '10.0.0.%s'%n , mac ='00:00:00:00:00:%s'%(format(n, '02x')) )
            net.addLink( s, h)
        sshd(network=net, ip='10.0.0.100/8',switch=s)
    else:
        s = net.addSwitch('s2')
        for n in (2,4,5):
            h = net.addHost( 'h%d' % n, ip = '10.0.0.%s'%n , mac ='00:00:00:00:00:%s'%(format(n, '02x'))  )
            net.addLink( s, h)
        sshd(network=net,ip='10.0.0.101/8',switch=s)




    #run services on n2,n3,n4,n5
    for host in net.hosts:
        i = int(host.name[1:])
        if i in (2,3,4,5):
            stunnel_script = path.join(prefix, "scripts/scrypt-n%s.sh" %(i)) 
            host.cmd('sh %s 1>/tmp/host-n%s-scrypt.log 2>&1 & '%(stunnel_script, i))

            trans_script = path.join(prefix, "scripts/transparent-n%s.sh" %(i)) 
            host.cmd('sh %s 1>/tmp/host-n%s-trans.log 2>&1 & '%(trans_script, i))

            qcrypt_script = path.join(prefix, "scripts/qcrypt-n%s.sh" %(i)) 
            host.cmd('sh %s 1>/tmp/host-n%s-qcrypt.log 2>&1 & '%(qcrypt_script, i)) 
    
    net.build()
    net.start()
    if hostn == 1:
        gre_ip = ip2
        switch = 's1'
    else:
        gre_ip = ip1
        switch = 's2'
    s.cmd('ovs-vsctl add-port '+switch+' '+switch+'-gre1 -- set interface '+switch+'-gre1 type=gre options:remote_ip='+gre_ip)
    s.cmdPrint('ovs-vsctl show')
    

    CLI( net )
except Exception as e:
    print 'exception', e
finally:
    
    #remove processes on hosts

    for host in net.hosts:
        host.cmd( "kill `ss -nlp|grep LISTEN|sed 's/.*pid=\\(.*\\),.*/\\1/'`" )
    net.stop()
