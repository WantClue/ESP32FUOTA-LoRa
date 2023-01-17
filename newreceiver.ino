// copyright by Lars Näther
//this implementation is been written for a heltec esp32 v2
//needs to be adjusted to your personal use

#include <SPIFFS.h>
#include <LoRa.h>
#include <U8x8lib.h>
#include <Update.h>

#define ss 18
#define rst 14
#define dio0 26
#define ONBOARD_LED 25
#define LORA_PACKET_SIZE 224
#define MAX_PACKETS 3000
int receivedPacketCounters[MAX_PACKETS] = { 0 };
int lastReceivedPacket = -1;  // initialize to -1

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(15, 4, 16);


const int buttonPin = 0;
const int state1 = 1;
const int state2 = 2;
const int state3 = 3;
int currentState = state3;
int timer = 0;

void setup() {
  //Serial begin
  Serial.begin(115200);
  //Idle mode blink
  pinMode(ONBOARD_LED, OUTPUT);


  //display start sequenz
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, "Test Mode");

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error occured while mounting the spiffs system");
    return;
  }

  //Set LoRa pins
  LoRa.setPins(ss, rst, dio0);

  //Initialize the LoRa transceiver module
  if (!LoRa.begin(868E6)) {
    Serial.print("LoRa initialization failed. Check the connections.");
    while (1)
      ;
  }

  //Set LoRa settings
  LoRa.setSyncWord(0xBA);
  LoRa.setCodingRate4(5);

  SPIFFS.remove("/receivedFirmware.bin");


  //listing all files in the spiffs filesystem
  File root = SPIFFS.open("/");

  File file = root.openNextFile();

  while (file) {

    Serial.print("FILE: ");
    Serial.println(file.name());

    file = root.openNextFile();
  }
  //open and read the content of receivedFile
  File file2 = SPIFFS.open("/test.txt");
  if (!file2) {
    Serial.println("Failed to open the file for reading");
    return;
  }
  Serial.println("File Content:");
  while (file2.available()) {
    Serial.write(file2.read());
  }
  file2.close();
}

void loop() {
  // ckeck if button is pressed
  if (digitalRead(buttonPin) == LOW) {
    switch (currentState) {
      case state1:
        currentState = state2;
        delay(1000);
        break;
      case state2:
        currentState = state3;
        delay(1000);
        break;
      case state3:
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
        u8x8.setCursor(0, 4);
        u8x8.print("Hold to exit!");
        while (timer < 10000) {
          delay(1000);
          digitalWrite(ONBOARD_LED, HIGH);
          delay(1000);
          digitalWrite(ONBOARD_LED, LOW);
          if (digitalRead(buttonPin) == LOW) {
            break;
          }
          timer++;
        }


        break;

      //STATE 2 receiving the SPIFFS file
      case state2:
        Serial.println("State2");
        u8x8.clear();
        u8x8.setCursor(0, 2);
        u8x8.print("RECEIVER MODE");
        u8x8.setCursor(0, 4);
        u8x8.print("Press to break!");
        //receive data over LoRa
        while (1) {
          receiver(LoRa.parsePacket());
          if (digitalRead(buttonPin) == LOW) {
            break;
          }
        }
        Serial.print("Ich bin aus der Schleife");
        break;
      //STATE 3 updating the firmware
      case state3:
        Serial.println("State 3");
        u8x8.clear();
        u8x8.setCursor(0, 2);
        u8x8.print("UPDATE MODE");

        updateMode();
        break;
    }
  }
}
void receiver(int packetSize) {
  //Receive the file packet by packet
  if (packetSize <= 0) {
    return;
  }
  //read packetCounter and payload
  byte lowerByte = LoRa.read();
  byte higherByte = LoRa.read();
  int receivedPacketCounter = (higherByte << 8) | lowerByte;
  uint8_t buffer[LORA_PACKET_SIZE];
  int bytesRead = 0;
  while (LoRa.available()) {
    buffer[bytesRead] = LoRa.read();
    bytesRead++;
  }
  Serial.println("Bytes read:");
  Serial.println(bytesRead);
  // check if packet has already been received
  bool packetReceived = false;
  for (int i = 0; i < MAX_PACKETS; i++) {
    if (receivedPacketCounters[i] == receivedPacketCounter && receivedPacketCounter != 0) {
      packetReceived = true;
      break;
    }
  }

  if (!packetReceived && receivedPacketCounter == lastReceivedPacket + 1) {
    //write payload to file
    File file = SPIFFS.open("/receivedFirmware.bin", "a");
    if (!file) {
      Serial.println("Failed to open file for writing");
    }
    file.write(buffer, bytesRead);
    file.close();
    Serial.println("File has been appended");

    // add packet counter to array
    for (int i = 0; i < MAX_PACKETS; i++) {
      if (receivedPacketCounters[i] == 0) {
        receivedPacketCounters[i] = receivedPacketCounter;
        break;
      }
    }
    lastReceivedPacket = receivedPacketCounter;
    u8x8.setCursor(0, 5);
    u8x8.print(lastReceivedPacket);

    // send ACK
    delay(100);
    LoRa.beginPacket();
    LoRa.write(receivedPacketCounter + 1);
    LoRa.endPacket();
  }



  //check if this was the last packet
  if (packetSize - 1 < 224) {
    // last packet received, exit loop
    return;
  }

  Serial.println("Received package #");
  Serial.println(receivedPacketCounter);
  Serial.println("Last received packet:");
  Serial.println(lastReceivedPacket);
}
void updateMode() {
  delay(1000);
  u8x8.clear();
  u8x8.setCursor(0, 4);
  u8x8.print("Update in 3");
  delay(1000);
  u8x8.clear();
  u8x8.setCursor(0, 4);
  u8x8.print("Update in 2");
  delay(1000);
  u8x8.clear();
  u8x8.setCursor(0, 4);
  u8x8.print("Update in 1");
  u8x8.clear();

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/firmware.bin");

  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("Starting update..");


  size_t fileSize = file.size();

  if (!Update.begin(fileSize)) {

    Serial.println("Cannot do the update");
    return;
  };

  Update.writeStream(file);

  if (Update.end()) {

    Serial.println("Successful update");
    u8x8.clear();
    u8x8.setCursor(1, 1);
    u8x8.print("Successful update");
  } else {

    Serial.println("Error Occurred: " + String(Update.getError()));
    u8x8.clear();
    u8x8.setCursor(1, 1);
    u8x8.print("Error Occured: ");
    u8x8.print(String(Update.getError()));
    return;
  }

  file.close();

  Serial.println("Reset in 4 seconds...");
  delay(4000);

  ESP.restart();
}