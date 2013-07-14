#include <avr/eeprom.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Time.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "led.h"
#include "lcd.h"
#include "sound.h"

#define PAYLOAD_SIZE 20
#define COMMAND_INIT 0xF0
#define COMMAND_INIT_RESPONSE 0xF1
#define DEVICE_TYPE 0x10
#define MAX_STR_LEN (12*6)
#define MAX_SCREENS 10
#define SCREEN_TIME 4000 //time to view screen in milli
#define UPDATE_INTERVAL 10000
#define DEFAULT_COLOR c_blue
const uint16_t MAGIC_CODE = 0xDE12;
// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
uint64_t pipes[2] = {0xF0F0F0F0D2LL,  0xF0F0F0F0E1LL};

typedef enum {stand_alone=0, first_packet=1, continued_packet=2, last_packet=3} packet_type_e;
typedef enum{date=0x1, string=0x2, bitmap=0x3, color_string=0x4, sound=0x5, remove_id=0x10, end_tx=0x20} data_type_e;
typedef enum{c_red=1, c_green=2, c_blue=3, c_purple=4, c_yellow=5, c_aqua=6} color_type_e;

typedef struct led_s{
    uint8_t red1;
    uint8_t green1;
    uint8_t blue1;
    uint8_t red2;
    uint8_t green2;
    uint8_t blue2;
}led_t;

uint8_t radio_buf[32];
uint8_t data[100];
uint8_t screens[MAX_SCREENS][MAX_STR_LEN+1];
uint8_t screens_size[MAX_SCREENS];
led_t screens_color[MAX_SCREENS];
uint8_t sound_data[100];
unsigned int data_len = 0;
unsigned int current_ptr = 0;
unsigned int last_screen_id = 0;
unsigned long last_screen_time = 0;
unsigned long last_update_time = 0;
unsigned int sound_len = 0;
uint8_t current_color = c_blue;
bool end_transmission = false;

typedef struct store {
    uint16_t magic_code;
    uint64_t rx_addr;
    uint64_t tx_addr;
} store_t;

typedef struct cmd_message {
    uint8_t  command;
    uint8_t dev_type;
    uint64_t addr;
} cmd_message_t;

typedef struct date_cmd {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} date_cmd_t;
store_t config_settings;

void color_out(){
  if(!last_screen_id)
      return;
      fade(screens_color[last_screen_id-1].red1,
                screens_color[last_screen_id-1].green1,
                screens_color[last_screen_id-1].blue1,
                screens_color[last_screen_id-1].red2,
                screens_color[last_screen_id-1].green2,
                screens_color[last_screen_id-1].blue2,
                5);
      fade(screens_color[last_screen_id-1].red2,
                screens_color[last_screen_id-1].green2,
                screens_color[last_screen_id-1].blue2,
                screens_color[last_screen_id-1].red1,
                screens_color[last_screen_id-1].green1,
                screens_color[last_screen_id-1].blue1,
                5);

}

void digitalClockDisplay(){
    LcdClear();
    drawLine();
    gotoXY(14,1);
    LcdString(dayStr(weekday()));
    gotoXY(22,3);
    printDigits(hour(), 2, ':');
    printDigits(minute(),2,0);
    gotoXY(7,4);
    printDigits(day(),2,'/');
    printDigits(month(),2,'/');
    printDigits(year(),4,0);
}

void printDigits(unsigned long digits, int num_of_digits, char delimiter){
    // utility function for digital clock display: prints preceding colon and leading 0
    for(int i=num_of_digits-1;i>=0;i--) {
        int d= (digits/pow(10,i));
        d=d%10;
        //printf( "digit: %d, d:%d, i:%d ddd: %d\n", digits, d,i,pow(10,i));
        LcdCharacter('0'+d);
    }
    if(delimiter)
        LcdCharacter(delimiter);

}

bool is_timeouted(unsigned long last, unsigned long timeout_interval /*millis*/)
{
    if (((millis() - last) < timeout_interval && (last < millis())))
        return false;
    return true;
}


void handle_message() {
    uint8_t cmd = data[0];
    if(cmd==COMMAND_INIT_RESPONSE) {
        if(config_settings.magic_code != MAGIC_CODE) {
            config_settings.magic_code = MAGIC_CODE;
            eeprom_write_block((const void*)&config_settings, (void*)0, sizeof(config_settings));
        }
        last_screen_time = 0;
    }
    else if (cmd==date) {
        date_cmd_t* dct = (date_cmd_t*)(data+1) ;
        setTime(dct->hour,dct->minute,dct->second,dct->day,dct->month,dct->year);
    }
    else if (cmd==string) {
        uint8_t id = data[1];
        if ((id>10) || ((data_len - 2) > MAX_STR_LEN)) {
            //support only 10 id's
            data_len = 0;
            current_ptr = 0;
            return;
        }
        memcpy(screens[id-1], data+2, data_len-2);
        screens_size[id-1] = data_len - 2;
        screens[id-1][data_len-2] = 0; //verify null termination
        memset(&screens_color[id-1], 0, sizeof(led_t)) ;
    }
    else if (cmd==sound) {
        memcpy(sound_data, data+1, data_len-1);
        sound_len = data_len - 1;
    }
    else if (cmd==color_string) {
        uint8_t id = data[1];
        if ((id>10) || ((data_len - 3) > MAX_STR_LEN)) {
            //support only 10 id's
            data_len = 0;
            current_ptr = 0;
            return;
        }
        memcpy(screens[id-1], data+2+sizeof(led_t), data_len-2-sizeof(led_t));
        screens_size[id-1] = data_len - 3;
        memcpy(&screens_color[id-1], data+2, sizeof(led_t));
        screens[id-1][data_len-2-sizeof(led_t)] = 0; //verify null termination
    }
    else if (cmd==remove_id) {
        uint8_t id = data[1];
        if(id>0 && id <= 10){
            screens_size[id-1] = 0;
        }
    }
    else if (cmd==end_tx) {
        end_transmission = true;
    }
    data_len = 0;
    current_ptr = 0;
}

