config = {
           'controllers': {'c0': ('192.168.0.2',6634), 'c1': (None,6635)},
           'switches' : {'s21':'c0', 's22':'c1'},
           'hosts': ['h3','h4'],
           'phys': ['eno3','eno4'],
           'links': [('s21','h3'),('s21','eno3'),('s22','h4'),('s22','eno4'),],
           'conf': 'conf2-fpga',
           'rootns': {'s21':'10.0.0.121/8',}
         }
