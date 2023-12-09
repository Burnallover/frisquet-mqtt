This arduino code is made for a heltec_wifi_lora_32_V3. 
It will automatically create a sensor named "Payload frisquet" in Home Assistant. 
Most of this code was found on https://forum.hacf.fr/t/pilotage-chaudiere-frisquet-eco-radio-system-visio/19814/90
 
# Requirement

1. Mosquitto broker installed and linked with Home assistant
2. user and password for mqtt 
3. IP of Mosquitto broker
4. SSID and password of the wifi
5. boiler's network ID

# how to retrieve the boiler's network ID

To retrieve the boiler's network ID you have to: 

1. Put this code on the heltec_wifi_lora_32_V3
```bash
#include <Arduino.h>
#include <RadioLib.h>

SX1262 radio = new Module(SS, DIO0, RST_LoRa, BUSY_LoRa); 

void setup() {
    Serial.begin(115200);
    int state = radio.beginFSK();
    state = radio.setFrequency(868.96);
    state = radio.setBitRate(25.0);
    state = radio.setFrequencyDeviation(50.0);
    state = radio.setRxBandwidth(250.0);
    state = radio.setPreambleLength(4);
    uint8_t network_id[] = {0xFF, 0xFF, 0xFF, 0xFF};
    state = radio.setSyncWord(network_id, sizeof(network_id));
}

void loop() {
    byte byteArr[RADIOLIB_SX126X_MAX_PACKET_LENGTH];
    int state = radio.receive(byteArr, 0);
    if (state == RADIOLIB_ERR_NONE) {
        int len = radio.getPacketLength();
        Serial.printf("RECEIVED [%2d] : ", len);
        for (int i = 0; i < len; i++) 
            Serial.printf("%02X ", byteArr[i]);
        Serial.println("");
    }
}
```
2. Remove the Visio module from your boiler and readjust it. You'll see on your Heltec console that lines will be received.

```bash
RECEIVED [11] : 00 80 33 D8 02 41 04 NN NN NN NN 
RECEIVED [11] : 00 80 1A 04 02 41 04 NN NN NN NN 
RECEIVED [14] : 80 08 1A 04 82 41 03 23 12 06 01 27 00 02
```
The NN part is the boiler's network ID

# Configuration

When you know your boiler's network ID, you can use the code in this repository's main.cpp file to modify the following lines :
```bash
11 // Configuration Wifi
12 const char* ssid = "ssid wifi";  // Mettre votre SSID Wifi
13 const char* password = "wifi password";  // Mettre votre mot de passe Wifi
```
```bash
15 // Définition de l'adresse du broket MQTT
16 const char* mqttServer = "192.168.XXX.XXX"; // Mettre l'ip du serveur mqtt
17 const int mqttPort = 1883;
18 const char* mqttUsername = "mqttUsername"; // Mettre le user mqtt
19 const char* mqttPassword = "mqttPassword"; // Mettre votre mot de passe mqtt
```
```bash
79 uint8_t network_id[] = {0xNN, 0xNN, 0xNN, 0xNN}; // remplacer NN par le network id de la chaudière
```
