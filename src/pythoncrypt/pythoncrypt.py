#!/usr/bin/env python
import socket
import select
import sys
import threading
import struct
from ConfigParser import ConfigParser
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from nifpga import Session

timeout_ms = 300

class Codec(object):
    def __init__(self, key_manager, first_slot, is_fpga):
        self.key_manager = key_manager
        self.is_fpga = is_fpga
        if self.is_fpga:
            self.session = Session("real.lvbitx", "RIO0") #Session("real.lvbitx", "PXI1Slot2")
            if(first_slot):
                self.key = self.session.registers['Key1']
                self.input = self.session.registers['Input1']
                self.output = self.session.registers['Output1']
                self.irq = 1
            else:
                self.key = self.session.registers['Key2']
                self.input = self.session.registers['Input2']
                self.output = self.session.registers['Output2']
                self.irq = 2

    def xor(self, v1, v2):
        result = []
        if self.is_fpga:
            self.key.write(v1)
            self.input.write(v2)
            irq_status = self.session.wait_on_irqs(self.irq, timeout_ms)
            if self.irq in irq_status.irqs_asserted:
                data = self.output.read()
                return data
        else:
            for x in range(0, len(v1)):
                result.append(v1[x] ^ v2[x])
            return result
        return None

    def encode(self, data):
        result = b""
        key = self.key_manager.current_key()
        if key is None:
            return None
        if len(data) > self.key_manager.buffer_size:
            print "MORE"
        while len(data) > 0:
            count = self.key_manager.block_size if len(data) > self.key_manager.block_size else len(data)
            if count + key.curpos > key.size:
                key = self.key_manager.next_key()
            if count > key.size:
                count = key.size
            for x in range(0, 8):
                result += struct.pack('B', key.num[x])
            result += struct.pack('I', key.curpos)
            result += struct.pack('I', count)
            towork = [ord(i) for i in data[:count]]
            tokey = [key.key[i] for i in range(key.curpos, key.curpos+count)]
            if count < self.key_manager.block_size:
                for n in range(count, self.key_manager.block_size):
                    towork.append(0)
                    tokey.append(0)
            for x in self.xor(towork, tokey)[:count]:
                result += struct.pack('B', x)
            #for x in range(0, count):
                #print ord(data[x])
                #print key.key[key.curpos+x]
                #result += struct.pack('B', self.xor(ord(data[x]), key.key[key.curpos+x]))
            key.curpos += count
            data = data[count:]
        return result

    def decode(self, data):
        result = b""
        errors = 0
        while len(data) > 16:
            #print ""
            #print len(data)
            num = data[0:8]
            curpos, = struct.unpack('I', data[8:12])
            count, = struct.unpack('I', data[12:16])
            key = self.key_manager.get_key_by_num(num)
            if key is not None:
                if count > len(data) - 16:
                    errors = 2
                    break
                towork = [struct.unpack('B', data[i+16])[0] for i in range(0, count)]
                tokey = [key.key[i] for i in range(curpos, curpos+count)]
                if count < self.key_manager.block_size:
                    for n in range(count, self.key_manager.block_size):
                        towork.append(0)
                        tokey.append(0)
                for x in self.xor(towork, tokey)[:count]:
                    result += chr(x)
                #for x in range(0, count):
                    #val, = struct.unpack('B', data[x+16])
                    #result += chr(self.xor(val, key.key[curpos+x]))
                    #print chr(val ^ key.key[curpos+x])
                    #print key.key[curpos+x]
                data = data[count+16:]
            else:
                errors = 1
                data = b''
                break
        return {'result': result,'diff': len(data),'errors': errors}


class Key(object):
    def __init__(self, data):
        self.ready = False
        self.size = len(data)
        data = bytearray(data,'ascii')
        self.key = data
        self.num = self.key[0:8]
        self.curpos = 0
        self.ready = True

class KeyManager(object):
    def __init__(self, packet_size):
        self.buffer_size = 2048
        self.block_size = packet_size
        self.keys = []
        self.cur_key = -1
        #self.count = 0

    def read_keys(self, work_mode, array):
        #self.keys = []
        #self.count = 0
        if len(array) > self.block_size:
            self.cur_key = 0
        if work_mode == 0:
            while len(array) > self.block_size:
                self.keys.append(Key(array[:self.block_size]))
                array = array[self.block_size:]
                #self.count += 1

    def print_keys(self):
        for key in self.keys:
            print key.size
            print [hex(i) for i in key.num]
            print [hex(i) for i in key.key]

    def current_key(self):
        if self.cur_key > -1:
            if self.cur_key > len(self.keys):
                self.cur_key = 0
            return self.keys[self.cur_key]
        return None

    def next_key(self):
        if self.cur_key > -1:
            self.keys[self.cur_key].curpos = 0
            self.cur_key += 1
            kl = len(self.keys)
            if kl > 20:
                self.keys = self.keys[10:]
            if self.cur_key >= len(self.keys):
                if len(self.keys) > 15:
                    self.cur_key = 10
                else:
                    self.cur_key = 0
            if self.keys[self.cur_key].ready:
                self.keys[self.cur_key].curpos = 0
                return self.keys[self.cur_key]
            else:
                return self.next_key()
        return None

    def get_key_by_num(self, num):
        for key in self.keys:
            if key.num == num:
                return key
        return None


