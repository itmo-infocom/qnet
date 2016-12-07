#!/usr/bin/python

"""
Create a network where different switches are connected to
different controllers, by creating a custom Switch() subclass.
"""

from mininet.net import Mininet
from mininet.node import OVSSwitch, Controller, RemoteController
from mininet.topolib import TreeTopo
from mininet.log import setLogLevel
from mininet.cli import CLI
from mininet.node import Host, Node
from mininet.util import ensureRoot, waitListening

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
    if not switch:
        switch = network[ 's1' ]  # switch to use
    if not routes:
        routes = [ '10.0.0.0/24' ]
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


setLogLevel( 'info' )
try:
    # two switches with a single remote controller on localhost:6633  
    c0 = RemoteController( 'c0', ip='127.0.0.1', port=6633 )
    c1 = RemoteController( 'c1', ip='127.0.0.1', port=6633 )
 
    cmap = { 's1': c0, 's2': c1 }

    class MultiSwitch( OVSSwitch ):
        "Custom Switch() subclass that connects to different controllers"
        def start( self, controllers ):
            return OVSSwitch.start( self, [ cmap[ self.name ] ] )

    net = Mininet( switch=MultiSwitch, build=False )


    s1 = net.addSwitch('s1')
    s2 = net.addSwitch('s2')


    n1,n2,n3,n4,n5 = [ net.addHost( 'h%d' % n ) for n in ( 1, 2, 3, 4 ,5 ) ]
    #set topology
    net.addLink( s1, n1)
    net.addLink( s1, n3)
    net.addLink( s2, n4)
    net.addLink( s2, n5)
    net.addLink( s1, s2)
    net.addLink( s2, n2)
    #set MACs
    for i,host in enumerate([n1,n2,n3,n4,n5]):
         host.setMAC('00:00:00:00:00:%s'%(format(i+1, '02x')))

    #run sshd on each host
    sshd(network=net, ip='10.0.0.100/8',switch=s1)

    #run services on n2,n3,n4,n5
    for i,host in enumerate([n1,n2,n3,n4,n5]):

        if i in (1,2,3,4):
            host.cmd('/usr/bin/stunnel stunnel/stunnel-n%s.conf  & '%(i+1))   
            host.cmd('sh scripts/transparent-n%s.sh 1>/tmp/host-n%s-trans.log 2>&1 & '%(i+1, i+1))
            host.cmd('sh scripts/qcrypt-n%s.sh 1>/tmp/host-n%s-qcrypt.log 2>&1 & '%(i+1, i+1))
    
    net.build()
    net.start()
    CLI( net )
except Exception as e:
    print 'exception', e
finally:
    
    #remove processes on hosts
    for host in net.hosts:
        host.cmd( "kill `ss -nlp|grep LISTEN|sed 's/.*pid=\\(.*\\),.*/\\1/'`" )
    net.stop()
