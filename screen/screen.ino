#include <avr/eeprom.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Time.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

/*
This Code has extra features 
including a XY positioning function on Display
and a Line Draw function on Nokia 3310 LCD 
It is modded from the original 
http://playground.arduino.cc/Code/PCD8544
*/
// Mods by Jim Park 
// jim(^dOt^)buzz(^aT^)gmail(^dOt^)com
// hope it works for you
#define PIN_SCE   7  // LCD CS  .... Pin 3
#define PIN_RESET 6  // LCD RST .... Pin 1
#define PIN_DC    5  // LCD Dat/Com. Pin 5
#define PIN_SDIN  4  // LCD SPIDat . Pin 6
#define PIN_SCLK  3  // LCD SPIClk . Pin 4
                     // LCD Gnd .... Pin 2
                     // LCD Vcc .... Pin 8
                     // LCD Vlcd ... Pin 7

#define LCD_C     LOW
#define LCD_D     HIGH

#define LCD_X     84
#define LCD_Y     48
#define LCD_CMD   0

int a = 0;

static const byte ASCII[][5] =
{
 {0x00, 0x00, 0x00, 0x00, 0x00} // 20  
,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c ¥
,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j 
,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e ←
,{0x00, 0x06, 0x09, 0x09, 0x06} // 7f →
};




void LcdCharacter(char character)
{
  LcdWrite(LCD_D, 0x00);
  for (int index = 0; index < 5; index++)
  {
    LcdWrite(LCD_D, ASCII[character - 0x20][index]);
  }
  LcdWrite(LCD_D, 0x00);
}

void LcdClear(void)
{
  for (int index = 0; index < LCD_X * LCD_Y / 8; index++)
  {
    LcdWrite(LCD_D, 0x00);
  }
          gotoXY (0,0);
}

void LcdInitialise(void)
{
  pinMode(PIN_SCE,   OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC,    OUTPUT);
  pinMode(PIN_SDIN,  OUTPUT);
  pinMode(PIN_SCLK,  OUTPUT);

  digitalWrite(PIN_RESET, LOW);
 // delay(1);
  digitalWrite(PIN_RESET, HIGH);

  LcdWrite( LCD_CMD, 0x21 );  // LCD Extended Commands.
  LcdWrite( LCD_CMD, 0xBf );  // Set LCD Vop (Contrast). //B1
  LcdWrite( LCD_CMD, 0x04 );  // Set Temp coefficent. //0x04
  LcdWrite( LCD_CMD, 0x14 );  // LCD bias mode 1:48. //0x13
  LcdWrite( LCD_CMD, 0x0C );  // LCD in normal mode. 0x0d for inverse
  LcdWrite(LCD_C, 0x20);
  LcdWrite(LCD_C, 0x0C);
}

void LcdString(char *characters)
{
  while (*characters)
  {
    LcdCharacter(*characters++);
  }
}

void LcdWrite(byte dc, byte data)
{
  digitalWrite(PIN_DC, dc);
  digitalWrite(PIN_SCE, LOW);
  shiftOut(PIN_SDIN, PIN_SCLK, MSBFIRST, data);
  digitalWrite(PIN_SCE, HIGH);
}

// gotoXY routine to position cursor 
// x - range: 0 to 84
// y - range: 0 to 5

void gotoXY(int x, int y)
{
  LcdWrite( 0, 0x80 | x);  // Column.
  LcdWrite( 0, 0x40 | y);  // Row.  

}



void drawLine(void)
{
  unsigned char  j;  
   for(j=0; j<84; j++) // top
	{
          gotoXY (j,0);
	  LcdWrite (1,0x01);
  } 	
  for(j=0; j<84; j++) //Bottom
	{
          gotoXY (j,5);
	  LcdWrite (1,0x80);
  } 	

  for(j=0; j<6; j++) // Right
	{
          gotoXY (83,j);
	  LcdWrite (1,0xff);
  } 	
	for(j=0; j<6; j++) // Left
	{
          gotoXY (0,j);
	  LcdWrite (1,0xff);
  }

}


#define PAYLOAD_SIZE 20
#define COMMAND_INIT 0xF0
#define COMMAND_INIT_RESPONSE 0xF1
#define DEVICE_TYPE 0x10
#define MAX_STR_LEN (12*6)
#define MAX_SCREENS 10
#define SCREEN_TIME 4000 //time to view screen in mili
const uint16_t MAGIC_CODE = 0xDE12;
// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
uint64_t pipes[2] = {0xF0F0F0F0D2LL,  0xF0F0F0F0E1LL};

