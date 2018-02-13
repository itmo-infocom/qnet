#!/usr/bin/python
import yaml
import sys
import pprint
import os.path

class Loader(yaml.Loader):
    def __init__(self, stream):
        if isinstance(stream,file):
            self._root = os.path.split(stream.name)[0]
        super(Loader, self).__init__(stream)
        Loader.add_constructor('!env', Loader.env)

    def env(self, node):
        if   isinstance(node, yaml.ScalarNode):
            try:
                return os.environ[self.construct_scalar(node)]
            except KeyError:
                print 'No such variable ', self.construct_scalar(node)
                return ''

        else:
             raise NotImplementedError

if __name__=='__main__':

    config = yaml.load(open(sys.argv[1]).read(), Loader)

    pprint.pprint(config)
