from rf24 import *
import struct
from exceptions import *

_DEBUG=True

class Byte(object):
    def __init__(self, size):
        self.ptr=new_byteArray(size+32)
        self.size=size
    def __del__(self):
        delete_byteArray(self.ptr)
    def write(self, iteritem):
        if len(iteritem)>self.size:
            raise IndexError('Item size: {} bigger than array size: {}'.format(len(iteritem), self.size))
        for i in range(len(iteritem)):
            byteArray_setitem(self.ptr, i, ord(iteritem[i]))
    def read(self, size=0):
        if not size:
            size = self.size
        if size>self.size:
            raise IndexError('Item size: {} bigger than array size: {}'.format(len(iteritem), self.size))
        res = ''
        for i in range(size):
            res += chr(byteArray_getitem(self.ptr, i))
        return res

def millis(): return int(round(time.time() * 1000))%0xffffffff

class RF24_Wrapper:
    def __init__(self, tx_addr=0xF0F0F0F0E1, rx_addrs=[0xF0F0F0F0D2],
                 channel=0x4c, level=RF24_PA_HIGH)
        self.radio=RF24(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_8MHZ)
        self.radio.begin()
        self.debug=_DEBUG
        self.radio.setRetries(15,15)
        self.radio.setChannel(channel)
        self.radio.setPALevel(level)
        self.radio.enableDynamicPayloads()
        self.radio.enableAckPayload()
        self.setPayloadSize(8)
        self.radio.setAutoAck(True)
        self.channel = channel
        self.level = level
        self.tx_addr = tx_addr
        self.rx_addr = rx_addr
        self._buffer = Byte(32)
        self._pipe_num = Byte(1)
        self.rx_len = len(rx_addrs)
        if self.rx_len>5:
            raise IndexError('too much rx addresses (max 5)')
        for i in range(self.rx_len):
            if (0xFFFFFFFF00 & rx_addrs[0]) != (0xFFFFFFFF00 & rx_addrs[i]):
                raise ValueError('only last adddress byte must be diffrent to all rx address')
            self.radio.openReadingPipe(i+1, rx_addrs[i])
        print "PRD (noise on channel) {}".format(self.radio.testRPD())
        self.startListening()
        
    def read(time=1000):
        pipes=['','','','','','']
        start=millis()
        while millis()<start+time:
            if self.radio.available(self._pipe_num.ptr):
                pipe = struct.unpack('B',self._pipe_num.read(1))
                last_size = self.radio.getDynamicPayloadSize()
                