class RequestHandler(BaseHTTPRequestHandler):

    key_manager = None

    def do_GET(self):
        request_path = self.path

        print("\n----- Request Start ----->\n")
        print(request_path)
        print(self.headers)
        print("<----- Request End -----\n")

        self.send_response(200)
        self.send_header("Set-Cookie", "foo=bar")

    def do_POST(self):
        request_path = self.path

        print("\n----- Request Start ----->\n")
        print(request_path)

        request_headers = self.headers
        content_length = request_headers.getheaders('content-length')
        length = int(content_length[0]) if content_length else 0

        print(request_headers)
        data = self.rfile.read(length)
        print(data)
        print(len(data))
        self.key_manager.read_keys(0, data.strip())
        print("<----- Request End -----\n")

        self.send_response(200)
        self.key_manager.print_keys()

    do_PUT = do_POST
    do_DELETE = do_GET


class Control(object):
    def __init__(self, control_ip, control_port, key_manager):
        self.control_ip = control_ip
        self.control_port = control_port
        RequestHandler.key_manager = key_manager
        self.server = HTTPServer((self.control_ip, self.control_port), RequestHandler)
        self.thread = threading.Thread(target=self.server.serve_forever)
        self.thread.daemon = True

    def main_loop(self):
        try:
            self.thread.start()
        except KeyboardInterrupt:
            self.server.shutdown()


