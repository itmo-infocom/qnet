config = {
           'controllers': {'c0': ('192.168.0.2',6633),},
           'switches' : {'s1':'c0',},
           'hosts': ['h1','h2'],
           'phys': ['eno3',],
           'links': [('s1','h1'),('s1','h2'),('s1','eno3'),],
           'conf': 'conf2',
           'rootns': {'s1':'10.0.0.100/8',}
         }
