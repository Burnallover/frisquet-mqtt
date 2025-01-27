#define DEBUG 0 // Mettre à 0 pour désactiver les impressions série, 1 pour les activer

#if DEBUG
#define DBG_PRINT(x) Serial.print(x)
#define DBG_PRINTLN(x) Serial.println(x)
#else
#define DBG_PRINT(x)
#define DBG_PRINTLN(x)
#endif
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
#include "conf.h"
volatile bool receivedFlag = false;
#define SS GPIO_NUM_8
#define RST_LoRa GPIO_NUM_12
#define BUSY_LoRa GPIO_NUM_13
#define DIO0 GPIO_NUM_14

SX1262 radio = new Module(SS, DIO0, RST_LoRa, BUSY_LoRa);
Preferences preferences;
unsigned long lastTxExtSonTime = 30000;        // Variable dernière transmission sonde ajout de 30 sec d'intervale pour ne pas envoyé en meme temps que le connect
const unsigned long txExtSonInterval = 600000; // Interval de transmission en millisecondes (10 minutes)
unsigned long lastConMsgTime = 0;
const unsigned long conMsgInterval = 600000; // 10 minutes
unsigned long lastconMsgInterval = 0;        // pour l'interval de 1 seconde entre les messages du Con a cha
String DateTimeRes;
String tempAmbiante;
String tempExterieure;
String tempConsigne;
String modeFrisquet;
String modeFrisquet2;
String tempAmbiante2;
String tempConsigne2;
String assSonFrisquet;
String assConFrisquet;
String eraseNvsFrisquet;
String byteArrayToHexString(uint8_t *byteArray, int length);
byte extSonTempBytes[2];
byte sonMsgNum = 0x01;
byte conMsgNum = 0x03;
byte msg79e0 = 0x00;
byte msg7a18 = 0x00;
int counter = 0;
uint8_t custom_network_id[4];
uint8_t custom_extSon_id;
uint8_t custom_friCon_id;
float temperatureValue;
float temperatureconsValue;
float extSonVal;
WiFiClient espClient;
PubSubClient client(espClient);
bool waitingForResponse = false;
bool waitingForResponse2 = false;
bool sequenceMsg = true;
unsigned long startWaitTime = 0;
unsigned long lastTxModeTime = 0;
const unsigned long maxWaitTime = 360000; // 6 minutes en ms
const unsigned long retryInterval = 2500; // 2.5 secondes en ms
const char hexDigits[] = "0123456789ABCDEF";
// Drapeaux pour indiquer si les données ont changé
bool tempAmbianteChanged = false;
bool tempAmbiante2Changed = false;
bool tempExterieureChanged = false;
bool tempConsigneChanged = false;
bool tempConsigne2Changed = false;
bool modeFrisquetChanged = false;
bool modeFrisquet2Changed = false;
bool assSonFrisquetChanged = false;
bool assConFrisquetChanged = false;
// Constantes pour les topics MQTT
const char *TEMP_AMBIANTE1_TOPIC = "homeassistant/sensor/frisquet/tempAmbiante1/state";
const char *TEMP_EXTERIEURE_TOPIC = "homeassistant/sensor/frisquet/tempExterieure/state";
const char *TEMP_CONSIGNE1_TOPIC = "homeassistant/sensor/frisquet/tempConsigne1/state";
const char *TEMP_AMBIANTE2_TOPIC = "homeassistant/sensor/frisquet/tempAmbiante2/state";
const char *TEMP_CONSIGNE2_TOPIC = "homeassistant/sensor/frisquet/tempConsigne2/state";
const char *MODE_TOPIC_STATE = "homeassistant/select/frisquet/mode/state";
const char *MODE_TOPIC2_STATE = "homeassistant/select/frisquet/mode2/state";
const char *MODE_TOPIC = "homeassistant/select/frisquet/mode/set";
const char *MODE_TOPIC2 = "homeassistant/select/frisquet/mode2/set";
const char *ASS_SON_TOPIC = "homeassistant/switch/frisquet/asssonde/set";
const char *ASS_CON_TOPIC = "homeassistant/switch/frisquet/assconnect/set";
const char *RES_NVS_TOPIC = "homeassistant/switch/frisquet/erasenvs/set";
uint8_t TempExTx[] = {0x80, 0x20, 0x00, 0x00, 0x01, 0x17, 0x9c, 0x54, 0x00, 0x04, 0xa0, 0x29, 0x00, 0x01, 0x02, 0x00, 0x00}; // envoi température
byte TxByteArr[10] = {0x80, 0x20, 0x00, 0x00, 0x82, 0x41, 0x01, 0x21, 0x01, 0x02};                                           // association Sonde exterieure
byte TxByteArrFriCon[10] = {0x80, 0x7e, 0x00, 0x00, 0x82, 0x41, 0x01, 0x21, 0x01, 0x02};                                     // association Frisquet connect
byte TxByteArrCon1[10] = {0x80, 0x7e, 0x21, 0xE0, 0x01, 0x03, 0xA0, 0x2B, 0x00, 0x04};                                       // message A0	2B	00	04 connect to chaudiere
byte TxByteArrCon2[10] = {0x80, 0x7e, 0x21, 0xE0, 0x01, 0x03, 0x79, 0xE0, 0x00, 0x1C};                                       // message 79	E0	00	1C connect to chaudiere
byte TxByteArrCon3[10] = {0x80, 0x7e, 0x21, 0xE0, 0x01, 0x03, 0x7A, 0x18, 0x00, 0x1C};                                       // message 7A	18	00	1C connect to chaudiere
byte TxByteArrCon4[10] = {0x80, 0x7e, 0x21, 0xE0, 0x01, 0x03, 0x7A, 0x34, 0x00, 0x1C};                                       // message 7A	34	00	1C connect to chaudiere
byte TxByteArrCon5[10] = {0x80, 0x7e, 0x21, 0xE0, 0x01, 0x03, 0x79, 0xFC, 0x00, 0x1C};                                       // message 79	FC	00	1C connect to chaudiere

