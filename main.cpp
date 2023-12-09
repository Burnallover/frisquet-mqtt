#include <Arduino.h>
#include <RadioLib.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

SX1262 radio = new Module(SS, DIO0, RST_LoRa, BUSY_LoRa); 

// Configuration Wifi
const char* ssid = "ssid wifi";  // Mettre votre SSID Wifi
const char* password = "wifi password";  // Mettre votre mot de passe Wifi

// Définition de l'adresse du broket MQTT
const char* mqttServer = "192.168.XXX.XXX"; // Mettre l'ip du serveur mqtt
const int mqttPort = 1883;
const char* mqttUsername = "mqttUsername"; // Mettre le user mqtt
const char* mqttPassword = "mqttPassword"; // Mettre votre mot de passe mqtt

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
    uint8_t network_id[] = {0xNN, 0xNN, 0xNN, 0xNN}; // remplacer NN par le network id de la chaudiere
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
        for (int i = 0; i < len; i++) 
            sprintf(message + strlen(message), "%02X ", byteArr[i]);
            client.publish("homeassistant/sensor/frisquet_payload/state", message);
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
