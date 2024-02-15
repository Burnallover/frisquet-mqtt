This Arduino code is designed for a Heltec WiFi LoRa 32 V3. It will automatically create sensors, buttons and an input select in Home Assistant using MQTT discovery. The sensors are as follows:

- Actual temperature
- Setpoint temperature
- Exterior temperature
- payload received
  
Buttons are :

- switch to initiate the association of an emulated external temperature sensor
- switch to initiate the association of an emulated Frisquet connect box (in the future)

Input select :

- prefilled mode to manage heating mode of the boiler  

Some of this code was found on https://forum.hacf.fr/t/pilotage-chaudiere-frisquet-eco-radio-system-visio/19814/90
 
# Requirement

1. Mosquitto broker installed and linked with Home assistant
2. user and password for mqtt 
3. IP of Mosquitto broker
4. SSID and password of the wifi

# Configuration

Add your Wifi and Mqtt information in the file named 'config.h'
```bash
 // Configuration Wifi
 const char* ssid = "ssid wifi";  // Mettre votre SSID Wifi
 const char* password = "wifi password";  // Mettre votre mot de passe Wifi

 // Définition de l'adresse du broket MQTT
 const char* mqttServer = "192.168.XXX.XXX"; // Mettre l'ip du serveur mqtt
 const int mqttPort = 1883;
 const char* mqttUsername = "mqttUsername"; // Mettre le user mqtt
 const char* mqttPassword = "mqttPassword"; // Mettre votre mot de passe mqtt
```
You can now, flash your Heltec device.

# Bind your external temp sensor on mqtt

After the program has created all sensors, and before launching the association of an emulated external sensor with the boiler, you must bind an external temperature from Home Assistant to the new MQTT topic created by this program.

1. On HA, create an automation with the UI and switch to YAML configuration.
2. Add this YAML configuration to your automation:
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
If you don't have any sensor for external temperature, you can bind the temperature of the integrated weather forecast module of HA with:
```bash
payload_template: "{{ state_attr('weather.XXXXXX', 'temperature') }}"
```
Though not very accurate, it does the job.

3. Verify if the temperature is correctly sent to the ESP by looking at the screen or directly in the device on HA.

# Exterior temperature sensor Association

If the exterior temperature sensor is correctly bound, you can begin the association of the exterior temperature sensor.

1. On the boiler, go to the configuration menu, modify the actual regulation mode, and select the line "temperature ambiante + exterieur."
2. I advise calculating your actual "pente" based on your region and altitude and inputting your calculated pente when asked.
3. Press OK until the screen asks to associate the exterior sensor.
4. On HA, go to the device and activate the switch that mentions "ass. sonde."
5. The boiler should indicate that the exterior sensor is associated, and the "ass. sonde" button should return to off.

That's all; after 10 minutes, Heltec scren should update, you should have the exterior temperature displayed on the boiler screen and on the interior satellite screen.

# Tweak
After the initial association, the network ID and the exterior sensor ID are written on the first line of the Heltec's screen as well as in the console. Even though this data is normally stored in the ESP's NVS memory, I advise you to save them to avoid starting over in case of a major update that would overwrite this memory.

If you already have your network ID, you can put it in the 'config.h' file before flashing your Heltec. It will not change with the exterior sensor association.

If you already have an exterior sensor ID from an older association, you can also put it in the 'config.h' file, but make sure that the exterior temperature is correctly bound to the correct MQTT topic.

If your boiler does not display the 'OK' association message, it's likely that your ESP is too far from your boiler. The Heltec is capable of receiving frames over very long distances, but its frame transmission is not very powerful.