byte TxByteArrConRep[49] = {
    0x80, 0x7E, 0x39, 0x18, 0x88, 0x17, 0x2A, 0x91, 0x6E, 0x1E, 0x05, 0x21, 0x00, 0x00, 0xE0, 0xFF,
    0xFF, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0x1F, 0x00,
    0xE0, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0xFF, 0xFF,
    0x1F};
byte TxByteArrConMod[63] = {
    0x80, 0x7E, 0x39, 0x18, 0x08, 0x17, 0xA1, 0x54, 0x00, 0x18, 0xA1, 0x54, 0x00, 0x18, 0x30, 0x91,
    0x6E, 0x1E, 0x08, 0x21, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0xFF, 0xFF,
    0x1F, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0xE0, 0xFF,
    0xFF, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0x1F};
//****************************************************************************
// Array pour envoyé les trames au connect avec un loop qui espace
int conMsgIndex = 0;
int conMsgToSendCount = 0;

byte *conMsgArrays[] = {
    TxByteArrCon1,
    TxByteArrCon2,
    TxByteArrCon3,
    TxByteArrCon4,
    TxByteArrCon5};
const int conMsgCount = 4;
const int sequenceA[] = {0, 1, 2, 4}; // Correspond à TxByteArrCon1, TxByteArrCon2, TxByteArrCon3, TxByteArrCon5
const int sequenceB[] = {0, 1, 3, 4}; // Correspond à TxByteArrCon1, TxByteArrCon2, TxByteArrCon4, TxByteArrCon5
//****************************************************************************
// Fonction pour publier un message MQTT
void publishMessage(const char *topic, const char *payload)
{
  if (!client.publish(topic, payload))
  {
    DBG_PRINTLN(F("Failed to publish message to MQTT"));
  }
}
//****************************************************************************
void eraseNvs()
{
  preferences.begin("net-conf", false); // Ouvrir la mémoire NVS en mode lecture/écriture
  preferences.clear();                  // Effacer complètement la mémoire NVS
  publishMessage("homeassistant/switch/frisquet/erasenvs/state", "OFF");
  eraseNvsFrisquet = "OFF";
}
//****************************************************************************
void initNvs()
{
  // Démarre l'instance de NVS
  preferences.begin("net-conf", false); // "customId" est le nom de l'espace de stockage
  // Déclare l'id de sonde externe et du connect
  custom_extSon_id = (extSon_id == 0xFF) ? preferences.getUChar("son_id", 0) : extSon_id;
  custom_friCon_id = (friCon_id == 0xFF) ? preferences.getUChar("con_id", 0) : friCon_id;
  // Vérifie si la clé "custom_network_id" existe dans NVS
  size_t custom_network_id_size = preferences.getBytes("net_id", custom_network_id, sizeof(custom_network_id));
  // Vérifie si custom_network_id est différent de {0xFF, 0xFF, 0xFF, 0xFF} ou si la clé n'existe pas dans NVS
  if (custom_network_id_size != sizeof(custom_network_id) || memcmp(custom_network_id, "\xFF\xFF\xFF\xFF", sizeof(custom_network_id)) == 0)
  {
    // Copie la valeur de network_id ou def_Network_id dans custom_network_id
    memcpy(custom_network_id, (memcmp(network_id, "\xFF\xFF\xFF\xFF", sizeof(network_id)) != 0) ? network_id : def_Network_id, sizeof(custom_network_id));
    // Enregistre custom_network_id dans NVS
    preferences.putBytes("net_id", custom_network_id, sizeof(custom_network_id));
    preferences.end(); // Ferme la mémoire NVS
  }
}
//****************************************************************************
void updateDisplay()
{
  if (tempAmbianteChanged || tempExterieureChanged || tempConsigneChanged || modeFrisquetChanged || tempAmbiante2Changed || tempConsigne2Changed || modeFrisquet2Changed)
  {
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "Net: " + byteArrayToHexString(custom_network_id, sizeof(custom_network_id)));
    Heltec.display->drawString(0, 11, "SonID: " + byteArrayToHexString(&custom_extSon_id, 1) + " ConID: " + byteArrayToHexString(&custom_friCon_id, 1));
    Heltec.display->drawString(0, 22, "T° Amb1: " + tempAmbiante + "°C " + "T° Ext: " + tempExterieure + "°C");
    Heltec.display->drawString(0, 33, "T° Con1: " + tempConsigne + "°C" + "Mode: " + modeFrisquet);

    if(sensorZ2) {
      Heltec.display->drawString(0, 44, "T° Amb2: " + tempAmbiante2 + "°C");
      Heltec.display->drawString(0, 55, "T° Con2: " + tempConsigne2 + "°C");

    tempAmbiante2Changed = false;
    tempConsigne2Changed = false;
    modeFrisquet2Changed = false;
    }

    Heltec.display->display();
    tempAmbianteChanged = false;
    tempExterieureChanged = false;
    tempConsigneChanged = false;
    modeFrisquetChanged = false;

  }
  else if (assSonFrisquetChanged || assConFrisquetChanged)
  {
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "Net: " + byteArrayToHexString(custom_network_id, sizeof(custom_network_id)));
    Heltec.display->drawString(0, 11, "SonID: " + byteArrayToHexString(&custom_extSon_id, 1) + " ConID: " + byteArrayToHexString(&custom_friCon_id, 1));
    Heltec.display->drawString(0, 22, "Ass. sonde en cours: " + assSonFrisquet);
    Heltec.display->drawString(0, 33, "Ass. connect en cours: " + assConFrisquet);

    Heltec.display->display();
    assSonFrisquetChanged = false;
    assConFrisquetChanged = false;
  }
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
void handleModeChange(const char *newMode,uint8_t zone)
{
  // Déterminer la valeur du mode à transmettre avec un switch
  uint8_t modeValue1 = 0x00; // Par défaut, valeur invalide
  uint8_t modeValue2 = 0x00;
  bool modeValid = true;

  switch (newMode[0]) // Utiliser le premier caractère pour optimiser
  {
  case 'A': // "Auto"
    modeValue1 = 0x05;
    modeValue2 = 0x21;
    break;
  case 'C': // "Confort"
    modeValue1 = 0x06;
    modeValue2 = 0x21;
    break;
  case 'R': // "Réduit"
    modeValue1 = 0x07;
    modeValue2 = 0x21;
    break;
  case 'H': // "Hors gel"
    modeValue1 = 0x08;
    modeValue2 = 0x21;
    break;
  default:
    modeValid = false; // Mode non valide
    break;
  }

  if (!modeValid)
  {
    DBG_PRINTLN("Mode non reconnu !");
    return; // Sortir si le mode est inconnu
  }

  // Assigner la valeur du mode à TxByteArrConMod
  TxByteArrConMod[2] = custom_friCon_id;
  TxByteArrConMod[3] = conMsgNum;
  TxByteArrConMod[4]= zone;
  TxByteArrConMod[18] = modeValue1;
  TxByteArrConMod[19] = modeValue2;

  conMsgNum += 1;
  // Si conMsgNum dépasse 255, le remettre à une valeur valide (par exemple, 3)
  if (conMsgNum > 0xFF)
  {
    conMsgNum = 0x03; // Recommencer à 3 pour maintenir le décalage
  }
  // Envoyer la chaîne via radio
  int state = radio.transmit(TxByteArrConMod, sizeof(TxByteArrConMod));
  if (state == RADIOLIB_ERR_NONE)
  {
    if (zone == 0x09)
    {
      waitingForResponse2 = true;
    }
    else
    {
      waitingForResponse = true;
    }
  }
  else
  {
    DBG_PRINTLN("Erreur lors de l'envoi du mode !");
  }
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE)
  {
    DBG_PRINTLN(F("startreceive success!"));
  }
  else
  {
    DBG_PRINT(F("failed, code "));
    DBG_PRINTLN(state);
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
  else if (strcmp(topic, TEMP_AMBIANTE2_TOPIC) == 0)
  {
    if (tempAmbiante2 != String(message))
    {
      tempAmbiante2 = String(message);
      tempAmbianteChanged = true; // @todo
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
  else if (strcmp(topic, TEMP_CONSIGNE2_TOPIC) == 0)
  {
    if (tempConsigne2 != String(message))
    {
      tempConsigne2 = String(message);
      tempConsigneChanged = true;
    }
  }
  else if (strcmp(topic, MODE_TOPIC) == 0)
  {
    if (modeFrisquet != String(message))
    {
      modeFrisquet = String(message);
      modeFrisquetChanged = true;
      handleModeChange(message,0x08);
      startWaitTime = millis(); // On note le début de l’attente
    }
  }
  else if (strcmp(topic, MODE_TOPIC2) == 0)
  {
    if (modeFrisquet2 != String(message))
    {
      modeFrisquet2 = String(message);
      modeFrisquet2Changed = true;
      handleModeChange(message,0x09);
      startWaitTime = millis(); // On note le début de l’attente
    }
  }
  else if (strcmp(topic, ASS_SON_TOPIC) == 0)
  {
    if (assSonFrisquet != String(message))
    {
      assSonFrisquet = String(message);
      assSonFrisquetChanged = true;
      client.publish("homeassistant/switch/frisquet/asssonde/state", message);
      if (assSonFrisquet == "OFF")
      {
        initNvs();
      }
    }
  }
  else if (strcmp(topic, ASS_CON_TOPIC) == 0)
  {
    if (assConFrisquet != String(message))
    {
      assConFrisquet = String(message);
      assConFrisquetChanged = true;
      client.publish("homeassistant/switch/frisquet/assconnect/state", message);
      if (assConFrisquet == "OFF")
      {
        initNvs();
      }
    }
  }
  else if (strcmp(topic, RES_NVS_TOPIC) == 0)
  {
    if (eraseNvsFrisquet != String(message))
    {
      eraseNvsFrisquet = String(message);
      client.publish("homeassistant/switch/frisquet/erasenvs/state", message);
    }
  }
}
//****************************************************************************
void connectToMqtt()
{
  if (!client.connected())
  {
    DBG_PRINTLN(F("Connecting to MQTT..."));
    if (client.connect("ESP32 Frisquet", mqttUsername, mqttPassword))
    {
      DBG_PRINTLN(F("Connected to MQTT"));
      // Vous pouvez publier des messages ou souscrire à des topics ici si nécessaire
    }
    else
    {
      DBG_PRINT(F("Failed to connect MQTT, rc="));
      DBG_PRINT(client.state());
      DBG_PRINTLN(F(" Retrying in 5 seconds..."));
      unsigned long retryTime = millis() + 5000;
      while (millis() < retryTime)
      {
        client.loop();
      }
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
  "name": "Temperature %s",
  "state_topic": "homeassistant/sensor/frisquet/%s/state",
  "unit_of_measurement": "°C",
  "device_class": "temperature",
  "device":{"ids":["Frisquet_MQTT"],"mf":"HA Community","name":"Frisquet MQTT","mdl":"ESP32 Heltec"}
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
  "name": "%s",
  "state_topic": "homeassistant/switch/frisquet/%s/state",
  "command_topic": "homeassistant/switch/frisquet/%s/set",
  "payload_on": "ON",
  "payload_off": "OFF",
  "device":{"ids":["Frisquet_MQTT"],"mf":"HA Community","name":"Frisquet MQTT","mdl":"ESP32 Heltec"}
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
  connectToSensor("tempCDC", "corps de chauffe");
  
  connectToSensor("tempDepart", "depart");
  connectToSwitch("asssonde", "ass. sonde");
  connectToSwitch("assconnect", "ass. connect");
  connectToSwitch("erasenvs", "erase NVS");
  if (sensorecs == true)
  {
    connectToSensor("tempECS", "eau chaude sanitaire");
  }
  if (sensorZ2 == true)
  {
    connectToSensor("tempAmbiante2", "ambiante Z2");
    connectToSensor("tempConsigne2", "consigne Z2");
  }
  // Configuration récupération Payload
  char payloadConfigTopic[] = "homeassistant/sensor/frisquet/payload/config";
  char payloadConfigPayload[] = R"(
{
  "uniq_id": "frisquet_payload",
  "name": "Payload",
  "state_topic": "homeassistant/sensor/frisquet/payload/state",
  "device":{"ids":["Frisquet_MQTT"],"mf":"HA Community","name":"Frisquet MQTT","mdl":"ESP32 Heltec"}
}
)";
  client.publish(payloadConfigTopic, payloadConfigPayload);

  // Publier le message de configuration pour MQTT du mode
  char modeConfigTopic[] = "homeassistant/select/frisquet/mode/config";
  char modeConfigPayload[] = R"({
        "uniq_id": "frisquet_mode",
        "name": "Mode",
        "state_topic": "homeassistant/select/frisquet/mode/state",
        "command_topic": "homeassistant/select/frisquet/mode/set",
        "options": ["Auto", "Confort", "Réduit", "Hors gel"],
        "device":{"ids":["Frisquet_MQTT"],"mf":"HA Community","name":"Frisquet MQTT","mdl":"ESP32 Heltec"}
      })";
  client.publish(modeConfigTopic, modeConfigPayload, true); // true pour retenir le message
  
  // Publier le message de configuration pour MQTT du mode pour la zone 2 si active
  if (sensorZ2 == true)
  { 
    char modeConfigTopic2[] = "homeassistant/select/frisquet/mode2/config";
    char modeConfigPayload2[] = R"({
          "uniq_id": "frisquet_mode2",
          "name": "Mode Zone 2",
          "state_topic": "homeassistant/select/frisquet/mode2/state",
          "command_topic": "homeassistant/select/frisquet/mode2/set",
          "options": ["Auto", "Confort", "Réduit", "Hors gel"],
          "device":{"ids":["Frisquet_MQTT"],"mf":"HA Community","name":"Frisquet MQTT","mdl":"ESP32 Heltec"}
        })";
    client.publish(modeConfigTopic2, modeConfigPayload2, true); // true pour retenir le message
  }
  // Publier le message de configuration pour MQTT de la consommation gaz pour le chauffage
  char consoChConfigTopic[] = "homeassistant/sensor/frisquet/consogaz-ch/config";
  char consoChConfigPayload[] = R"({
        "uniq_id": "frisquet_consogaz_chauffage",
        "name": "consommation gaz chauffage",
        "state_topic": "homeassistant/sensor/frisquet/consogaz-ch/state",
        "unit_of_measurement": "kWh",
        "device_class": "energy",
        "state_class": "total_increasing",
        "device":{"ids":["Frisquet_MQTT"],"mf":"HA Community","name":"Frisquet MQTT","mdl":"ESP32 Heltec"}
      })";
  client.publish(consoChConfigTopic, consoChConfigPayload, true); // true pour retenir le message

   // Publier le message de configuration pour MQTT de la consommation gaz pour l'eau chaude
  if (sensorecs == true)
  { 
    char consoEcsConfigTopic[] = "homeassistant/sensor/frisquet/consogaz-ecs/config";
    char consoEcsConfigPayload[] = R"({
          "uniq_id": "frisquet_consogaz_ecs",
          "name": "consommation gaz ECS",
          "state_topic": "homeassistant/sensor/frisquet/consogaz-ecs/state",
          "unit_of_measurement": "kWh",
          "device_class": "energy",
          "state_class": "total_increasing",
          "device":{"ids":["Frisquet_MQTT"],"mf":"HA Community","name":"Frisquet MQTT","mdl":"ESP32 Heltec"}
        })";
    client.publish(consoEcsConfigTopic, consoEcsConfigPayload, true); // true pour retenir le message
  }

  // Souscrire aux topics temp ambiante, consigne, ext et mode
  client.subscribe(MODE_TOPIC);
  client.subscribe(ASS_SON_TOPIC);
  client.subscribe(ASS_CON_TOPIC);
  client.subscribe(TEMP_AMBIANTE1_TOPIC);
  client.subscribe(TEMP_EXTERIEURE_TOPIC);
  client.subscribe(TEMP_CONSIGNE1_TOPIC);

  if(sensorZ2) {
    client.subscribe(TEMP_CONSIGNE2_TOPIC);
    client.subscribe(TEMP_AMBIANTE2_TOPIC);
    client.subscribe(MODE_TOPIC2);
  }
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

  DBG_PRINTLN();
  // Transmettre la chaine TempExTx
  int state = radio.transmit(TempExTx, sizeof(TempExTx));
  if (state == RADIOLIB_ERR_NONE)
  {
    DBG_PRINTLN(F("Transmission temp. ext. réussie"));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE)
    {
      DBG_PRINTLN(F("startreceive success!"));
    }
    else
    {
      DBG_PRINT(F("failed, code "));
      DBG_PRINTLN(state);
    }
  }
  else
  {
    DBG_PRINTLN(F("Erreur trans. temp ext."));
    DBG_PRINTLN(state);
  }
}
//****************************************************************************
void txfriConMsg()
{
  // Insérer conMsgNum et custom_friCon_id dans les trames à envoyer
  if (millis() - lastconMsgInterval >= 2000) // 4000 ms = 4 secondes
  {
    const int *currentSequence = sequenceMsg ? sequenceA : sequenceB;
    int trameIndex = currentSequence[conMsgIndex];
    conMsgArrays[trameIndex][2] = custom_friCon_id;
    conMsgArrays[trameIndex][3] = conMsgNum;
    // envoi de la trame
    int state = radio.transmit(conMsgArrays[trameIndex], 10); // 10 est la taille de chaque tableau TxByteArrConX
    if (state == RADIOLIB_ERR_NONE)
    {
      DBG_PRINTLN(F("Transmission msg. Con. réussie"));
      if (conMsgIndex == 1)
      {
        msg79e0 = conMsgNum;
      };
      if (trameIndex == 2)
      {
        msg7a18 = conMsgNum;
      };
      state = radio.startReceive();
      if (state == RADIOLIB_ERR_NONE)
      {
        DBG_PRINTLN(F("startreceive success!"));
      }
      else
      {
        DBG_PRINT(F("failed, code "));
        DBG_PRINTLN(state);
      }
    }
    else
    {
      DBG_PRINTLN(F("Erreur lors de la transmission Con"));
    }

    // Incrémenter conMsgNum de 4 et gérer le débordement
    conMsgNum += 4;
    // Si conMsgNum dépasse 255, le remettre à une valeur valide
    if (conMsgNum > 0xFF)
    {
      conMsgNum = 0x03; // Recommencer à 3 pour maintenir le décalage
    }

    // Préparer la prochaine trame
    conMsgIndex++;
    conMsgToSendCount--;
    lastconMsgInterval = millis();
  }
  else
  {
    // Attente si le délai de 1 secondes n'est pas encore atteint
  }
}
//****************************************************************************
bool associateDevice(
    uint8_t *deviceTxArr, size_t deviceTxArrLen, // Tableau et taille de la payload d'association
    const uint8_t *defaultNetworkId,             // Réseau par défaut
    const char *idKey,                           // Clé NVS pour stocker l'ID du device (ex: "son_id" ou "con_id")
    const char *assCommandTopic,                 // Topic MQTT sur lequel on a reçu la demande d'association (ex: ASS_SON_TOPIC)
    const char *assStateTopic                    // Topic MQTT pour publier l'état OFF après association
)
{
  // Vérifier si le réseau est par défaut, sinon le réinitialiser
  if (memcmp(custom_network_id, defaultNetworkId, sizeof(custom_network_id)) != 0)
  {
    setDefaultNetwork();
    txConfiguration();
  }

  DBG_PRINT(F("En attente d'association...\r"));

  byte byteArr[RADIOLIB_SX126X_MAX_PACKET_LENGTH];
  int state = radio.receive(byteArr, 0);
  if (state == RADIOLIB_ERR_NONE)
  {
    int len = radio.getPacketLength();
    Serial.printf("RECEIVED [%2d] : ", len);
    for (int i = 0; i < len; i++)
      Serial.printf("%02X ", byteArr[i]);
    DBG_PRINTLN();

    // Vérifier la longueur attendue (ici 11 comme pour l'extSon) ou adapter selon le device
    if (len == 11)
    {
      // Adapter le payload à partir du message reçu
      deviceTxArr[2] = byteArr[2];
      deviceTxArr[3] = byteArr[3];
      deviceTxArr[4] = byteArr[4] | 0x80; // Ajouter 0x80 au 5eme byte
      deviceTxArr[5] = byteArr[5];
      deviceTxArr[6] = byteArr[6];

      // Envoi de la chaine d'association
      int txState = radio.transmit(deviceTxArr, deviceTxArrLen);
      if (txState == RADIOLIB_ERR_NONE)
      {
        // Mise à jour du custom_network_id
        for (int i = 0; i < 4; i++)
        {
          custom_network_id[i] = byteArr[len - 4 + i];
        }

        preferences.begin("net-conf", false);
        preferences.putBytes("net_id", custom_network_id, sizeof(custom_network_id));
        preferences.putUChar(idKey, byteArr[2]); // Stocker l'ID du device
        uint8_t deviceId = preferences.getUChar(idKey, 0);
        preferences.end();

        DBG_PRINT(F("Custom Network ID: "));
        for (int i = 0; i < sizeof(custom_network_id); i++)
        {
          Serial.printf("%02X ", custom_network_id[i]);
        }
        DBG_PRINTLN();

        DBG_PRINT(F("Custom Device ID: "));
        Serial.printf("%02X ", deviceId);
        DBG_PRINTLN();

        // Publier sur MQTT pour indiquer que l'association est terminée (mettre l'état OFF)
        publishMessage(assCommandTopic, "OFF");
        publishMessage(assStateTopic, "OFF");

        DBG_PRINTLN(F("Association effectuée !"));
        txConfiguration();
        DBG_PRINTLN(F("Reprise de la boucle initiale"));
        return true;
      }
      else
      {
        DBG_PRINTLN(F("Erreur lors de la transmission de la trame d'association"));
        return false;
      }
    }
    else
    {
      DBG_PRINTLN(F("Taille du message inattendue pour l'association."));
      return false;
    }
  }
  else
  {
    DBG_PRINTLN(F("Aucun message reçu ou erreur radio."));
    return false;
  }
}
//****************************************************************************
bool assExtSon()
{
  bool result = associateDevice(
      TxByteArr, sizeof(TxByteArr),
      def_Network_id,
      "son_id",
      ASS_SON_TOPIC,
      "homeassistant/switch/frisquet/asssonde/state");

  if (result)
  {
    DBG_PRINTLN("Association de la sonde réussie !");
    assSonFrisquet = "OFF";
  }
  else
  {
    DBG_PRINTLN("Echec de l'association de la sonde !");
    // Action en cas d'échec, ex. retenter ou notifier
  }
  return result;
}
//****************************************************************************
bool assFriCon()
{
  bool result = associateDevice(
      TxByteArrFriCon, sizeof(TxByteArrFriCon),
      def_Network_id,
      "con_id",
      ASS_CON_TOPIC,
      "homeassistant/switch/frisquet/assconnect/state");

  if (result)
  {
    DBG_PRINTLN("Association du Frisquet Connect réussie !");
    assConFrisquet = "OFF";
  }
  else
  {
    DBG_PRINTLN("Echec de l'association du Frisquet Connect !");
  }
  return result;
}
//****************************************************************************
void initOTA()
{
  ArduinoOTA.setHostname("ESP32Frisquet");
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
    DBG_PRINTLN("Start updating " + type); })
      .onEnd([]()
             { DBG_PRINTLN(F("\nEnd")); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DBG_PRINTLN(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) DBG_PRINTLN(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) DBG_PRINTLN(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) DBG_PRINTLN(F("Receive Failed"));
    else if (error == OTA_END_ERROR) DBG_PRINTLN(F("End Failed")); });
  ArduinoOTA.begin();
}
//****************************************************************************
void setFlag(void)
{
  // we got a packet, set the flag
  receivedFlag = true;
}
//****************************************************************************
void setup()
{
  Serial.begin(115200);
  DBG_PRINTLN(F("Booting"));
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("ESP32Frisquet");
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    DBG_PRINTLN(F("Connection Failed! Rebooting..."));
    delay(5000);
    ESP.restart();
  }

  initNvs(); // écrit dans la nvs les bytes nécéssaires

  // Initialize OLED display
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->init();
  // Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->drawXbm(0, 0, 128, 64, myLogo);
  Heltec.display->display();

  initOTA();
  DBG_PRINTLN(F("Ready"));
  DBG_PRINT(F("IP address: "));
  DBG_PRINTLN(WiFi.localIP());
  // Initialisation de la connexion MQTT
  client.setServer(mqttServer, mqttPort);
  client.setBufferSize(2048);
  connectToMqtt();
  connectToTopic();
  client.setCallback(callback);

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);
  // start listening for Radio packets
  int state = radio.beginFSK();
  state = radio.setFrequency(868.96);
  state = radio.setBitRate(25.0);
  state = radio.setFrequencyDeviation(50.0);
  state = radio.setRxBandwidth(250.0);
  state = radio.setPreambleLength(4);
  state = radio.setSyncWord(custom_network_id, sizeof(custom_network_id));
  DBG_PRINT(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE)
  {
    DBG_PRINTLN(F("success!"));
  }
  else
  {
    DBG_PRINT(F("failed, code "));
    DBG_PRINTLN(state);
  }

  preferences.end(); // Fermez la mémoire NVS ici
}
//****************************************************************************
void adaptMod(uint8_t modeValue,uint8_t zone)
{
  const char *topic;
  if (zone == 0x09)
  {
    topic = "homeassistant/select/frisquet/mode2/state"; // Topic MQTT pour le mode
  }
  else 
  {
    topic = "homeassistant/select/frisquet/mode/state"; // Topic MQTT pour le mode
  }
  const char *mode;
  // Traduire la valeur en un mode texte
  switch (modeValue)
  {
  case 0x05:
    mode = "Auto";
    break;
  case 0x06:
    mode = "Confort";
    break;
  case 0x07:
    mode = "Réduit";
    break;
  case 0x08:
    mode = "Hors gel";
    break;
  default:
    mode = "Inconnu"; // Valeur par défaut si non reconnue
    break;
  }

  // Publier le mode sur le topic MQTT
  if (client.publish(topic, mode))
  {
    DBG_PRINTLN("Mode mis a jour sur MQTT");
  }
  else
  {
    Serial.printf("Erreur lors de la publication du mode !");
    DBG_PRINTLN("Erreur lors de la publication du mode !");
  }
}
//****************************************************************************
void handleRadioPacket(byte *byteArr, int len)
{
  Serial.printf("RECEIVED [%2d] : ", len);
  char message[255];
  message[0] = '\0';
  if (custom_friCon_id != 0x00)
  {
    if (len == 63)
    {
      if (byteArr[0] == 0x7e && byteArr[1] == 0x80 && byteArr[3] == msg79e0 && byteArr[4] == 0x81 && byteArr[5] == 0x03)
      {
        if (sensorecs == true)
        {
          // Extract bytes 8 and 9 ECS
          int decimalValue1 = byteArr[7] << 8 | byteArr[8];
          float ecsValue = decimalValue1 / 10.0;
          char tempECS[10];
          snprintf(tempECS, sizeof(tempECS), "%.2f", ecsValue);
          publishMessage("homeassistant/sensor/frisquet/tempECS/state", tempECS);
        }
        // Extract bytes 10 and 11 CDC
        int decimalValue2 = byteArr[9] << 8 | byteArr[10];
        float cdcValue = decimalValue2 / 10.0;
        char tempCDC[10];
        snprintf(tempCDC, sizeof(tempCDC), "%.2f", cdcValue);
        publishMessage("homeassistant/sensor/frisquet/tempCDC/state", tempCDC);
        // Extract bytes 12 and 13 Départ
        int decimalValue3 = byteArr[11] << 8 | byteArr[12];
        float departValue = decimalValue3 / 10.0;
        char tempDEP[10];
        snprintf(tempDEP, sizeof(tempDEP), "%.2f", departValue);
        publishMessage("homeassistant/sensor/frisquet/tempDepart/state", tempDEP);
        // Extract bytes 44 and 45 temp ambiante
        int decimalValueTemp = byteArr[43] << 8 | byteArr[44];
        float temperatureValue = decimalValueTemp / 10.0;
        char temperaturePayload[10];
        snprintf(temperaturePayload, sizeof(temperaturePayload), "%.2f", temperatureValue);
        publishMessage(TEMP_AMBIANTE1_TOPIC, temperaturePayload);
        // Extract bytes 56 and 57 temp consigne
        int decimalValueCons = byteArr[55] << 8 | byteArr[56];
        float temperatureconsValue = decimalValueCons / 10.0;
        char tempconsignePayload[10];
        snprintf(tempconsignePayload, sizeof(tempconsignePayload), "%.2f", temperatureconsValue);
        publishMessage(TEMP_CONSIGNE1_TOPIC, tempconsignePayload);
      }
      else if (byteArr[0] == 0x7e && byteArr[1] == 0x80 && byteArr[3] == msg7a18 && byteArr[4] == 0x81 && byteArr[5] == 0x03)
      {
        // Extract bytes 28 and 29 conso gaz de la veille
        int decimalValue1 = byteArr[27] << 8 | byteArr[28];
        // float gazValue = decimalValue1;
        char consoGazCh[10];
        snprintf(consoGazCh, sizeof(consoGazCh), "%d", decimalValue1);
        publishMessage("homeassistant/sensor/frisquet/consogaz-ch/state", consoGazCh);
        if (sensorecs == true)
        {
          // Extract bytes 26 and 27 conso gaz de la veille
          int decimalValue2 = byteArr[25] << 8 | byteArr[26];
          // float gazValue = decimalValue1;
          char consoGazEcs[10];
          snprintf(consoGazEcs, sizeof(consoGazEcs), "%d", decimalValue2);
          publishMessage("homeassistant/sensor/frisquet/consogaz-ecs/state", consoGazEcs);
        }
      }
      else if (byteArr[0] == 0x7e && byteArr[1] == 0x80 && byteArr[4] == 0x08 && byteArr[5] == 0x17)
      {
        TxByteArrConRep[2] = custom_friCon_id;
        TxByteArrConRep[3] = byteArr[3];
        TxByteArrConRep[4] = byteArr[4];
        memcpy(&TxByteArrConRep[7], &byteArr[15], 41); // Copie 41 octets depuis byteArr[15] dans TxByteArrConRep[7]
        memcpy(&TxByteArrConMod[15], &byteArr[15], 48); // Copie 48 octets depuis byteArr[15] dans TxByteArrConMod[7]

        // Envoi de la confirmation de reception
        int State = radio.transmit(TxByteArrConRep, sizeof(TxByteArrConRep));
        if (State == RADIOLIB_ERR_NONE)
        {
          //  Appeler adaptMod avec la valeur extraite de byteArr[10]
          uint8_t modeValue = TxByteArrConRep[10];
          uint8_t zone = TxByteArrConRep[4];
          adaptMod(modeValue,zone);
        }
        else
        {
          DBG_PRINTLN("Erreur lors de la transmission !");
        }
      }
      else if (byteArr[0] == 0x7e && byteArr[1] == 0x80 && byteArr[4] == 0x09 && byteArr[5] == 0x17)
      {
        TxByteArrConRep[2] = custom_friCon_id;
        TxByteArrConRep[3] = byteArr[3];
        TxByteArrConRep[4] = byteArr[4];
        memcpy(&TxByteArrConRep[7], &byteArr[15], 41); // Copie 41 octets depuis byteArr[15] dans TxByteArrConRep[7]
        // Envoi de la chaine d'association
        int State = radio.transmit(TxByteArrConRep, sizeof(TxByteArrConRep));
        if (State == RADIOLIB_ERR_NONE)
        {
          //  Appeler adaptMod avec la valeur extraite de byteArr[10]
          uint8_t mode2Value = TxByteArrConRep[10];
          uint8_t zone = TxByteArrConRep[4];
          adaptMod(mode2Value,zone);
        }
        else
        {
          DBG_PRINTLN("Erreur lors de la transmission !");
        }
      }
    
    }

    else if (len == 55 && byteArr[0] == 0x7e && byteArr[1] == 0x80 && byteArr[4] == 0x88 && byteArr[5] == 0x17)
    {
      uint8_t modeValue = byteArr[10];
      uint8_t zone = byteArr[4];
      adaptMod(modeValue,zone);
      waitingForResponse = false;
    }
  }
  else
  {
    if (len == 23)
    { // Check if the length is 23 bytes


      bool sat1=byteArr[1] == 0x08;
        
      // Extract bytes 16 and 17
      int decimalValueTemp = byteArr[15] << 8 | byteArr[16];
      float temperatureValue = decimalValueTemp / 10.0;
      // Extract bytes 18 and 19
      int decimalValueCons = byteArr[17] << 8 | byteArr[18];
      float temperatureconsValue = decimalValueCons / 10.0;
      // Publish temperature to the "frisquet_temperature" MQTT topic
      char temperaturePayload[10];
      snprintf(temperaturePayload, sizeof(temperaturePayload), "%.2f", temperatureValue);
      publishMessage(sat1 ? TEMP_AMBIANTE1_TOPIC : TEMP_AMBIANTE2_TOPIC, temperaturePayload);
      // Publish temperature to the "tempconsigne" MQTT topic
      char tempconsignePayload[10];
      snprintf(tempconsignePayload, sizeof(tempconsignePayload), "%.2f", temperatureconsValue);
      publishMessage(sat1 ? TEMP_CONSIGNE1_TOPIC : TEMP_CONSIGNE2_TOPIC, tempconsignePayload);
    }
  }
  int pos = 0;

  // Convertir chaque octet en deux caractères hexadécimaux
  for (int i = 0; i < len; i++)
  {
    uint8_t b = byteArr[i];
    // Premier nibble (4 bits)
    message[pos++] = hexDigits[b >> 4];
    // Deuxième nibble
    message[pos++] = hexDigits[b & 0x0F];
    // Ajouter un espace séparateur
    message[pos++] = ' ';
    // Vérifier qu'on ne dépasse pas la taille du buffer
    if (pos >= (int)(sizeof(message) - 2))
      break;
  }

  // Si on a ajouté un espace en trop à la fin, on l'enlève
  if (pos > 0 && message[pos - 1] == ' ')
  {
    pos--;
  }

  // Terminer la chaîne
  message[pos] = '\0';
  if (!client.publish("homeassistant/sensor/frisquet/payload/state", message))
  {
    DBG_PRINTLN(F("Failed to publish Payload to MQTT"));
  }
  DBG_PRINTLN(F(""));
}
//****************************************************************************
void loop()
{
  // check if the flag is set
  if (receivedFlag)
  {
    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    byte byteArr[RADIOLIB_SX126X_MAX_PACKET_LENGTH];
    int state = radio.readData(byteArr, 0);
    if (state == RADIOLIB_ERR_NONE)
    {
      // packet was successfully received
      DBG_PRINTLN(F("[SX1262] Received packet!"));

      // print data of the packet
      DBG_PRINT(F("[SX1262] Data:\t\t"));
      int len = radio.getPacketLength();
      handleRadioPacket(byteArr, len);

      // print RSSI (Received Signal Strength Indicator)
      DBG_PRINT(F("[SX1262] RSSI:\t\t"));
      DBG_PRINT(radio.getRSSI());
      DBG_PRINTLN(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      DBG_PRINT(F("[SX1262] SNR:\t\t"));
      DBG_PRINT(radio.getSNR());
      DBG_PRINTLN(F(" dB"));

      // print frequency error
      DBG_PRINT(F("[SX1262] Frequency error:\t"));
      DBG_PRINT(radio.getFrequencyError());
      DBG_PRINTLN(F(" Hz"));
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
      // packet was received, but is malformed
      DBG_PRINTLN(F("CRC error!"));
    }
    else
    {
      // some other error occurred
      DBG_PRINT(F("failed, code "));
      DBG_PRINTLN(state);
    }
  }

  if (eraseNvsFrisquet == "ON")
  {
    eraseNvs();
    initNvs();
  }
  else if (assSonFrisquet == "ON")
  {
    assExtSon();
  }
  else if (assConFrisquet == "ON")
  {
    assFriCon();
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
        DBG_PRINTLN(F("Id sonde externe non connue"));
      }
      // Mettre à jour le temps de la dernière transmission
      lastTxExtSonTime = currentTime;
    }
    // Vérifier si c'est le moment de démarrer l'envoi des 4 trames
    if (currentTime - lastConMsgTime >= conMsgInterval)
    {
      // Vérifier si custom_friCon_id n'est pas égal à 0 byte
      if (memcmp(custom_network_id, "\xFF\xFF\xFF\xFF", 4) != 0 && custom_friCon_id != 0x00)
      {
        // On remet à zéro l'index et on indique qu'il faut envoyer 4 trames
        conMsgIndex = 0;
        conMsgToSendCount = 4;
        //lastConMsgTime = currentTime; // on remet le timer à zéro
        sequenceMsg = !sequenceMsg;
      }
      else
      {
        DBG_PRINTLN(F("Id frisquet connect non connue"));
      }
      lastConMsgTime = currentTime;
    }

    // Si on a des trames à envoyer depuis connect
    if (conMsgToSendCount > 0)
    {
      txfriConMsg(); // Appeler la fonction pour transmettre les données
    }
    if (!client.connected())
    {
      connectToMqtt();
    }
    ArduinoOTA.handle();

    // Compteur pour limiter la déclaration des topic
    if (counter >= 100000)
    {
      connectToTopic();
      counter = 0;
    }
    counter++;
    // Changement de mode depuis HA
    if (waitingForResponse)
    {
      unsigned long currentTime = millis();

      // Vérification du timeout des 4 minutes
      if (currentTime - startWaitTime >= maxWaitTime)
      {
        waitingForResponse = false;
        DBG_PRINTLN("Timeout mode");
      }
      else
      {
        // Réenvoi toutes les 2 secondes
        if (currentTime - lastTxModeTime >= retryInterval)
        {
          handleModeChange(modeFrisquet.c_str(),0x08);
          // int state = radio.transmit(TxByteArrConMod, sizeof(TxByteArrConMod));
          lastTxModeTime = currentTime;
        }
      }
    }
    if (waitingForResponse2)
    {
      unsigned long currentTime = millis();

      // Vérification du timeout des 4 minutes
      if (currentTime - startWaitTime >= maxWaitTime)
      {
        waitingForResponse2 = false;
        DBG_PRINTLN("Timeout mode");
      }
      else
      {
        // Réenvoi toutes les 2 secondes
        if (currentTime - lastTxModeTime >= retryInterval)
        {
          handleModeChange(modeFrisquet2.c_str(),0x09);
          // int state = radio.transmit(TxByteArrConMod, sizeof(TxByteArrConMod));
          lastTxModeTime = currentTime;
        }
      }
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

