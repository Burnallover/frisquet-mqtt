// conf.h
#ifndef CONF_H
#define CONF_H

// Configuration Wifi
const char* ssid = "ssid wifi";  // Mettre votre SSID Wifi
const char* password = "wifi password";  // Mettre votre mot de passe Wifi
// Configuration MQTT
const char* mqttServer = "192.168.XXX.XXX"; // Mettre l'IP du serveur MQTT
const int mqttPort = 1883;
const char* mqttUsername = "mqttUsername"; // Mettre le user MQTT
const char* mqttPassword = "mqttPassword"; // Mettre votre mot de passe MQTT
//activation sensor Zone 2
const bool sensorZ2 = false;
//activation eau chaude saniataire
const bool sensorecs = true;

// network id & ext.sonde id if they are known
uint8_t network_id[] = {0xFF, 0xFF, 0xFF, 0xFF};
uint8_t def_Network_id[] = {0xFF, 0xFF, 0xFF, 0xFF};
uint8_t extSon_id = 0xFF;
uint8_t friCon_id = 0xFF;

#endif // CONFIG_H
