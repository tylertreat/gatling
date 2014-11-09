import socket
import struct


DEFAULT_HOST = 'localhost'
DEFAULT_PORT = 9999

SUBSCRIBE = 0
UNSUBSCRIBE = 1
PUBLISH = 2


class Client(object):

    def __init__(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self, host=DEFAULT_HOST, port=DEFAULT_PORT):
        self.socket.connect((host, port))

    def disconnect(self):
        self.socket.close()

    def subscribe(self, topic):
        data = struct.pack('!Hi%ds' % len(topic), SUBSCRIBE, len(topic), topic)
        self.socket.send(data)

    def unsubscribe(self, topic):
        data = struct.pack('!Hi%ds' % len(topic), UNSUBSCRIBE,
                           len(topic), topic)
        self.socket.send(data)

    def publish(self, topic, msg):
        size = len(topic) + len(msg) + 4
        data = struct.pack('!Hii%ds%ds' % (len(topic), len(msg)), PUBLISH,
                           size, len(topic), topic, msg)
        self.socket.send(data)

    def recv(self):
        proto, msg_len, topic_len = struct.unpack('!Hii', self.socket.recv(10))
        if proto != PUBLISH:
            raise Exception('Invalid protocol frame')

        body_len = msg_len - topic_len - 4
        return struct.unpack('!%ds%ds' % (topic_len, body_len),
                             self.socket.recv(topic_len + body_len))

