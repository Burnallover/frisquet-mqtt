[English](README.md) | [Français](README.fr.md)

This Arduino code is designed for a Heltec WiFi LoRa 32 V3. It will automatically create sensors, buttons and an input select in Home Assistant using MQTT discovery. The sensors are as follows:

## Sensors :

- Actual temperature
- Setpoint temperature
- Exterior temperature
- payload received
- Ecs temperature
- start temperature
- Cdc temperature
- Gas heating consumption
- Gas water heating consumption
  
## Buttons :

- switch to initiate the association of an emulated external temperature sensor
- switch to initiate the association of an emulated Frisquet connect box
- switch to erase the NVS memory of the ESP32

## Input select :

- prefilled mode to manage heating mode of the boiler for default zone
- prefilled mode to manage heating mode of the boiler for second zone if activated

Some of this code was found on https://forum.hacf.fr/t/pilotage-chaudiere-frisquet-eco-radio-system-visio/19814/90

---

# Requirement

1. Mosquitto broker installed and linked with Home assistant
2. user and password for mqtt 
3. IP of Mosquitto broker
4. SSID and password of the wifi

---

# Configuration

Add your Wifi and Mqtt information in the file named 'conf.example.h' and rename it conf.h
If you have a second Zone, and/or water heating on your boiler model, you can modify sensorZ2 and sensorecs to 'true'
```bash
 // Configuration Wifi
 const char* ssid = "ssid wifi";  // Mettre votre SSID Wifi
 const char* password = "wifi password";  // Mettre votre mot de passe Wifi

 // Définition de l'adresse du broket MQTT
 const char* mqttServer = "192.168.XXX.XXX"; // Mettre l'ip du serveur mqtt
 const int mqttPort = 1883;
 const char* mqttUsername = "mqttUsername"; // Mettre le user mqtt
 const char* mqttPassword = "mqttPassword"; // Mettre votre mot de passe mqtt
 ....
 //activation sensor Zone 2
 const bool sensorZ2 = false;
 //activation eau chaude saniataire
 const bool sensorecs = false;
```
You can now, flash your Heltec device.

---

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

---

# Exterior temperature sensor Association

If the exterior temperature sensor is correctly bound, you can begin the association of the exterior temperature sensor.

1. On the boiler, go to the configuration menu, modify the actual regulation mode, and select the line "temperature ambiante + exterieur."
2. I advise calculating your actual "pente" based on your region and altitude and inputting your calculated pente when asked.
3. Press OK until the screen asks to associate the exterior sensor.
4. On HA, go to the device and activate the switch that mentions "ass. sonde."
5. The boiler should indicate that the exterior sensor is associated, and the "ass. sonde" button should return to off.

That's all; after 10 minutes, Heltec screen should update, you should have the exterior temperature displayed on the boiler screen and on the interior satellite screen.

---

# Emulated frisquet connect Association

1. On the boiler, go to the configuration menu, launch the frisquet connect association
2. Press OK until the screen asks to associate the frisquet connect.
3. On HA, go to the device and activate the switch that mentions "ass. connect."
4. The boiler should indicate that the Frisquet connect is associated, and the "ass. connect" button should return to off.

Note that before you can change the mode of the boiler, you should first change the mode with the satellite at each reboot of the ESP, or directly replace the correct byte array 'TxByteArrConMod' with your actual configuration, or you will have your configuration replaced by mine :D (this part will be improved in the future)

---

# Bind the gas consumption on the dashboard energy of HA

The boiler only sends the previous day's gas consumption data, and this value is not cumulative. Therefore, it is necessary to add an automation in Home Assistant to daily reset this value and provide an accurate sensor.
Here is the yaml code to create the automation :
```bash 
alias: maj consogaz
description: ""
triggers:
  - trigger: time_pattern
    hours: "00"
    minutes: "00"
    seconds: "00"
conditions: []
actions:
  - action: mqtt.publish
    data:
      evaluate_payload: false
      qos: "1"
      retain: false
      topic: homeassistant/sensor/frisquet/consogaz-ch/state
      payload: |-
        {{
        0
        }}
mode: single
```
If your boiler manage water heating, you can add this part as well
```bash
  - action: mqtt.publish
    metadata: {}
    data:
      evaluate_payload: false
      qos: "1"
      retain: false
      topic: homeassistant/sensor/frisquet/consogaz-ecs/state
      payload: |-
        {{
        0
        }}
mode: single
```

# Tweak
After the initial association, the network ID and the exterior sensor ID are written on the first line of the Heltec's screen as well as in the console. Even though this data is normally stored in the ESP's NVS memory, I advise you to save them to avoid starting over in case of a major update that would overwrite this memory.

