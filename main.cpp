#include <Arduino.h>
#include <RadioLib.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <heltec.h>
#include <Preferences.h>
#include "image.h"
#include "config.h" // Include the configuration file

SX1262 radio = new Module(SS, DIO0, RST_LoRa, BUSY_LoRa);
Preferences preferences;

String tempAmbiante;
String tempExterieure;
String tempConsigne;
String modeFrisquet;
String byteArrayToHexString(uint8_t *byteArray, int length);
int counter = 0;
uint8_t custom_network_id[4];
float temperatureValue;
float temperatureconsValue;

WiFiClient espClient;
PubSubClient client(espClient);

char temperatureExtConfigTopic[] = "homeassistant/sensor/frisquet/tempExterieure/config"; // Déclarer la variable globale
const char *modeConfigTopic = "homeassistant/select/frisquet/mode/config";

// Drapeaux pour indiquer si les données ont changé
bool tempAmbianteChanged = false;
bool tempExterieureChanged = false;
bool tempConsigneChanged = false;
bool modeFrisquetChanged = false;

void updateDisplay()
{
  if (tempAmbianteChanged || tempExterieureChanged || tempConsigneChanged || modeFrisquetChanged)
  {
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "Network ID: " + byteArrayToHexString(custom_network_id, sizeof(custom_network_id)));
    Heltec.display->drawString(0, 11, "Temp Ambiante: " + tempAmbiante + "°C");
    Heltec.display->drawString(0, 22, "Temp Exterieure: " + tempExterieure + "°C");
    Heltec.display->drawString(0, 33, "Temp Consigne: " + tempConsigne + "°C");
    Heltec.display->drawString(0, 44, "Mode: " + modeFrisquet);

    // Display other information as needed

    Heltec.display->display();
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // Convertir le payload en une chaîne de caractères
  char message[length + 1];
  strncpy(message, (char *)payload, length);
  message[length] = '\0';

  // Vérifier le topic et agir en conséquence
  // Vérifier le topic et mettre à jour les variables globales
  if (strcmp(topic, "homeassistant/sensor/frisquet/tempAmbiante/state") == 0)
  {
    if (tempAmbiante != String(message))
    {
      tempAmbiante = String(message);
      tempAmbianteChanged = true;
    }
  }
  else if (strcmp(topic, "homeassistant/sensor/frisquet/tempExterieure/state") == 0)
  {
    if (tempExterieure != String(message))
    {
      tempExterieure = String(message);
      tempExterieureChanged = true;
    }
  }
  else if (strcmp(topic, "homeassistant/sensor/frisquet/tempConsigne/state") == 0)
  {
    if (tempConsigne != String(message))
    {
      tempConsigne = String(message);
      tempConsigneChanged = true;
    }
  }
  else if (strcmp(topic, "homeassistant/select/frisquet/mode/set") == 0)
  {
    if (modeFrisquet != String(message))
    {
      modeFrisquet = String(message);
      modeFrisquetChanged = true;
      client.publish("homeassistant/select/frisquet/mode/state", message);
    }
  }
}

