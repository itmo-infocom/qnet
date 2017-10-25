config = {
           'controllers': {'c0':('127.0.0.1',6633),},
           'switches' : {'s1':'c0', 's2':'c0'},
           'hosts': ['h1','h2','h3','h4','h5'],
           'phys': [],
           'links': [('s1','h1'),('s1','h2'),('s2','h3'),('s2','h4'),('s2','h5'),('s1','s2')],
           'conf': 'conf2',
           'rootns': {'s1':'10.0.0.102/8',}
         }
