#include <avr/eeprom.h>
#include <SPI.h>
#include <EEPROM.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define PAYLOAD_SIZE 20
#define COMMAND_INIT 0xF0
#define COMMAND_INIT_RESPONSE 0xF1
#define DEVICE_TYPE 0x10
const uint16_t MAGIC_CODE = 0xDE12;
// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
uint64_t pipes[2] = {0xF0F0F0F0D2LL,  0xF0F0F0F0E1LL};

typedef enum {stand_alone=0, first_packet=1, continued_packet=2, last_packet=3} packet_type_e;

uint8_t inited = 0;
char radio_buf[32];
char data[1024];
unsigned int data_len = 0;
unsigned int current_ptr = 0;

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

store_t config_settings;

void handle_read(char* buf, unsigned int len){
     
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
    message.type = DEVICE_TYPE;
    message.addr = config_settings.rx_addr;
    radio.stopListening();
    handle_write((void *)&message, sizeof(message));
    radio.startListening();
 }

void setup()
{
        Serial.begin(9600);
        printf_begin();
        printf("\n\rNokia screen receiver\n\r");
        randomSeed(analogRead(0));
        eeprom_read_block((void*)&config_settings, (void*)0, sizeof(config_settings));
        // Setup and configure rf radio
        if (0xDE12 != config_settings.magic_code) {
           config_settings.rx_addr = random(0xFFFFFFFFFF);
           config_settings.tx_addr = 0xF0F0F0F0D2LL;
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

void loop()
{
        if (!inited) {
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
            radio.stopListening();
            uint8_t payload_size = radio.getDynamicPayloadSize();
            radio.read(radio_buf, sizeof(radio_buf));
            printf("got message, length: %d content:\n", payload_size);
            for(int i=0;i<payload_size;i++)
                printf("0x%X ", radio_buf[i]);
            printf("\n");            
            handle_read(radio_buf, payload_size);
        }
      delay(1000);      
}

