#include <Arduino.h>
#include <RadioLib.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <heltec.h>
#include <Preferences.h>
#include <NTPClient.h>
#include <Timezone.h>
#include "image.h"
#include "config.h"

SX1262 radio = new Module(SS, DIO0, RST_LoRa, BUSY_LoRa);
Preferences preferences;
unsigned long lastTxExtSonTime = 0;            // Variable dernière transmission sonde
const unsigned long txExtSonInterval = 600000; // Interval de transmission en millisecondes (10 minutes)
String DateTimeRes;
String tempAmbiante;
String tempExterieure;
String tempConsigne;
String modeFrisquet;
String assSonFrisquet;
String assConFrisquet;
String byteArrayToHexString(uint8_t *byteArray, int length);
byte extSonTempBytes[2];
byte sonMsgNum = 0x01;
int counter = 0;
uint8_t custom_network_id[4];
uint8_t custom_extSon_id;
float temperatureValue;
float temperatureconsValue;
float extSonVal;
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Règle pour l'heure d'été
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};    // Règle pour l'heure d'hiver
Timezone timeZone(CEST, CET);
// Drapeaux pour indiquer si les données ont changé
bool tempAmbianteChanged = false;
bool tempExterieureChanged = false;
bool tempConsigneChanged = false;
bool modeFrisquetChanged = false;
bool assSonFrisquetChanged = false;
bool assConFrisquetChanged = false;
bool eraseNvs = false;
// Constantes pour les topics MQTT
const char *TEMP_AMBIANTE1_TOPIC = "homeassistant/sensor/frisquet/tempAmbiante1/state";
const char *TEMP_EXTERIEURE_TOPIC = "homeassistant/sensor/frisquet/tempExterieure/state";
const char *TEMP_CONSIGNE1_TOPIC = "homeassistant/sensor/frisquet/tempConsigne1/state";
const char *MODE_TOPIC = "homeassistant/select/frisquet/mode/set";
const char *ASS_SON_TOPIC = "homeassistant/switch/frisquet/asssonde/set";
const char *ASS_CON_TOPIC = "homeassistant/switch/frisquet/assconnect/set";
uint8_t TempExTx[] = {0x80, 0x20, 0x00, 0x00, 0x01, 0x17, 0x9c, 0x54, 0x00, 0x04, 0xa0, 0x29, 0x00, 0x01, 0x02, 0x00, 0x00}; // envoi température
byte TxByteArr[10] = {0x80, 0x20, 0x00, 0x00, 0x82, 0x41, 0x01, 0x21, 0x01, 0x02};                                           // association Sonde exterieure
//****************************************************************************
void sendTxByteArr()
{
  int state = radio.transmit(TxByteArr, sizeof(TxByteArr));
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println("Transmission réussie");
    for (int i = 0; i < sizeof(TxByteArr); i++)
    {
      Serial.printf("%02X ", TxByteArr[i]); // Serial.print(TempExTx[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  else
  {
    Serial.println("Erreur lors de la transmission");
  }
}
//****************************************************************************
void updateDisplay()
{
  if (tempAmbianteChanged || tempExterieureChanged || tempConsigneChanged || modeFrisquetChanged)
  {
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "Net: " + byteArrayToHexString(custom_network_id, sizeof(custom_network_id)) + " Son: " + byteArrayToHexString(&custom_extSon_id, 1));
    Heltec.display->drawString(0, 11, "Temp Ambiante: " + tempAmbiante + "°C");
    Heltec.display->drawString(0, 22, "Temp Exterieure: " + tempExterieure + "°C");
    Heltec.display->drawString(0, 33, "Temp Consigne: " + tempConsigne + "°C");
    Heltec.display->drawString(0, 44, "Mode: " + modeFrisquet);

    Heltec.display->display();
    tempAmbianteChanged = false;
    tempExterieureChanged = false;
    tempConsigneChanged = false;
    modeFrisquetChanged = false;
  }
  else if (assSonFrisquetChanged || assConFrisquetChanged)
  {
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "Net: " + byteArrayToHexString(custom_network_id, sizeof(custom_network_id)) + " Son: " + byteArrayToHexString(&custom_extSon_id, 1));
    Heltec.display->drawString(0, 11, "Ass. sonde en cours: " + assSonFrisquet);
    Heltec.display->drawString(0, 22, "Ass. connect en cours: " + assConFrisquet);

    Heltec.display->display();
    assSonFrisquetChanged = false;
    assConFrisquetChanged = false;
  }
}
//****************************************************************************
void callback(char *topic, byte *payload, unsigned int length)
{
  // Convertir le payload en une chaîne de caractères
  char message[length + 1];
  strncpy(message, (char *)payload, length);
  message[length] = '\0';

  // Vérifier le topic et mettre à jour les variables globales
  if (strcmp(topic, TEMP_AMBIANTE1_TOPIC) == 0)
  {
    if (tempAmbiante != String(message))
    {
      tempAmbiante = String(message);
      tempAmbianteChanged = true;
    }
  }
  else if (strcmp(topic, TEMP_EXTERIEURE_TOPIC) == 0)
  {
    if (tempExterieure != String(message))
    {
      tempExterieure = String(message);
      extSonVal = tempExterieure.toFloat() * 10;
      int extSonTemp = int(extSonVal);
      extSonTempBytes[0] = (extSonTemp >> 8) & 0xFF; // Octet de poids fort
      extSonTempBytes[1] = extSonTemp & 0xFF;        // Octet de poids faible
      tempExterieureChanged = true;
    }
  }
  else if (strcmp(topic, TEMP_CONSIGNE1_TOPIC) == 0)
  {
    if (tempConsigne != String(message))
    {
      tempConsigne = String(message);
      tempConsigneChanged = true;
    }
  }
  else if (strcmp(topic, MODE_TOPIC) == 0)
  {
    if (modeFrisquet != String(message))
    {
      modeFrisquet = String(message);
      modeFrisquetChanged = true;
      client.publish("homeassistant/select/frisquet/mode/state", message);
    }
  }
  else if (strcmp(topic, ASS_SON_TOPIC) == 0)
  {
    if (assSonFrisquet != String(message))
    {
      assSonFrisquet = String(message);
      assSonFrisquetChanged = true;
      client.publish("homeassistant/switch/frisquet/asssonde/state", message);
    }
  }
  else if (strcmp(topic, ASS_CON_TOPIC) == 0)
  {
    if (assConFrisquet != String(message))
    {
      assConFrisquet = String(message);
      assConFrisquetChanged = true;
      client.publish("homeassistant/switch/frisquet/assconnect/state", message);
    }
  }
}
//****************************************************************************
void DateTime()
{
  timeClient.update();
  time_t localTime = timeZone.toLocal(timeClient.getEpochTime());
  struct tm *timeinfo = localtime(&localTime);
  int monthDay = timeinfo->tm_mday;
  int currentMonth = timeinfo->tm_mon + 1;
  int currentYear = timeinfo->tm_year + 1900;
  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  int currentSeconde = timeinfo->tm_sec;
  char buffer[15];

  String resfinal;

  sprintf(buffer, "%04d", currentYear);
  resfinal = String(buffer);
  sprintf(buffer, "%02d", currentMonth);
  resfinal = resfinal + String(buffer);
  sprintf(buffer, "%02d", monthDay);
  resfinal = resfinal + String(buffer) + "-";

  sprintf(buffer, "%02d", currentHour);
  resfinal = resfinal + String(buffer);
  sprintf(buffer, "%02d", currentMinute);
  resfinal = resfinal + String(buffer);
  sprintf(buffer, "%02d", currentSeconde);
  resfinal = resfinal + String(buffer);

  DateTimeRes = resfinal;
}
//****************************************************************************
void txConfiguration()
{
  int state = radio.beginFSK();
  state = radio.setFrequency(868.96);
  state = radio.setBitRate(25.0);
  state = radio.setFrequencyDeviation(50.0);
  state = radio.setRxBandwidth(250.0);
  state = radio.setPreambleLength(4);
  state = radio.setSyncWord(custom_network_id, sizeof(custom_network_id));
}
//****************************************************************************
// Fonction pour publier un message MQTT
void publishMessage(const char *topic, const char *payload)
{
  if (!client.publish(topic, payload))
  {
    Serial.println("Failed to publish message to MQTT");
  }
}
//****************************************************************************
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
//****************************************************************************
void connectToSensor(const char *topic, const char *name)
{
  char configTopic[60];
  char configPayload[350];
  snprintf(configTopic, sizeof(configTopic), "homeassistant/sensor/frisquet/%s/config", topic);
  snprintf(configPayload, sizeof(configPayload), R"(
{
  "uniq_id": "frisquet_%s",
  "name": "Frisquet - Temperature %s",
  "state_topic": "homeassistant/sensor/frisquet/%s/state",
  "unit_of_measurement": "°C",
  "device_class": "temperature",
  "device":{"ids":["FrisquetConnect"],"mf":"Frisquet","name":"Frisquet Connect","mdl":"Frisquet Connect"}
}
)",
           topic, name, topic);
  client.publish(configTopic, configPayload);
}
//****************************************************************************
void connectToSwitch(const char *topic, const char *name)
{
  char configTopic[60];
  char configPayload[400];
  snprintf(configTopic, sizeof(configTopic), "homeassistant/switch/frisquet/%s/config", topic);
  snprintf(configPayload, sizeof(configPayload), R"(
{
  "uniq_id": "frisquet_%s",
  "name": "Frisquet - %s",
  "state_topic": "homeassistant/switch/frisquet/%s/state",
  "command_topic": "homeassistant/switch/frisquet/%s/set",
  "payload_on": "ON",
  "payload_off": "OFF",
  "device":{"ids":["FrisquetConnect"],"mf":"Frisquet","name":"Frisquet Connect","mdl":"Frisquet Connect"}
}
)",
           topic, name, topic, topic);
  client.publish(configTopic, configPayload);
}
//****************************************************************************
void connectToTopic()
{
  connectToSensor("tempAmbiante1", "ambiante Z1");
  connectToSensor("tempExterieure", "exterieure");
  connectToSensor("tempConsigne1", "consigne Z1");
  connectToSwitch("asssonde", "ass. sonde");
  connectToSwitch("assconnect", "ass. connect");
  if (sensorZ2 == true)
  {
    connectToSensor("tempAmbiante2", "ambiante Z2");
    connectToSensor("tempConsigne2", "consigne Z2");
    connectToSensor("tempCDC", "CDC");
    connectToSensor("tempECS", "ECS");
    connectToSensor("tempDepart", "départ");
  }
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
  char modeConfigTopic[] = "homeassistant/select/frisquet/mode/config";
  char modeConfigPayload[] = R"({
        "uniq_id": "frisquet_mode",
        "name": "Frisquet - Mode",
        "state_topic": "homeassistant/select/frisquet/mode/state",
        "command_topic": "homeassistant/select/frisquet/mode/set",
        "options": ["Auto", "Confort", "Réduit", "Hors gel"],
        "device":{"ids":["FrisquetConnect"],"mf":"Frisquet","name":"Frisquet Connect","mdl":"Frisquet Connect"}
      })";
  client.publish(modeConfigTopic, modeConfigPayload, true); // true pour retenir le message
  // Souscrire aux topics temp ambiante, consigne, ext et mode
  client.subscribe(MODE_TOPIC);
  client.subscribe(ASS_SON_TOPIC);
  client.subscribe(ASS_CON_TOPIC);
  client.subscribe(TEMP_AMBIANTE1_TOPIC);
  client.subscribe(TEMP_EXTERIEURE_TOPIC);
  client.subscribe(TEMP_CONSIGNE1_TOPIC);
}
void setDefaultNetwork()
{
  memcpy(custom_network_id, def_Network_id, sizeof(def_Network_id));
}
//****************************************************************************
void txExtSonTemp()
{
  sonMsgNum += 4;
  // Remplacer les bytes spécifiés dans TempExTx
  TempExTx[2] = custom_extSon_id;
  TempExTx[3] = sonMsgNum;
  TempExTx[15] = extSonTempBytes[0]; // Remplacer le 16ème byte par l'octet de poids fort de extSonTemp
  TempExTx[16] = extSonTempBytes[1]; // Remplacer le 17ème byte par l'octet de poids faible de extSonTemp
  // Afficher le payload dans la console
  Serial.print("Payload Sonde transmit: ");
  for (int i = 0; i < sizeof(TempExTx); i++)
  {
    Serial.printf("%02X ", TempExTx[i]);
    Serial.print(" ");
  }
  Serial.println();
  // Transmettre la chaine TempExTx
  int state = radio.transmit(TempExTx, sizeof(TempExTx));
}
//****************************************************************************
void assExtSonde()
{
  if (memcmp(custom_network_id, def_Network_id, sizeof(def_Network_id)) != 0)
  {
    setDefaultNetwork();
    txConfiguration();
  }
  Serial.print("En attente d'association...");
  Serial.print("\r");
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
      TxByteArr[2] = byteArr[2];
      TxByteArr[3] = byteArr[3];
      TxByteArr[4] = byteArr[4] | 0x80; // Ajouter 0x80 au 5eme byte
      delay(100);
      // envoi de la chaine d'association
      sendTxByteArr();
      // Copiez les 4 derniers octets du payload dans custom_network_id
      for (int i = 0; i < 4; i++)
      {
        custom_network_id[i] = byteArr[len - 4 + i];
      }
      preferences.begin("net-conf", false);
      preferences.putBytes("net_id", custom_network_id, sizeof(custom_network_id));
      preferences.putUChar("son_id", byteArr[2]); // Nouvelle entrée pour le byte 3
      custom_extSon_id = preferences.getUChar("son_id", 0);
      // Maintenant, custom_network_id contient les 4 derniers octets du payload et son_id est stocké en NVS
      Serial.print("Custom Network ID: ");
      for (int i = 0; i < sizeof(custom_network_id); i++)
      {
        Serial.printf("%02X ", custom_network_id[i]);
      }
      Serial.println("");
      Serial.print("custom ext Son ID: ");
      Serial.printf("%02X ", custom_extSon_id);
      preferences.end(); // Ferme la mémoire NVS
      publishMessage(ASS_SON_TOPIC, "OFF");
      publishMessage("homeassistant/switch/frisquet/asssonde/state", "OFF");
      assSonFrisquet = "OFF";
      Serial.println("");
      Serial.print("association effectuée !");
      Serial.println("");
      txConfiguration();
      Serial.print("reprise de la boucle initiale");
      Serial.println("");
    }
  }
}
//****************************************************************************
void initOTA();
//****************************************************************************
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
  timeClient.begin(); // Démarre le client NTP
  // Démarre l'instance de NVS
  preferences.begin("net-conf", false); // "customId" est le nom de l'espace de stockage
  if (eraseNvs)
  {
    preferences.begin("net-conf", false); // Ouvrir la mémoire NVS en mode lecture/écriture
    preferences.clear();                  // Effacer complètement la mémoire NVS
  }
  // Déclare l'id de sonde externe
  custom_extSon_id = (extSon_id == 0xFF) ? preferences.getUChar("son_id", 0) : extSon_id;
  // Vérifie si la clé "custom_network_id" existe dans NVS
  size_t custom_network_id_size = preferences.getBytes("net_id", custom_network_id, sizeof(custom_network_id));
  // Vérifie si custom_network_id est différent de {0xFF, 0xFF, 0xFF, 0xFF} ou si la clé n'existe pas dans NVS
  if (custom_network_id_size != sizeof(custom_network_id) || memcmp(custom_network_id, "\xFF\xFF\xFF\xFF", sizeof(custom_network_id)) == 0)
  {
    // Copie la valeur de network_id ou def_Network_id dans custom_network_id
    memcpy(custom_network_id, (memcmp(network_id, "\xFF\xFF\xFF\xFF", sizeof(network_id)) != 0) ? network_id : def_Network_id, sizeof(custom_network_id));
    // Enregistre custom_network_id dans NVS
    preferences.putBytes("net_id", custom_network_id, sizeof(custom_network_id));
  }
  // Initialize OLED display
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->drawXbm(0, 0, 128, 64, myLogo);
  Heltec.display->display();

  txConfiguration();
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
//****************************************************************************
void loop()
{
  // DateTime();
  // Vérifiez si custom_network_id est égal à la valeur par défaut
  if (assSonFrisquet == "ON")
  {
    assExtSonde();
  }
  else
  {
    unsigned long currentTime = millis();
    // Vérifier si 10 minutes se sont écoulées depuis la dernière transmission
    if (currentTime - lastTxExtSonTime >= txExtSonInterval)
    {
      // Vérifier si custom_extSon_id n'est pas égal à 0 byte
      if (memcmp(custom_network_id, "\xFF\xFF\xFF\xFF", 4) != 0 && custom_extSon_id != 0x00)
      {
        txExtSonTemp(); // Appeler la fonction pour transmettre les données
      }
      else
      {
        Serial.println("Id sonde externe non connue");
      }
      // Mettre à jour le temps de la dernière transmission
      lastTxExtSonTime = currentTime;
    }
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
        char temperaturePayload[10];
        snprintf(temperaturePayload, sizeof(temperaturePayload), "%.2f", temperatureValue);
        publishMessage(TEMP_AMBIANTE1_TOPIC, temperaturePayload);
        // Publish temperature to the "tempconsigne" MQTT topic
        char tempconsignePayload[10];
        snprintf(tempconsignePayload, sizeof(tempconsignePayload), "%.2f", temperatureconsValue);
        publishMessage(TEMP_CONSIGNE1_TOPIC, tempconsignePayload);
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
  client.loop();
  updateDisplay(); // Mettre à jour l'affichage si nécessaire
}
//************************************************************
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
//************************************************************
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
