import rf_prot
import shelve
import time
import struct
import urllib2
from datetime import datetime
from xml.dom.minidom import parse
from exceptions import *

def enum(**enums):
    return type('Enum', (), enums)

COMMAND_TYPE = enum(device_init=0xf0, device_init_response=0xf1, sensor_data=0x10, screen_data=0x11)
DEVICE_TYPE = enum(screen=0x10, sensor=0x20)
DATA_TYPE = enum(date=0x1, string=0x2, bitmap=0x3, color_string=0x4, remove_id=0x10, end_tx=0x20)
COLOR_TYPE = enum(c_red=1, c_green=2, c_blue=3,c_purple=4,c_yellow=5,c_aqua=6)

colors = {'red':(200,0,0),
          'green':(0,200,0),
          'blue':(0,0,200),
          'purple':(200,0,200),
          'aqua':(0,200,200),
          'yellow':(200,200,0),
          'orange':(200,100,0),
         }

def set_rgb(red1,green1,blue1,red2,green2,blue2):
    return chr(red1)+chr(green1)+chr(blue1)+chr(red2)+chr(green2)+chr(blue2)

def seconds(): return int(round(time.time()))

class App(object):
    APP_NAME = 'undef'
    def __init__(self, interval=6):
        self.last_time = 0
        self.interval = interval
        self.last_data = ''

    def update_data(self, _id):
        self.last_time = seconds()
                    
    def get_data(self, _id):
        return self.data

    def should_run(self):
        if self.last_time + self.interval > seconds():
            return False
        return True

    def valid(self):
        if (self.last_time + (self.interval*10)) < seconds():
            return False
        return True

class DT(App):
    APP_NAME = 'time'
    def valid(self):
        return True

    def get_data(self, _id):
        self.update_data(_id)
        return self.data
        
    def update_data(self, _id):
        self.last_time = seconds()
        dt = datetime.now().timetuple()[0:6]
        self.data = chr(DATA_TYPE.date) + struct.pack('HBBBBB', *dt)
        
class ShortTest(App):
    APP_NAME='shorttest'
    def valid(self):
        return True
    
    def get_data(self, _id):
        return chr(DATA_TYPE.color_string) + chr(_id) + set_rgb(*(colors['red'] + colors['orange'])) +  'short test'
    
class LongTest(App):
    APP_NAME='longtest'
    def valid(self):
        return True
        
    def get_data(self,_id):
        return  chr(DATA_TYPE.color_string) + chr(_id) +  set_rgb(*(colors['purple'] + colors['aqua'])) +  'long test ' * 4

class Gmail(App):
    APP_NAME='gmail'
    def update_data(self, _id):
        user,passwd = open('gmail.txt').readline().split(',')
        self.count = self.get_unread_msgs(user,passwd)
        self.last_time = seconds()


    def get_data(self,_id):
        if self.count:
            return chr(DATA_TYPE.color_string) + chr(_id) +  set_rgb(*(colors['blue'] + colors['green'])) +  'Gmail unread{}'.format(self.count)
        return ''
            
    def valid(self):
        return self.count
        
    def get_unread_msgs(self, user, passwd):
        auth_handler = urllib2.HTTPBasicAuthHandler()
        auth_handler.add_password(
        realm='New mail feed',
        uri='https://mail.google.com',
        user='%s@gmail.com' % user,
        passwd=passwd)
        opener = urllib2.build_opener(auth_handler)
        urllib2.install_opener(opener)
        feed = urllib2.urlopen('https://mail.google.com/mail/feed/atom')
        dom=parse(feed)
        count_obj = dom.getElementsByTagName("fullcount")[0]
        # get its text and convert it to an integer
        return int(count_obj.firstChild.wholeText)

class Sensor(object):
    REP_STR = "Sensor data: {}"
    def __init__(self, _id):
        self._id = _id
        self.last_data = ''
        self.last_time = 0
        self.new_data = False

    def update(self, data):
        self.last_data = data
        self.last_time = seconds()
        self.new_data = True

    def get_data(self):
        REP_STR.format(*self.last_data)

    def is_new_data(self):
        if self.new_data:
            self.new_data = False
            return True
        return False

class RFExchange(object):
    def __init__(self, apps):
        #rx_addrs = [0xF0F0F0F01F, 0xF0F0F0F02F, 0xF0F0F0F03F, 0xF0F0F0F04F, 0xF0F0F0F05F]
        self._rf = rf_prot.RF24_Wrapper()
        self._db = shelve.open('rf_exchange_db')
        self.apps = apps
        if not self._db.has_key('init'):
            self._db['init'] = True
            self._db['current_pipe'] = 0
            self._db['in_devices'] = []
            self._db['out_devices'] = []
            self._db['app_data'] = {}
            self._db['sensors'] = {}

    def handle_rx_data(self, data):
        cmd = struct.unpack('B', data[0])[0]
        if cmd == COMMAND_TYPE.device_init:
            dev_type = struct.unpack('B', data[1])[0]
            addr = struct.unpack('Q', data[2:10])[0]
            print "got device init cmd: {}, dev type: {}, addr: {}".format(hex(cmd), hex(dev_type), hex(addr))
            if dev_type==DEVICE_TYPE.screen:
                if not addr in self._db['out_devices']:
                    print repr(addr)
                    x=self._db['out_devices']
                    x.append(addr)
                    self._db['out_devices'] = x
                data=chr(COMMAND_TYPE.device_init_response)+data[1:]
                self._rf.write(addr, data[:10])
                for _id, app in enumerate(self.apps):
                    if app.valid():
                        data = app.get_data(_id)
                        print repr(data)
                        self._rf.write(addr, data)
                    else:
                        data = chr(DATA_TYPE.remove_id) + chr(_id)
                        self._rf.write(addr, data)
                self.send_end_tx_msg(addr)

    def send_end_tx_msg(self, addr):
        data = chr(DATA_TYPE.end_tx)
        self._rf.write(addr, data)
                        
    def run(self):
        try:
            while True:
                for _id, app in enumerate(self.apps):
                    if app.should_run():
                        app.update_data(_id)
                for _id, sensor in self._db['sensors'].items():
                    if sensor.is_new_data:
                        changed.append(_id)
                (pipe, data) = self._rf.read(5000)
                if data:
                    self.handle_rx_data(data)
                
        finally:
            self._db.close()


RFExchange([DT(),LongTest(),ShortTest(),Gmail()]).run()
