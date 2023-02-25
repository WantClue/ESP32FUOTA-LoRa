![GitHub release (latest by date)](https://img.shields.io/github/downloads/WantClue/ESP32FUOTA-LoRa/latest/total)
# ESP32FUOTA-LoRa
This is a method to update a Heltec ESP32 V2 over LoRa using the SPIFFS Filesystem.



## Tools you need for this Repo to work on your Heltec ESP32 V2
1. **Arduino IDE 1.8.18 or lower.** [Download](https://www.arduino.cc/en/software/OldSoftwareReleases)
2. **Two Heltec ESP32 V2**
3. **SPIFFS Filesystem Plugin** [Tutorial](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/)
4. You will need the ESP32 Dev Board Manager and the U8g2 Libary by Oliver in the Arduino IDE aswell as the LoRa Libary by Sandeep Mistry

## Instructions

Upload the newreceiver.ino and the newtransmitter.ino
Upload the firmware.bin to the Transmitter ESP using the SPIFFS Filesystem Plugin.
The Firmware.bin is the same firmware as the newreceiver firmware with the change that the pin 39 is used to read the analog output of a potentiometer.

If you have an analog potentiometer connect it to GND and 3V3 and the data output to pin 39.

You can also change the newreceiver code section: `case state1:` to your personal preferences so that your receiver devices does something differently in IDLE mode.

### Disclaimer

This code is far from perfect and will hopefully in the future see an interation for LoRaWAN-Networks. Feels free to improve this code or change it to your personal behaiviour. 
