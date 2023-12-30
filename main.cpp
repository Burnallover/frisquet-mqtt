#include <Arduino.h>
#include <RadioLib.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "config.h"  // Include the configuration file

SX1262 radio = new Module(SS, DIO0, RST_LoRa, BUSY_LoRa); 

WiFiClient espClient;
PubSubClient client(espClient);

void connectToMqtt() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32 Frisquet", mqttUsername, mqttPassword)) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("Failed to connect to MQTT, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
      }
  }
}
void connectToTopic() {
  // Configuration du capteur de température
char temperatureConfigTopic[] = "homeassistant/sensor/frisquet_temperature/config";
char temperatureConfigPayload[] = R"(
{
  "name": "Maison Temperature interieur",
  "state_topic": "homeassistant/sensor/frisquet_temperature/state",
  "unit_of_measurement": "°C",
  "device_class": "temperature"
}
)";
client.publish(temperatureConfigTopic, temperatureConfigPayload);

  // Configuration du capteur de température de consigne
char tempconsigneConfigTopic[] = "homeassistant/sensor/frisquet_consigne/config";
char tempconsigneConfigPayload[] = R"(
{
  "name": "Maison Temperature consigne",
  "state_topic": "homeassistant/sensor/frisquet_consigne/state",
  "unit_of_measurement": "°C",
  "device_class": "temperature"
}
)";
client.publish(tempconsigneConfigTopic, tempconsigneConfigPayload);

// Configuration récupération Payload
char payloadConfigTopic[] = "homeassistant/sensor/frisquet_payload/config";
char payloadConfigPayload[] = R"(
{
  "name": "Payload frisquet",
  "state_topic": "homeassistant/sensor/frisquet_payload/state"
}
)";
client.publish(payloadConfigTopic, payloadConfigPayload);
}

void initOTA();

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
    
  int state = radio.beginFSK();
  state = radio.setFrequency(868.96);
  state = radio.setBitRate(25.0);
  state = radio.setFrequencyDeviation(50.0);
  state = radio.setRxBandwidth(250.0);
  state = radio.setPreambleLength(4);
  state = radio.setSyncWord(network_id, sizeof(network_id));

  initOTA();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialisation de la connexion MQTT
  client.setServer(mqttServer, mqttPort);
  connectToMqtt();
  connectToTopic();
}

void loop() {
  if (!client.connected()) {
    connectToMqtt();
  }
  ArduinoOTA.handle();
  connectToTopic();

  char message[255];
  byte byteArr[RADIOLIB_SX126X_MAX_PACKET_LENGTH];
  int state = radio.receive(byteArr, 0);
  if (state == RADIOLIB_ERR_NONE) {
    int len = radio.getPacketLength();
    Serial.printf("RECEIVED [%2d] : ", len);
    message[0] = '\0';

    if (len == 23) {  // Check if the length is 23 bytes

      // Extract bytes 16 and 17
      int decimalValueTemp = byteArr[15] << 8 | byteArr[16];
      float temperatureValue = decimalValueTemp / 10.0;

      // Extract bytes 18 and 19
      int decimalValueCons = byteArr[17] << 8 | byteArr[18];
      float temperatureconsValue = decimalValueCons / 10.0;

      // Publish temperature to the "frisquet_temperature" MQTT topic
      char temperatureTopic[] = "homeassistant/sensor/frisquet_temperature/state";
      char temperaturePayload[10];
      snprintf(temperaturePayload, sizeof(temperaturePayload), "%.2f", temperatureValue);
      if (!client.publish(temperatureTopic, temperaturePayload)) {
        Serial.println("Failed to publish temperature to MQTT");
      }
      // Publish temperature to the "tempconsigne" MQTT topic
      char tempconsigneTopic[] = "homeassistant/sensor/frisquet_consigne/state";
      char tempconsignePayload[10];
      snprintf(tempconsignePayload, sizeof(tempconsignePayload), "%.2f", temperatureconsValue);
      if (!client.publish(tempconsigneTopic, tempconsignePayload)) {
        Serial.println("Failed to publish consigne to MQTT");
      }
    }
    for (int i = 0; i < len; i++) {
      sprintf(message + strlen(message), "%02X ", byteArr[i]);
      Serial.printf("%02X ", byteArr[i]);}
      if (!client.publish("homeassistant/sensor/frisquet_payload/state", message)) {
        Serial.println("Failed to publish Payload to MQTT");
    }
    Serial.println("");
  }
  client.loop();
}

void initOTA() {
 
  ArduinoOTA.setHostname("ESP32 Frisquetconnect");
  ArduinoOTA.setTimeout(25);  // Augmenter le délai d'attente à 25 secondes

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
}
