config = {
           'controllers': {'c0': (None,6634), 'c1': (None,6635)},
           'switches' : {'s21':'c0', 's22':'c1'},
           'hosts': ['h3','h4'],
           'phys': ['ens34','enp4s4'],
           'links': [('s21','h3'),('s21','ens34'),('s22','h4'),('s22','enp4s4'),],
           'conf': 'conf-fpga',
           'rootns': {'s21':'10.0.0.121/8',}
         }
