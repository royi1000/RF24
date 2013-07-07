#include <avr/eeprom.h>
#include <SPI.h>
#include <EEPROM.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define PAYLOAD_SIZE 10
#define COMMAND_INIT 1
#define COMMAND_INIT_RESPONSE 2

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
uint64_t pipes[2] = {0xF0F0F0F01FLL,  0xF0F0F0F0E1LL};

typedef enum {stand_alone=0, first_packet=1, continued_packet=2, last_packet=3} packet_type_e;

uint8_t inited = 0;
char radio_buf[32];
char data[1024];
unsigned int data_len = 0;
unsigned int current_ptr = 0;

typedef struct store {
    uint16_t id;
    uint64_t rx_addr;
    uint64_t tx_addr;
} store_t;

typedef struct cmd_message {
    uint8_t  command;
    uint16_t id;
    uint64_t addr;
} cmd_message_t;

store_t config_settings;

void handle_read(char* buf, unsigned int len){
     
}

void handle_write(char* buf, unsigned int len){
     tx_buf[32];
     if(len<radio.getPayloadSize()) {
         tx_buf[0] = (stand_alone << 6) & len+1;
         memcpy(tx_buf+1, buf, len);
         printf("got message, length: %d content:\n", payload_size);
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
    message.id = id;
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

        eeprom_read_block((void*)&config_store, (void*)0, sizeof(config_settings));
        // Setup and configure rf radio
        if (!config_store.id) {
           config_store.rx_addr = 0xF0F0F0F0E1LL;
           config_store.tx_addr = 0xF0F0F0F01FLL;
        }
        radio.begin();
        

        // optionally, increase the delay between retries & # of retries
        radio.setRetries(15,15);

        radio.enableDynamicPayloads();
        radio.setPayloadSize(PAYLOAD_SIZE);
  
        radio.openWritingPipe(config_store.tx_addr);
        radio.openReadingPipe(1, config_store.rx_addr);

        radio.startListening();
        radio.printDetails();      

}

void loop()
{
        if (!inited) {
            radio.stopListening();
            send_init_packet();
            radio.startListening();
        }
        unsigned long started_waiting_at = millis();
        bool timeout = false;
        while (!radio.available() && ! timeout)
            if (millis() - started_waiting_at > 1000)
               timeout = true;
        }
        if (!timeout) {
            radio.stopListening();
            uint8_t payload_size = getDynamicPayloadSize();
            radio.read(radio_buf, sizeof(radio_buf));
            printf("got message, length: %d content:\n", payload_size);
            for(int i=0;i<payload_size;i++)
                printf("0x%X ", radio_buf[i]);
            printf("\n");            
            handle_read(radio_buf, payload_size);
        }
        
}