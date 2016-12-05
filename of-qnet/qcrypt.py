import json
import logging
import os
import requests	

from ryu.app import simple_switch
from webob import Response
from ryu.app.wsgi import ControllerBase, WSGIApplication, route
from ryu.app import ofctl_rest
from ryu.lib.mac import haddr_to_bin

from ryu.base import app_manager
from ryu.controller import ofp_event, dpset
from ryu.lib.ofctl_v1_0 import mod_flow_entry, delete_flow_entry


simple_switch_instance_name = 'qchannel_api_app'
qconf = '/var/lib/qcrypt/channels.json'
#qpath = '/etc/qcrypt/'
qpath = '/usr/share/qcrypt/'

#class QuantumSwitchRest(simple_switch.SimpleSwitch, ofctl_rest.StatsController):
class QuantumSwitchRest(simple_switch.SimpleSwitch):

    _CONTEXTS = { 'dpset': dpset.DPSet,
                  'wsgi': WSGIApplication }

    def __init__(self, *args, **kwargs):
        super(QuantumSwitchRest, self).__init__(*args, **kwargs)
        self.switches = {}
        dpset = kwargs['dpset']
        wsgi = kwargs['wsgi']
        self.data = {}
        self.data['dpset'] = dpset
        self.data['waiters'] = {}

        wsgi.register(QuantumSwitchController, {simple_switch_instance_name : self})

    def add_flow(self, datapath, in_port, dst, actions):
        ofproto = datapath.ofproto

        match = datapath.ofproto_parser.OFPMatch(
            in_port=in_port, dl_dst=haddr_to_bin(dst))

        mod = datapath.ofproto_parser.OFPFlowMod(
            datapath=datapath, match=match, cookie=0,
            command=ofproto.OFPFC_ADD, idle_timeout=0, hard_timeout=0,
            #priority=ofproto.OFP_DEFAULT_PRIORITY,
            priority=100,
            flags=ofproto.OFPFF_SEND_FLOW_REM, actions=actions)
        datapath.send_msg(mod)


class QuantumSwitchController(ControllerBase):

    def __init__(self, req, link, data, **config):
        super(QuantumSwitchController, self).__init__(req, link, data, **config)
        self.simpl_switch_spp = data[simple_switch_instance_name]

        #self._set_logger()
        #self.logger.info("dir(data): " + `dir(data)`)

        self.dpset = self.simpl_switch_spp.data['dpset']

        jsondict = json.load(open(qconf))
        self.dst = jsondict["dst"]
        self.channels = jsondict["channels"]

    def set_flows(self, dp, f):
        ofproto = dp.ofproto

        delete_flow_entry(dp)

        flow = {"priority":1000,
                "match":{"dl_type": 0x0800, "nw_proto": 6,"in_port":f["iport"], "nw_dst":self.dst['nw'],"tp_dst":self.dst['tp']},
                "actions":[{"type":"SET_NW_DST","nw_dst":f["nw"]},{"type":"SET_TP_DST","tp_dst":f["tp"]},
                           {"type":"SET_DL_DST","dl_dst":f["dl"]},{"type":"OUTPUT", "port":f["oport"]}]}
        mod_flow_entry(dp, flow, ofproto.OFPFC_ADD)

        flow = {"priority":1000,
                "match":{"dl_type": 0x0800, "nw_proto": 6,"in_port":f["oport"],"nw_src":f["nw"],"tp_src":f["tp"]},
                "actions":[{"type":"SET_NW_SRC","nw_src":self.dst['nw']},{"type":"SET_TP_SRC","tp_src":self.dst['tp']},
                           {"type":"SET_DL_SRC","dl_src":f["dl"]},{"type":"OUTPUT", "port":f["iport"]}]}
        mod_flow_entry(dp, flow, ofproto.OFPFC_ADD)

    @route('qkey', '/qkey/{channel}', methods=['POST'],
           requirements={'channel': r'[0-9]'})
    def post_handler(self, req, **kwargs):

        # In JSON file channel number is the key (string)
        channel = kwargs['channel']
        c = self.channels[channel]
        dp = self.dpset.get(c['dpid'])

        qkey = req.body
        addr = req.remote_addr
        
        body = json.dumps({'channel': channel,
                           'qkey': qkey,
                           'addr': addr})

        body = "ADDR: %s QKEY: %s\n" % (addr, qkey)
        for url in c["qcrypt"]["key_poins"]:
            body += "URL %s RESULT:\n" % url
            try:
                r = requests.post(url, qkey)
                body += r.text
            except: #requests.ConnectionError:
                body += "URL %s: can't connect\n" % url
        return Response(content_type='application/json', body=body)

    @route('conf', '/conf', methods=['GET'])
    def get_conf_handler(self, req, **kwargs):

        body = json.dumps({'dst': self.dst,
                           'channels': self.channels})

        return Response(content_type='application/json', body=body)

    @route('conf', '/conf', methods=['POST'])
    def set_conf_handler(self, req, **kwargs):
        body = json.dumps({'dst': self.dst,
                           'channels': self.channels})

        jsondict = json.loads(req.body)
        self.dst = jsondict["dst"]
        self.channels = jsondict["channels"]

        f = open(qconf, "w+")
        f.write(json.dumps({'dst': self.dst,
                            'channels': self.channels}))
        f.close()
        return Response(content_type='application/json', body=body)

    @route('qchannel', '/qchannel/{channel}/{status}', methods=['GET'], requirements={'channel': r'[0-9]', 'status': r'[0-6]'})
    def get_handler(self, req, **kwargs):
        # In JSON file channel number is the key (string)
        channel = kwargs['channel']

        status = int(kwargs['status'])
        result = -1
        
        c = self.channels[channel]
        dp = self.dpset.get(c['dpid'])

        if status == 0:
            result = c["scrypt"]
            self.set_flows(dp, result) 
        elif status == 1:
            result = c["qcrypt"]
            self.set_flows(dp, result)
        elif status == 2:
            result = 'REREAD' # reread quantum key file
        elif status == 3:
            result = c["transp"]
            self.set_flows(dp, result)
        elif status == 4:
            result = 'DELETE'
            delete_flow_entry(dp)
        elif status == 5:
            result = os.system(qpath + '/bin/crypto_stat eth1 eth2')
        elif status == 6:
            p = os.popen(qpath + '/bin/crypto_stat_get eth1 eth2')
            result = p.readlines()

        body = json.dumps({'channel': channel, 'status': status, 'result': result})

        return Response(content_type='application/json', body=body)

