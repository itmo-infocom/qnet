config = {
           'controllers': {'c0': (192.168.244.243,6634),},
           'switches' : {'s1':'c0',},
           'hosts': ['h1','h2'],
           'phys': ['ens33',],
           'links': [('s1','h1'),('s1','h2'),('s1','ens33'),],
           'conf': 'conf-fpga',
           'rootns': {'s1':'10.0.0.100/8',}
         }
