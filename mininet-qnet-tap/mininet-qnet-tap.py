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
import yaml
import sys
import pprint
import time

config = yaml.load(open(sys.argv[1]))
host = sys.argv[2]
#pprint.pprint(config)

setLogLevel('info')

host_config=config['hosts'][host]
try:
    controllers={}
    controllers_to_add = list()
    for c in host_config['controllers']:
        controller_ip = c['ip']
        controller_port = c['port']
        controller_name = c['name']
        print 'Creating controller:', controller_name
        if controller_ip == 'None':
            c0 = Controller( controller_name, port=controller_port)
            controllers_to_add.append(c0)
        else:
            c0 = RemoteController( controller_name, ip=controller_ip, port=controller_port )
        controllers[controller_name]=c0

    devices = host_config['phys']
    to_kill = []
    for task in sorted(host_config['pre-scripts'], key=lambda x: x['prio']):
        if task['type'] == 'device':
             devices.append(task)
        print "Running pre-script", task['cmdline'].format(**task)
        if isinstance(task['vhost'], dict):
            host = hosts[task['vhost']['name']]
            host.cmd(task['cmdline'].format(**task))
        else:
            call(task['cmdline'].format(**task), shell=True)
        to_kill.append(task)
    time.sleep(5)
             


    for device in devices:
        checkIntf(device['device'])
    cmap={}
    for switch_data in host_config['switches']:
         switch_name = switch_data['name']
         switch_controller_name = switch_data['controller']['name']
         cmap[switch_name]=controllers[switch_controller_name]
    class MultiSwitch( OVSSwitch ):
        "Custom Switch() subclass that connects to different controllers"
        def start( self, controllers ):
            return OVSSwitch.start( self, [ cmap[ self.name ] ])

    net = Mininet( switch=MultiSwitch, build=False)

    for c in controllers_to_add:
        net.addController(c)

    switches = {}
    for  s in host_config['switches']:
        switches[s['name']]=net.addSwitch(s['name'])
    hosts = {}
    for h in host_config['vhosts']:
         host_ip = h['ip']
         host_mac = h['mac']
         host_name = h['name']
         hosts[host_name] = net.addHost(host_name,ip=host_ip, mac=host_mac)
          

    for n1,n2 in config['links']:
        if n1['type']=='switch' and n2['type']=='device':
             intfName = n2['device']
             checkIntf(intfName)

             _intf = Intf(intfName, switches[n1['name']])
        elif n2['type']=='switch' and n1['type']=='device':
             intfName = n1['device']
             checkIntf(intfName)
             _intf = Intf(intfName, switches[n2['name']])
        else:
             net.addLink(n1['name'],n2['name'])
    if 'ssh' in host_config: 
        for instance in host_config['ssh']:
            print instance
            rootns_ip = instance['ip']
            switch_name = instance['switch']['name']
            sshd(network=net, ip=rootns_ip, switch=switches[switch_name], host=switch_name)
    net.build()
    net.start()
    for task in sorted(host_config['post-scripts'], key=lambda x: x['prio']):
        if task['type'] == 'device':
             devices.append(task)
        print "Running post-script", task['cmdline'].format(**task)
        if isinstance(task['vhost'],dict):
            host = hosts[task['vhost']['name']]
            host.cmd(task['cmdline'].format(**task))
        else:
            call(task['cmdline'].format(**task), shell=True)
        to_kill.append(task)
    time.sleep(5)

    CLI(net)
except:
    traceback.print_exc(file=sys.stdout)
finally:
    call(["pkill", "controller"])
    for task in to_kill:
       if 'killcmd' in task:
           print task['killcmd'].format(**task)
           call(task['killcmd'].format(**task), shell=True)
    net.stop()

