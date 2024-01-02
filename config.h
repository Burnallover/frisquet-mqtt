// config.h
#ifndef CONFIG_H
#define CONFIG_H

const char* ssid = "ssid wifi";  // Mettre votre SSID Wifi
const char* password = "wifi password";  // Mettre votre mot de passe Wifi

const char* mqttServer = "192.168.XXX.XXX"; // Mettre l'IP du serveur MQTT
const int mqttPort = 1883;
const char* mqttUsername = "mqttUsername"; // Mettre le user MQTT
const char* mqttPassword = "mqttPassword"; // Mettre votre mot de passe MQTT

uint8_t network_id[] = {0xFF, 0xFF, 0xFF, 0xFF}; // Remplacer NN par le network id de la chaudière

#endif // CONFIG_H