void connectToMqtt()
{
  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32 Frisquet", mqttUsername, mqttPassword))
    {
      Serial.println("Connected to MQTT");
    }
    else
    {
      Serial.print("Failed to connect to MQTT, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void connectToTopic()
{
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

  // Publier le message de configuration pour MQTT du mode
  const char *modeConfigTopic = "homeassistant/select/frisquet/mode/config";
  const char *modeConfigPayload = R"({
        "uniq_id": "frisquet_mode",
        "name": "Frisquet - Mode",
        "state_topic": "homeassistant/select/frisquet/mode/state",
        "command_topic": "homeassistant/select/frisquet/mode/set",
        "options": ["Auto", "Confort", "Réduit", "Hors gel"],
        "device":{"ids":["FrisquetConnect"],"mf":"Frisquet","name":"Frisquet Connect","mdl":"Frisquet Connect"}
      })";
  client.publish(modeConfigTopic, modeConfigPayload, true); // true pour retenir le message
  // Souscrire aux topics temp ambiante, consigne, ext et mode
  client.subscribe("homeassistant/select/frisquet/mode/set");
  client.subscribe("homeassistant/sensor/frisquet/tempAmbiante/state");
  client.subscribe("homeassistant/sensor/frisquet/tempExterieure/state");
  client.subscribe("homeassistant/sensor/frisquet/tempConsigne/state");
}

void initOTA();

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  // Démarre l'instance de NVS
  preferences.begin("net-conf", false); // "customId" est le nom de l'espace de stockage

  // Vérifie si la clé "custom_network_id" existe dans NVS
  size_t custom_network_id_size = preferences.getBytes("net_id", custom_network_id, sizeof(custom_network_id));
  if (custom_network_id_size != sizeof(custom_network_id))
  {

    // Vérifier si network_id est différent de {0xFF, 0xFF, 0xFF, 0xFF}
    if (memcmp(network_id, "\xFF\xFF\xFF\xFF", sizeof(network_id)) != 0)
    {
      // network_id est différent de {0xFF, 0xFF, 0xFF, 0xFF}
      // Copie la valeur de network_id dans custom_network_id
      memcpy(custom_network_id, network_id, sizeof(network_id));
      preferences.putBytes("net_id", custom_network_id, sizeof(custom_network_id));
    }
    else
    {
      // network_id est égal à {0xFF, 0xFF, 0xFF, 0xFF}
      // Initialisez custom_network_id avec {0xFF, 0xFF, 0xFF, 0xFF}
      custom_network_id[0] = 0xFF;
      custom_network_id[1] = 0xFF;
      custom_network_id[2] = 0xFF;
      custom_network_id[3] = 0xFF;
      preferences.putBytes("net_id", custom_network_id, sizeof(custom_network_id));
    }
  }

  // Initialize OLED display
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);

  Heltec.display->clear();
  Heltec.display->drawXbm(0, 0, 128, 64, myLogo);
  Heltec.display->display();

  int state = radio.beginFSK();
  state = radio.setFrequency(868.96);
  state = radio.setBitRate(25.0);
  state = radio.setFrequencyDeviation(50.0);
  state = radio.setRxBandwidth(250.0);
  state = radio.setPreambleLength(4);
  state = radio.setSyncWord(custom_network_id, sizeof(custom_network_id));

  initOTA();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialisation de la connexion MQTT
  client.setServer(mqttServer, mqttPort);
  client.setBufferSize(2048);
  connectToMqtt();
  connectToTopic();
  client.setCallback(callback);

  preferences.end(); // Fermez la mémoire NVS ici
}
void loop()
{
  // Vérifiez si custom_network_id est égal à la valeur par défaut
  if (memcmp(custom_network_id, "\xFF\xFF\xFF\xFF", 4) == 0)
  {
    Serial.print("En attente d'association d'un satellite");
    Serial.print("\r");
    // Stockez la nouvelle valeur dans NVS
    byte byteArr[RADIOLIB_SX126X_MAX_PACKET_LENGTH];
    int state = radio.receive(byteArr, 0);
    if (state == RADIOLIB_ERR_NONE)
    {
      int len = radio.getPacketLength();
      Serial.printf("RECEIVED [%2d] : ", len);
      for (int i = 0; i < len; i++)
        Serial.printf("%02X ", byteArr[i]);
      Serial.println("");
      if (len == 11)
      {
        // Copiez les 4 derniers octets du payload dans custom_network_id
        for (int i = 0; i < 4; i++)
        {
          custom_network_id[i] = byteArr[len - 4 + i];
        }
        preferences.begin("net-conf", false);
        preferences.putBytes("net_id", custom_network_id, sizeof(custom_network_id));
        // Maintenant, custom_network_id contient les 4 derniers octets du payload
        Serial.println("Custom Network ID:");
        for (int i = 0; i < sizeof(custom_network_id); i++)
        {
          Serial.printf("%02X ", custom_network_id[i]);
        }
        preferences.end(); // Ferme la mémoire NVS ici
        Serial.println("");
        Serial.print("network_id stockée en mémoire");
        Serial.println("");
        Serial.print("reboot dans 10 seconde");
        delay(10000);
        ESP.restart();
      }
    }
  }
  else
  {
    if (!client.connected())
    {
      connectToMqtt();
    }
    ArduinoOTA.handle();

    // Compteur pour limiter la déclaration des topic
    if (counter >= 100)
    {
      connectToTopic();
      counter = 0;
    }
    counter++;

    char message[255];
    byte byteArr[RADIOLIB_SX126X_MAX_PACKET_LENGTH];
    int state = radio.receive(byteArr, 0);
    if (state == RADIOLIB_ERR_NONE)
    {
      int len = radio.getPacketLength();
      Serial.printf("RECEIVED [%2d] : ", len);
      message[0] = '\0';

      if (len == 23)
      { // Check if the length is 23 bytes

        // Extract bytes 16 and 17
        int decimalValueTemp = byteArr[15] << 8 | byteArr[16];
        float temperatureValue = decimalValueTemp / 10.0;

        // Extract bytes 18 and 19
        int decimalValueCons = byteArr[17] << 8 | byteArr[18];
        float temperatureconsValue = decimalValueCons / 10.0;

        // Publish temperature to the "frisquet_temperature" MQTT topic
        char temperatureTopic[] = "homeassistant/sensor/frisquet/tempAmbiante/state";
        char temperaturePayload[10];
        snprintf(temperaturePayload, sizeof(temperaturePayload), "%.2f", temperatureValue);
        if (!client.publish(temperatureTopic, temperaturePayload))
        {
          Serial.println("Failed to publish temperature to MQTT");
        }
        // Publish temperature to the "tempconsigne" MQTT topic
        char tempconsigneTopic[] = "homeassistant/sensor/frisquet/tempConsigne/state";
        char tempconsignePayload[10];
        snprintf(tempconsignePayload, sizeof(tempconsignePayload), "%.2f", temperatureconsValue);
        if (!client.publish(tempconsigneTopic, tempconsignePayload))
        {
          Serial.println("Failed to publish consigne to MQTT");
        }
      }
      for (int i = 0; i < len; i++)
      {
        sprintf(message + strlen(message), "%02X ", byteArr[i]);
        Serial.printf("%02X ", byteArr[i]);
      }
      if (!client.publish("homeassistant/sensor/frisquet/payload/state", message))
      {
        Serial.println("Failed to publish Payload to MQTT");
      }
      Serial.println("");
    }
  }
  preferences.end();
  client.loop();
  updateDisplay(); // Mettre à jour l'affichage si nécessaire
}
String byteArrayToHexString(uint8_t *byteArray, int length)
{
  String result = "";
  for (int i = 0; i < length; i++)
  {
    char hex[3];
    sprintf(hex, "%02X", byteArray[i]);
    result += hex;
  }
  return result;
}
void initOTA()
{

  ArduinoOTA.setHostname("ESP32 Frisquetconnect");
  ArduinoOTA.setTimeout(25); // Augmenter le délai d'attente à 25 secondes

  ArduinoOTA
      .onStart([]()
               {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

  ArduinoOTA.begin();
}