void handle_read(uint8_t* buf, unsigned int len){
    uint8_t msg_type = buf[0] >> 6;
    uint8_t msg_len = buf[0] & 63;
    if(stand_alone==msg_type){
        memcpy(data, buf+1, msg_len-1);
        data_len = msg_len-1;
        handle_message();
        data_len = 0;
        current_ptr = 0;
        return;
    }
    else if(first_packet==msg_type) {
        data_len = *((uint16_t*)(buf+1));
        memcpy(data, buf+3, msg_len-3);
        current_ptr = msg_len-3;
        //printf("got first message, total len: %d\n", data_len);
        return;
    }
    else if(continued_packet==msg_type) {
        if(!current_ptr) {
            //printf("got invalid continued message\n");
        }
        memcpy(data+current_ptr, buf+1, msg_len-1);
        current_ptr += msg_len-1;
        if(current_ptr > data_len) {
            // printf("invalid continued message, ptr overflow\n");
        }
        return;
    }
    else if(last_packet==msg_type) {
        if(!current_ptr) {
            //printf("got invalid last message\n");
            return;
        }
        memcpy(data+current_ptr, buf+1, msg_len-1);
        current_ptr += msg_len-1;
        if(current_ptr != data_len) {
            //printf("invalid ptr: %d, data len: %d\n", current_ptr, data_len);
            return;
        }
        handle_message();
        data_len = 0;
        current_ptr = 0;
    }
}

void handle_write(void* buf, unsigned int len){
    uint8_t tx_buf[32];
    if(len<radio.getPayloadSize()) {
        tx_buf[0] = (stand_alone << 6) | (len+1);
        memcpy(tx_buf+1, buf, len);
        //printf("sending message, length: %d content:\n", len);
        for(int i=0;i<len+1;i++) {
            //printf("0x%X ", tx_buf[i]);
        }
        //printf("\n");
        radio.stopListening();
        int success = radio.write(tx_buf, len+1);
        radio.startListening();
        if(success) {
            // printf("send single packet succeded\n");
        }
        else{
            //printf("send single packet failed\n");
        }
        return;
    }
}


void send_init_packet() {
    cmd_message_t message;
    message.command = COMMAND_INIT;
    message.dev_type = DEVICE_TYPE;
    message.addr = config_settings.rx_addr;
    radio.stopListening();
    handle_write((void *)&message, sizeof(message));
    radio.startListening();
}

void setup()
{
    Serial.begin(9600);
    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A2, OUTPUT);
    digitalWrite(A0, HIGH);
    digitalWrite(A2, HIGH);
    digitalWrite(A1, HIGH);
    LcdInitialise();
    LcdClear();
    //printf_begin();
    //printf("\n\rNokia screen receiver\n\r");
    randomSeed(analogRead(0));
    eeprom_read_block((void*) &config_settings, (void*) 0, sizeof(config_settings));
    // Setup and configure rf radio
    if (0xDE12 != config_settings.magic_code) {
        config_settings.tx_addr = 0xF0F0F0F0E1LL;//0xF0F0F00000LL | random(0xFFFFLL);
        config_settings.rx_addr = 0xF0F0000000LL | random(0xFFFFFFLL);
    }
    radio.begin();


    // optionally, increase the delay between retries & # of retries
    radio.setRetries(15,15);

    //        radio.enableDynamicPayloads();
    radio.setPayloadSize(PAYLOAD_SIZE);

    radio.openWritingPipe(config_settings.tx_addr);
    radio.openReadingPipe(1, config_settings.rx_addr);

    radio.startListening();
    radio.printDetails();

}

void next_screen()
{
    if (!is_timeouted(last_screen_time, SCREEN_TIME)) {
        return;
    }

    last_screen_time = millis();
    last_screen_id=(last_screen_id+1) % (MAX_SCREENS+1);
        LcdClear();
    while(last_screen_id && !screens_size[last_screen_id-1]) {
        last_screen_id=(last_screen_id+1) % (MAX_SCREENS+1);
    }
    if(!last_screen_id){
        LcdClear();
        digitalClockDisplay();
        current_color = c_blue;
        return;
    }
    LcdClear();
    LcdString((char*)screens[last_screen_id-1]);
}

void get_data_from_master()
{
    if(!is_timeouted(last_update_time, UPDATE_INTERVAL) || last_screen_id)
    {
        return;
    }
    last_update_time = millis();
    send_init_packet();
    bool timeout = false;
    end_transmission = false;
    while(!timeout && !end_transmission) {
        if (is_timeouted(last_update_time, 1000)) {
            timeout = true;
        }
        else if(radio.available()){
            uint8_t payload_size = radio.getPayloadSize();
            radio.read(radio_buf, payload_size);
            handle_read(radio_buf, payload_size);
        }
    }
    if(sound_len) {
        play_tones(sound_data, sound_len);
        sound_len = 0;
    }
}

void loop()
{
    get_data_from_master();
    color_out();
    next_screen();
}