class Proxy(object):
    """Accepts connections on listen_address and forwards them to
    target_address
    """

    def __init__(self, listen_address, target_address, codec, work_mode, is_coder):
        self.target_address = target_address
        self.channels = {}  # associate client <-> target
        self.connections = {}  # 2nd dict for sockets to be indexed by fileno
        self.buffers = {}
        self.proxy_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.proxy_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.proxy_socket.bind(listen_address)
        self.proxy_socket.listen(1)
        self.proxy_socket.setblocking(0)

        self.work_mode = work_mode

        self.is_coder = is_coder

        self.inputs = []

        self.codec = codec

        self.epoll = select.epoll()
        self.epoll.register(self.proxy_socket.fileno(), select.EPOLLIN)

        self.running = False

    def accept_new_client(self):
        """Try to connect to the target and when this succeeds, accept the
        client connection.  Keep track of both and their connections
        """
        target = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            target.connect(self.target_address)
        except socket.error as serr:
            print("Error: " + str(serr))
            target = False

        connection, address = self.proxy_socket.accept()
        if target:

            if self.is_coder:
                self.inputs.append(connection.fileno())
            else:
                self.inputs.append(target.fileno())

            connection.setblocking(0)
            self.epoll.register(connection.fileno(), select.EPOLLIN)
            self.epoll.register(target.fileno(), select.EPOLLIN)

            # save the sockets for the target and client
            self.connections[connection.fileno()] = connection
            self.connections[target.fileno()] = target

            # save the sockets but point them to each other
            self.channels[connection.fileno()] = target
            self.channels[target.fileno()] = connection

            self.buffers[connection.fileno()] = b''
            self.buffers[target.fileno()] = b''
        #else:
            #connection.send(bytes("Can't connect to {}\n".format(address), 'UTF-8'))
            #connection.close()

    def relay_data(self, fileno):
        """Receive a chunk of data on fileno and relay that data to its
        corresponding target socket. Spill over to buffers what can't be sent
        """
        try:
            data = self.connections[fileno].recv(self.codec.key_manager.buffer_size)
        except:
            print "W0"
            data = b''
            self.epoll.modify(fileno, 0)
            try:
                self.channels[fileno].shutdown(socket.SHUT_RDWR)
            except:
                pass
            pass
        if len(data) == 0:
            # peer closed connection, shut down socket and
            # wait for EPOLLHUP to clean internal structures
            self.epoll.modify(fileno, 0)
            try:
                self.connections[fileno].shutdown(socket.SHUT_RDWR)
            except:
                print "W1"
                pass
        else:
            # if there's already something in the send queue, add to it
            if (len(self.buffers[fileno]) > 0) & (not (fileno in self.inputs)):
                data = self.buffers[fileno] + data
            #else:
            try:
                    #print "relay_data"
                    #print len(data)
                    #print [hex(ord(i)) for i in data]
                    #print data
                    #result = self.codec.encode(data)
                    #print [hex(ord(i)) for i in result]
                    #print self.codec.decode(result)
                    #if len(data) > self.codec.key_manager.block_size:
                    #print len(data)
                    if fileno in self.inputs:
                        data = self.codec.encode(data)
                        try:
                            byteswritten = self.channels[fileno].send(data)
                        except:
                            print "W2"
                            self.connections[fileno].shutdown(socket.SHUT_RDWR)
                            pass
                            #if len(data) > byteswritten:
                                #self.buffers[fileno] = data[byteswritten:]
                                #self.epoll.modify(fileno, select.EPOLLOUT)
                    else:
                            result = self.codec.decode(data)
                            if result['diff'] > 0:
                                self.buffers[fileno] = data[len(data)-result['diff']:]
                            else:
                                self.buffers[fileno] = b''
                            #print "DIFF"
                            #print result['diff']
                            #print result['errors']
                            self.channels[fileno].send(result['result'])

                    #else:
                    #    self.epoll.modify(fileno, select.EPOLLIN)
            except socket.error:
                    #self.connections[fileno].send(
                    #    bytes("Can't reach server\n", 'UTF-8'))
                    self.epoll.modify(fileno, 0)
                    try:
                        self.connections[fileno].shutdown(socket.SHUT_RDWR)
                    except:
                        print "W3"
                        pass

    def send_buffer(self, fileno):
        print "send_buffer"
        print len(self.buffers[fileno])
        if len(self.buffers[fileno]) > self.codec.key_manager.block_size:
            count = 0
            if fileno in self.inputs:
                self.buffers[fileno] = self.codec.encode(self.buffers[fileno])
                count = len(self.buffers[fileno])
            else:
                result = self.codec.decode(self.buffers[fileno])
                self.buffers[fileno] = result['result']
                count = len(self.buffers[fileno])-result['diff']
            byteswritten = self.channels[fileno].send(self.buffers[fileno])
            if len(self.buffers[fileno]) > byteswritten:
                self.buffers[fileno] = self.buffers[fileno][byteswritten:]
                self.epoll.modify(fileno, select.EPOLLOUT)
        else:
            self.epoll.modify(fileno, select.EPOLLIN)

    def close_channels(self, fileno):
        """Close the socket and its corresponding target socket and stop
                    listening for them"""
        out_fileno = self.channels[fileno].fileno()
        self.epoll.unregister(fileno)

        # close and delete both ends
        self.channels[out_fileno].close()
        self.channels[fileno].close()
        del self.channels[out_fileno]
        del self.channels[fileno]
        del self.connections[out_fileno]
        del self.connections[fileno]
        if fileno in self.inputs:
            self.inputs.remove(fileno)
        elif out_fileno in self.inputs:
            self.inputs.remove(out_fileno)

    def main_loop(self):
        self.running = True
        try:
            while self.running:
                events = self.epoll.poll(1)
                for fileno, event in events:
                    if fileno == self.proxy_socket.fileno():
                        self.accept_new_client()
                    elif event & select.EPOLLIN:
                        self.relay_data(fileno=fileno)
                    elif event & select.EPOLLOUT:
                        self.send_buffer(fileno=fileno)
                    elif event & (select.EPOLLERR | select.EPOLLHUP):
                        self.close_channels(fileno=fileno)
                        break
        finally:
            self.epoll.unregister(self.proxy_socket.fileno())
            self.epoll.close()
            self.proxy_socket.shutdown(socket.SHUT_RDWR)
            self.proxy_socket.close()


if __name__ == '__main__':

    print len(sys.argv)

    if len(sys.argv) == 1:
        print "Use with cfg file in cmd line"
        sys.exit(1)


    parser = ConfigParser()
    #parser.read("coder0.cfg")
    parser.read(sys.argv[1])
    mode = int(parser.get('default', 'mode'))
    coder = int(parser.get('default', 'coder'))
    port = int(parser.get('default', 'port'))
    is_fpga = int(parser.get('default', 'isFpga'));
    packet_size = int(parser.get('default', 'packetSize'));
    portCtrl = int(parser.get('default', 'portCtrl'))
    portDest = int(parser.get('default', 'portDest'))
    ip = parser.get('default', 'ip').replace('"', '')

    manager = KeyManager(packet_size=packet_size)

    server = Proxy(listen_address=('0.0.0.0', port),
                   target_address=(ip, portDest),
                   codec=Codec(key_manager=manager, first_slot=coder == 1, is_fpga=is_fpga == 1),
                   work_mode=mode,
                   is_coder=coder == 1)
    control = Control('0.0.0.0', portCtrl, manager)
    try:
        control.main_loop()
        server.main_loop()
    except KeyboardInterrupt:
        sys.exit(1)
