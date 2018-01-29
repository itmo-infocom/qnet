#!/usr/bin/python
from mininet.node import Node
from mininet.util import quietRun
from mininet.link import Intf
from mininet.util import ensureRoot, waitListening
from mininet.log import info, error

import re

cmd='/usr/sbin/sshd'

def connectToRootNS( network, switch, ip, routes ,host=''):
    """Connect hosts to root namespace via switch. Starts network.
      network: Mininet() network object
      switch: switch to connect to root namespace
      ip: IP address for root namespace node
      routes: host networks to route to"""
    # Create a node in root namespace and link to switch 0
    root = Node( 'root'+host, inNamespace=False )
    intf = network.addLink( root, switch ).intf1
    root.setIP( ip, intf=intf )
    # Start network that now includes link to root namespace
    network.start()
    # Add routes from root ns to hosts
    for route in routes:
        root.cmd( 'route add -net ' + route + ' dev ' + str( intf ) )

def sshd( network, cmd='/usr/sbin/sshd', opts='-D -o UseDNS=no -u0 -f /var/lib/qcrypt/sshd_config',
          ip='10.123.123.1/32', routes=None, switch=None ,host=''):
    """Start a network, connect it to root ns, and run sshd on all hosts.
       ip: root-eth0 IP address in root namespace (10.123.123.1/32)
       routes: Mininet host networks to route to (10.0/24)
       switch: Mininet switch to connect to root namespace (s1)"""
    if not switch:
        switch = network[ 's1' ]  # switch to use
    if not routes:
        routes = [ '10.0.0.0/24' ]
    connectToRootNS( network, switch, ip, routes ,host)
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

def checkIntf( intf ):
    "Make sure intf exists and is not configured."
    config = quietRun( 'ifconfig %s 2>/dev/null' % intf, shell=True )
    if not config:
        error( 'Error:', intf, 'does not exist!\n' )
        exit( 1 )
    ips = re.findall( r'\d+\.\d+\.\d+\.\d+', config )
    if ips:
        error( 'Error:', intf, 'has an IP address,'
               'and is probably in use!\n' )
        exit( 1 )


