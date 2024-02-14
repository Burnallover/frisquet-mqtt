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

# Exterior temperature sensor Association

If the Exterior temperature sensor is correctly binded, you can begin the association of the Exterior temperature sensor

1. On the Boiler, go to the configuration menu, modify the actual regulation mode and select the line "temperature ambiante + exterieur"
2. I advice to calculate your actual "pente" based on your region and altitude and to put your calculate pente when it's asked
3. press ok until the screen ask to associate the exterior sensor
4. On HA, go to the device and activate the switch that mention "ass. sonde"
5. Boiler should indicate that the exterior sensor is associated, and ass. sonde button should return to off.
6. That's all, after 10 minutes, you should have exterior temp. on the boiler screen, and on the Interior satellite screen.

# Tweak

If you already have your network id, you can put it on the config.h file before flashing your Heltec. it will not change with the exterior sensor association
If you already have an exterior sensor id from an older association, you can put it aswell on the config.h file, but make sure that the exterior Temp. is correctly binded to the correct mqtt topic.  