If you already have your network ID, you can put it in the 'config.h' file before flashing your Heltec. It will not change with the exterior sensor association.

If you already have an exterior sensor ID from an older association, you can also put it in the 'config.h' file, but make sure that the exterior temperature is correctly bound to the correct MQTT topic.

If your boiler does not display the 'OK' association message, it's likely that your ESP is too far from your boiler. The Heltec is capable of receiving frames over very long distances, but its frame transmission is not very powerful.

Example of nodered flow to handlemode based on presence of phone
```
[{
    "id": "e34c43b8434975c0",
    "type": "group",
    "z": "2ac199aef7c7d38b",
    "name": "Gestion chaudière",
    "style": {
        "label": true
    },
    "nodes": ["a792a752b3f73808", "428cfef86014907f", "566eb2d9edc5b46b", "5d247b73a13d088d", "47089b9d4678c9ee", "d247d9e1a19bc004", "a0fc922d11df7489"],
    "x": 14,
    "y": 2059,
    "w": 1232,
    "h": 162
}, {
    "id": "a792a752b3f73808",
    "type": "server-state-changed",
    "z": "2ac199aef7c7d38b",
    "g": "e34c43b8434975c0",
    "name": "Phones States",
    "server": "3c658989.0dc5e6",
    "version": 6,
    "outputs": 1,
    "entities": {
        "entity": ["device_tracker.phone_1", "device_tracker.phone_2"]
    },
    "outputInitially": false,
    "stateType": "str",
    "outputProperties": [{
        "property": "payload",
        "propertyType": "msg",
        "value": "",
        "valueType": "entityState"
    }],
    "x": 120,
    "y": 2140,
    "wires": [["566eb2d9edc5b46b"]]
}, {
    "id": "428cfef86014907f",
    "type": "function",
    "z": "2ac199aef7c7d38b",
    "g": "e34c43b8434975c0",
    "name": "Presence Check",
    "func": "const phone1 = global.get('homeassistant.homeAssistant.states[\"device_tracker.phone_1\"].state');\nconst phone2 = global.get('homeassistant.homeAssistant.states[\"device_tracker.phone_2\"].state');\n\nif (phone1 === 'not_home' && phone2 === 'not_home') {\n    msg.payload = 'disable';\n} else if (phone1 === 'home' || phone2 === 'home') {\n    msg.payload = 'enable';\n} else {\n    return null;\n}\nreturn msg;",
    "outputs": 1,
    "x": 460,
    "y": 2140,
    "wires": [["5d247b73a13d088d"]]
}, {
    "id": "566eb2d9edc5b46b",
    "type": "ha-get-entities",
    "z": "2ac199aef7c7d38b",
    "g": "e34c43b8434975c0",
    "rules": [{
        "condition": "state_object",
        "property": "entity_id",
        "logic": "includes",
        "value": "device_tracker.phone_1, device_tracker.phone_2",
        "valueType": "str"
    }],
    "outputLocation": "payload",
    "x": 290,
    "y": 2140,
    "wires": [["428cfef86014907f"]]
}, {
    "id": "5d247b73a13d088d",
    "type": "switch",
    "z": "2ac199aef7c7d38b",
    "g": "e34c43b8434975c0",
    "property": "payload",
    "rules": [{
        "t": "eq",
        "v": "enable",
        "vt": "str"
    }, {
        "t": "eq",
        "v": "disable",
        "vt": "str"
    }],
    "x": 630,
    "y": 2140,
    "wires": [["47089b9d4678c9ee"], ["a0fc922d11df7489"]]
}, {
    "id": "47089b9d4678c9ee",
    "type": "api-call-service",
    "z": "2ac199aef7c7d38b",
    "g": "e34c43b8434975c0",
    "name": "Control Auto",
    "server": "3c658989.0dc5e6",
    "action": "select.select_option",
    "entityId": ["select.boiler_mode"],
    "data": "{ \"option\": \"Auto\" }",
    "x": 820,
    "y": 2100,
    "wires": [["d247d9e1a19bc004"]]
}, {
    "id": "d247d9e1a19bc004",
    "type": "debug",
    "z": "2ac199aef7c7d38b",
    "g": "e34c43b8434975c0",
    "name": "debug",
    "x": 1140,
    "y": 2140,
    "wires": []
}, {
    "id": "a0fc922d11df7489",
    "type": "api-call-service",
    "z": "2ac199aef7c7d38b",
    "g": "e34c43b8434975c0",
    "name": "Control Reduced",
    "server": "3c658989.0dc5e6",
    "action": "select.select_option",
    "entityId": ["select.boiler_mode"],
    "data": "{ \"option\": \"Reduced\" }",
    "x": 820,
    "y": 2180,
    "wires": [["d247d9e1a19bc004"]]
}]
```

