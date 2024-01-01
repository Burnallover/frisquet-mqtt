#include <Arduino.h>
#include <RadioLib.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <heltec.h>
#include <Preferences.h>
#include "config.h"  // Include the configuration file

SX1262 radio = new Module(SS, DIO0, RST_LoRa, BUSY_LoRa); 
Preferences preferences;

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
char temperatureConfigTopic[] = "homeassistant/sensor/frisquet/tempAmbiante/config";
char temperatureConfigPayload[] = R"(
{
  "uniq_id": "frisquet_tempAmbiante",
  "name": "Frisquet - Temperature ambiante",
  "state_topic": "homeassistant/sensor/frisquet/tempAmbiante/state",
  "unit_of_measurement": "°C",
  "device_class": "temperature",
  "device":{"ids":["FrisquetConnect"],"mf":"Frisquet","name":"Frisquet Connect","mdl":"Frisquet Connect"}
}
)";
client.publish(temperatureConfigTopic, temperatureConfigPayload);

  // Configuration du capteur de température exterieure
char temperatureExtConfigTopic[] = "homeassistant/sensor/frisquet/tempExterieure/config";
char temperatureExtConfigPayload[] = R"(
{
  "uniq_id": "frisquet_tempExterieure",
  "name": "Frisquet - Temperature Exterieure",
  "state_topic": "homeassistant/sensor/frisquet/tempExterieure/state",
  "unit_of_measurement": "°C",
  "device_class": "temperature",
  "device":{"ids":["FrisquetConnect"],"mf":"Frisquet","name":"Frisquet Connect","mdl":"Frisquet Connect"}
}
)";
client.publish(temperatureExtConfigTopic, temperatureExtConfigPayload);
  
// Configuration du capteur de température de consigne
char tempconsigneConfigTopic[] = "homeassistant/sensor/frisquet/tempConsigne/config";
char tempconsigneConfigPayload[] = R"(
{
  "uniq_id": "frisquet_tempConsigne",
  "name": "Frisquet - Temperature consigne",
  "state_topic": "homeassistant/sensor/frisquet/tempConsigne/state",
  "unit_of_measurement": "°C",
  "device_class": "temperature",
  "device":{"ids":["FrisquetConnect"],"mf":"Frisquet","name":"Frisquet Connect","mdl":"Frisquet Connect"}
}
)";
client.publish(tempconsigneConfigTopic, tempconsigneConfigPayload);

// Configuration récupération Payload
char payloadConfigTopic[] = "homeassistant/sensor/frisquet/payload/config";
char payloadConfigPayload[] = R"(
{
  "name": "Frisquet - Payload",
  "state_topic": "homeassistant/sensor/frisquet/payload/state",
  "device":{"ids":["FrisquetConnect"],"mf":"Frisquet","name":"Frisquet Connect","mdl":"Frisquet Connect"}
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
  // Démarrer l'instance de NVS
  preferences.begin("customId", false); // "customId" est le nom de l'espace de stockage

  // Vérifier si la clé "custom_network_id" existe dans NVS
  if (preferences.containsKey("custom_network_id")) {
    // Si la clé existe, lisez la nouvelle valeur
    size_t network_id_size = preferences.getBytes("custom_network_id", network_id, sizeof(network_id));
  }
  
  // Initialize OLED display
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);

  
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
  client.setBufferSize(2048);
  connectToMqtt();
  connectToTopic();

String byteArrayToHexString(uint8_t* byteArray, int length) {
    String result = "";
    for (int i = 0; i < length; i++) {
        char hex[3];
        sprintf(hex, "%02X", byteArray[i]);
        result += hex;
    }
    return result;
}
int counter = 0;
}
void loop() {
  // Vérifiez si network_id est égal à la valeur par défaut
  if (memcmp(network_id, "\xFF\xFF\xFF\xFF", 4) == 0) {
    // Stockez la nouvelle valeur dans NVS
    byte byteArr[RADIOLIB_SX126X_MAX_PACKET_LENGTH];
    int state = radio.receive(byteArr, 0);
    if (state == RADIOLIB_ERR_NONE) {
        int len = radio.getPacketLength();
        Serial.printf("RECEIVED [%2d] : ", len);
        for (int i = 0; i < len; i++) 
            Serial.printf("%02X ", byteArr[i]);
        Serial.println("");
        if (len == 11) {
          // Copiez les 4 derniers octets du payload dans custom_network_id
          for (int i = 0; i < 4; i++) {
            custom_network_id[i] = byteArr[len - 4 + i];
          }
          preferences.putBytes("custom_network_id", custom_network_id, sizeof(custom_network_id));
          // Maintenant, custom_network_id contient les 4 derniers octets du payload
        }
    } else {
  if (!client.connected()) {
    connectToMqtt();
  }
  ArduinoOTA.handle();
  
    //Compteur pour limiter la déclaration des topic  
    if (counter >= 100) {
    connectToTopic();
    counter = 0;
    }
    counter++;
  

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
      Heltec.display->clear();
      Heltec.display->drawString(0, 0, "Network ID: " + byteArrayToHexString(network_id, 4));
      Heltec.display->drawString(0, 12, "Temperature: " + String(temperatureValue) + "°C");
      Heltec.display->drawString(0, 24, "Consigne: " + String(temperatureconsValue) + "°C");

      // Display other information as needed

      Heltec.display->display();
    }
    for (int i = 0; i < len; i++) {
      sprintf(message + strlen(message), "%02X ", byteArr[i]);
      Serial.printf("%02X ", byteArr[i]);}
      if (!client.publish("homeassistant/sensor/frisquet_payload/state", message)) {
        Serial.println("Failed to publish Payload to MQTT");
    }
    Serial.println("");
  }
  }
  preferences.end();
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