typedef enum {stand_alone=0, first_packet=1, continued_packet=2, last_packet=3} packet_type_e;
typedef enum{date=0x1, string=0x2, bitmap=0x3}data_type_e;

uint8_t inited = 0;
uint8_t radio_buf[32];
uint8_t data[100];
uint8_t screens[MAX_SCREENS][MAX_STR_LEN+1];
uint8_t screens_size[MAX_SCREENS];
unsigned int data_len = 0;
unsigned int current_ptr = 0;
unsigned int last_screen_id = 0;
unsigned int last_screen_time = 0;

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

void printDigits(int digits, int num_of_digits, char delimiter){
  // utility function for digital clock display: prints preceding colon and leading 0
  for(int i=num_of_digits-1;i>=0;i--) {
    int d= (digits/pow(10,i));
    d=d%10;
    printf( "digit: %d, d:%d, i:%d ddd: %d\n", digits, d,i,pow(10,i));
    LcdCharacter('0'+d);
  }
  if(delimiter)
      LcdCharacter(delimiter);
  
}
void handle_message() {
        uint8_t cmd = data[0];
        if(cmd==COMMAND_INIT_RESPONSE) {
                LcdClear();
                LcdString("Registered, waiting for data");
              if(config_settings.magic_code != MAGIC_CODE) {
                    config_settings.magic_code = MAGIC_CODE;
                    eeprom_write_block((const void*)&config_settings, (void*)0, sizeof(config_settings));
              }
              inited = 1;
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
             screens[id-1][data_len] = 0; //verify null termination
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
        printf("got first message, total len: %d\n", data_len);
        return;
    }    
    else if(continued_packet==msg_type) {
      if(!current_ptr) {
          printf("got invalid continued message\n");
      }
        memcpy(data+current_ptr, buf+1, msg_len-1);
        current_ptr += msg_len-1;
        if(current_ptr > data_len)
                printf("invalid continued message, ptr overflow\n");
        return;
    }    
    else if(last_packet==msg_type) {
      if(!current_ptr) {
          printf("got invalid last message\n");
          return;  
    }
        memcpy(data+current_ptr, buf+1, msg_len-1);
        current_ptr += msg_len-1;
        if(current_ptr+1 != data_len) {
                printf("invalid ptr: %d, data len: %d\n", current_ptr, data_len);
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
         printf("sending message, length: %d content:\n", len);
         for(int i=0;i<len+1;i++) {
             printf("0x%X ", tx_buf[i]);
         }
         printf("\n");
         radio.stopListening();
         int success = radio.write(tx_buf, len+1);
         radio.startListening();
         if(success)
             printf("send single packet succeded\n");
         else
             printf("send single packet failed\n");
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
       LcdInitialise();
       LcdClear();
        printf_begin();
        printf("\n\rNokia screen receiver\n\r");
        randomSeed(analogRead(0));
        eeprom_read_block((void*)&config_settings, (void*)0, sizeof(config_settings));
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
//  Serial.printf("going to refresh screen\n") ;
    if (abs(now() - last_screen_time) < SCREEN_TIME) {
        return;
    }
   last_screen_time = now();
   last_screen_id=(last_screen_id+1) % (MAX_SCREENS+1);
    while(last_screen_id && !screens_size[last_screen_id-1])
          last_screen_id=(last_screen_id+1) % (MAX_SCREENS+1);
    if(!last_screen_id){
//              LcdClear();
//              digitalClockDisplay();
              return;
    }
     LcdClear();
     LcdString((char*)screens[last_screen_id-1]);
}

void loop()
{
        if (!inited) {
            LcdClear();
            LcdString("Not Registered, trying to connect host");
            printf("not initialized, trying to connect host\n");
            radio.stopListening();
            send_init_packet();
            radio.startListening();
        }
        unsigned long started_waiting_at = millis();
        bool timeout = false;
        while (!radio.available() && ! timeout) {
            if (millis() - started_waiting_at > 1000)
               timeout = true;
        }
        if (!timeout) {
    //        radio.stopListening();
            uint8_t payload_size = radio.getPayloadSize();
            radio.read(radio_buf, payload_size);
            printf("got message, length: %d content: ", payload_size);
            for(int i=0;i<payload_size;i++)
                printf("0x%X ", radio_buf[i]);
            printf("\n");            
            handle_read(radio_buf, payload_size);
        }
        next_screen();
      delay(10);
}

