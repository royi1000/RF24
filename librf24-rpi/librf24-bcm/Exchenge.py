import rf_prot
import shelve
import time
from exceptions import *

def seconds(): return int(round(time.time() * 1000000))


class App(object):
    APP_NAME = 'undef'
    def __init__(self, interval=60):
        self.last_time = 0
        self.interval = interval
        self.last_data = ''

    def get_data(self):
        self.last_time = seconds()
        raise NotImplemented

    def should_run(self):
        if self.last_time + self.interval > seconds():
            return False
        return True

    def valid(self):
        if (self.last_time + (self.interval*10)) < seconds():            
            return False
        return True



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
        rx_addrs = [0xF0F0F0F01F, 0xF0F0F0F02F, 0xF0F0F0F03F, 0xF0F0F0F04F, 0xF0F0F0F05F]
        self._rf = rf_prot.RF24_Wrapper(rx_addrs=rx_addrs)
        self._db = shelve.open('rf_exchange_db')
        self.apps = apps
        if not self._db.has_key('init'):
            self._db['init'] = True
            self._db['current_pipe'] = 0
            self._db['in_devices'] = []
            self._db['out_devices'] = []
            self._db['app_data'] = {}
            self._db['sensors'] = {}

    def run(self):
        try:
            while True:
                self._rf.read(5000)
                changed = []
                for app in self.apps:
                    if app.should_run():
                        self._db['app_data'][app.APP_NAME] = app.get_data()
                        changed.append(app.APP_NAME)
                for _id, sensor in self._db['sensors'].items():
                    if sensor.is_new_data:
                        changed.append(_id)
                        
        finally:
            self._db.close()
