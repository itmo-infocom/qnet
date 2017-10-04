controller_ip="192.168.244.242"
config={
        1: {'ip':"192.168.122.243", 'controller':controller_ip,'port':6633, 'sw':'s1', 'hosts':(1,2)},
        2: {'ip':"192.168.122.243", 'controller':controller_ip,'port':6633, 'sw':'s2', 'hosts':(3,4,5)},
        'tunnels': [(1,2),],
        'single_host':True,
        'conf': 'conf'
       }

