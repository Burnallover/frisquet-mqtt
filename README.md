# Not up to date !!!!!! Coming soon

This arduino code is made for a heltec_wifi_lora_32_V3. 
It will automatically create sensors buttons, and an input select in Home Assistant with Mqtt discovery. 
Sensors are :
- Actual temperature
- Setpoint temperature
- Outdoor temperature
- payload received
  
Buttons are :

- switch to launch the association of an emulated external temperature sensor
- switch to launch the association of an emulated Frisquet connect box (in the future)

Input select :

- prefilled mode to manage heating mode of the boiler  

Some of this code has been found on https://forum.hacf.fr/t/pilotage-chaudiere-frisquet-eco-radio-system-visio/19814/90
 
# Requirement

1. Mosquitto broker installed and linked with Home assistant
2. user and password for mqtt 
3. IP of Mosquitto broker
4. SSID and password of the wifi

# Bind your external temp sensor on mqtt

Once the program have created all sensors, and before launch the association of an emulated external sensor to the boiler, you must bind an external temperature from Home assistant to the new Mqtt topic created by this program.

1. Create an automation with UI, and move to Yaml configuration
2. Put this yaml conf to your automation
```bash
alias: Mqtt temperature exterieure
description: ""
trigger:
  - platform: time_pattern
    minutes: /5 # execution time
condition: []
action:
  - service: mqtt.publish
    data:
      qos: "1"
      retain: true
      topic: homeassistant/sensor/frisquet/tempExterieure/state
      payload_template: "{{ states('sensor.your_sensor_temperature') }}" # add the sensor name
mode: single
```
If you don't have any sensor for external temp, you can bind the temperature of the integrated weather forecast module of HA with :
```bash
payload_template: "{{ state_attr('weather.XXXXXX', 'temperature') }}"
```
Not really accurate, but do the job.

3. verify if the temperature is correctly sent to the ESP by looking at the screen, or directly in the device on HA.

# Configuration

When you know your boiler's network ID, you can use the code in this repository's main.cpp and config.h file, modify the following lines on config.h :
```bash
 // Configuration Wifi
 const char* ssid = "ssid wifi";  // Mettre votre SSID Wifi
 const char* password = "wifi password";  // Mettre votre mot de passe Wifi

 // Définition de l'adresse du broket MQTT
 const char* mqttServer = "192.168.XXX.XXX"; // Mettre l'ip du serveur mqtt
 const int mqttPort = 1883;
 const char* mqttUsername = "mqttUsername"; // Mettre le user mqtt
 const char* mqttPassword = "mqttPassword"; // Mettre votre mot de passe mqtt


 uint8_t network_id[] = {0xNN, 0xNN, 0xNN, 0xNN}; // remplacer NN par le network id de la chaudière
```
