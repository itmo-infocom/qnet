config={
        1: {'ip':"192.168.122.243", 'controller':"192.168.122.243",'port':6633, 'sw':'s1', 'hosts':(1,2)},
        2: {'ip':"192.168.122.169", 'controller':"192.168.122.243",'port':6633, 'sw':'s2', 'hosts':(3,4)},
        3: {'ip':"192.168.122.251", 'controller':"192.168.122.243",'port':6633, 'sw':'s3', 'hosts':(5,)},
        'tunnels': [(1,2),(2,3)]
       }
