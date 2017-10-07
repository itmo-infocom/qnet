config = { 'controllers': {'c0':('192.168.122.243',6633)},
           'switches' : {'s2':'c0'},
           'hosts': ['h3','h4'],
           'phys' : ['ens8',],
           'links': [('s2','h3'),('s2',h4'),('s2','ens8')],
           'conf' : 'conf',
           'rootns': {'s2':'10.0.0.101/8'}
         }
