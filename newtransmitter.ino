#include <SPIFFS.h>
#include <SPI.h>
#include <LoRa.h>
#include <U8x8lib.h>

#define ss 18
#define rst 14
#define dio0 26
#define MAX_RETRIES 5
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(15, 4, 16);
const int buttonPin = 0;
const int state1 = 1;
const int state2 = 2;
int currentState = state2;
int LORA_PACKET_SIZE = 224;
const uint8_t ACK_PACKET = 0x01;

void setup() {
  Serial.begin(115200);
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, "Setup Mode");

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error occured while mounting SPIFFS");
    return;
  }
  //listing all files in the spiffs filesystem
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file) {
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }

  //Set LoRa module pins
  LoRa.setPins(ss, rst, dio0);

  //Initialize the LoRa transceiver module
  if (!LoRa.begin(868E6)) {
    Serial.print("LoRa initialization failed. Check the connections.");
    while (1)
      ;
  }

  //Adjust the LoRa settings for transmitting
  LoRa.setSpreadingFactor(7);
  LoRa.setSyncWord(0xBA);
  LoRa.setCodingRate4(5);
  delay(2000);
}

void loop() {
  // ckeck if PRG button is pressed
  if (digitalRead(buttonPin) == LOW) {
    switch (currentState) {
      case state1:
        currentState = state2;
        delay(1000);
        break;
      case state2:
        currentState = state1;
        delay(1000);
        break;
    }
    switch (currentState) {
      //STATE 1 IDLE
      case state1:
        Serial.println("State1");
        u8x8.clear();
        u8x8.setCursor(0, 2);
        u8x8.print("IDLE MODE");

        break;

      //STATE 2 transmitting the SPIFFS file
      case state2:
        Serial.println("State2");
        u8x8.clear();
        u8x8.setCursor(0, 2);
        u8x8.print("TANSMITTER MODE");
        sender();
        break;
    }
  }
}

//transmitter function
void sender() {
  int packetCounter = 0;

  //open the SPIFFS file
  File file = SPIFFS.open("/firmware.bin");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  // calculate needed packets to transmitt firmware
  int totalPackets = ceil(file.size() / LORA_PACKET_SIZE);
  // buffer
  uint8_t buffer[LORA_PACKET_SIZE];
  int size = file.size();

  while (size > 0) {
    if (size < 224) {
      LORA_PACKET_SIZE = size;
    }
    delay(500);
    // read from file
    int bytesRead = file.read(buffer, LORA_PACKET_SIZE);
    size -= LORA_PACKET_SIZE;
    if (bytesRead <= 0) {
      break;
    }
    //split the number into two 8-bit pieces
    byte lowerByte = packetCounter & 0xFF;          //get the lower 8 bits
    byte higherByte = (packetCounter >> 8) & 0xFF;  //get the higher 8 bits

    int retries = 0;
    while (retries < MAX_RETRIES) {
      // current send packet of maximum
      u8x8.setCursor(0, 4);
      u8x8.print("Packet:");
      u8x8.setCursor(0, 5);
      u8x8.print(packetCounter);
      u8x8.setCursor(0, 6);
      u8x8.print("of");
      u8x8.setCursor(0, 7);
      u8x8.print(totalPackets);

      //send package over lora
      LoRa.beginPacket();
      LoRa.write(lowerByte);
      LoRa.write(higherByte);
      LoRa.write(buffer, bytesRead);
      LoRa.endPacket();

      
      bool ack_receive = false;
      long start = millis();
      long wait = 2200;
      while (start + wait > millis()) {
        if (LoRa.parsePacket() > 0) {
          // received ACK, move on to next packet
          packetCounter++;
          ack_receive = true;
          break;
        }
      }
      if (ack_receive == true) {
        break;
      }
      retries++;
      // logging
      Serial.println(packetCounter);
      Serial.println(lowerByte);
      Serial.println(higherByte);
      Serial.println("Bytes read: ");
      Serial.println(bytesRead);
      //printf("%.*s", LORA_PACKET_SIZE, (char*)buffer);
      delay(1000);
    }

    if (retries == MAX_RETRIES) {
      // maximum retries reached, give up
      Serial.println("Failed to send packet");
      return;
    }
  }
  file.close();
  // file was sent successfully
  Serial.println("File sent successfully");
}