config = { 
           'controllers' : {'c0':('192.168.122.243',6633)},
           'switches' : {'s1':'c0'},
           'hosts': ['h1','h2']
           'phys' : 'ens8',
           'links' : [('s1','h1'),('s1',h2'),('s1','ens8')]
           'conf' : 'conf',
           'rootns' : {'s1':'10.0.0.100/8'}
         }
