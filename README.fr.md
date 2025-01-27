[English](README.md) | [Français](README.fr.md)

Ce code Arduino est conçu pour un Heltec WiFi LoRa 32 V3. Il créera automatiquement des capteurs, des boutons et une sélection d’entrée dans Home Assistant via la découverte MQTT. Voici les éléments pris en charge :

## Capteurs

- Température actuelle
- Température de consigne
- Température extérieure
- Payload reçu
- Température ECS
- Température de démarrage
- Température CDC
- Consommation de gaz pour le chauffage
- Consommation de gaz pour l’eau chaude sanitaire

## Boutons

- Bouton pour initier l’association d’un capteur de température extérieure émulé
- Bouton pour initier l’association d’une box Frisquet Connect émulée
- Bouton pour effacer la mémoire NVS de l’ESP32

## Sélection d’entrée

- Mode prédéfini pour gérer le mode de chauffage de la zone par défaut de la chaudière
- Mode prédéfini pour gérer le mode de chauffage de la deuxième zone (si activée)

Certains morceaux de ce code proviennent de [ce sujet sur le forum HACF](https://forum.hacf.fr/t/pilotage-chaudiere-frisquet-eco-radio-system-visio/19814/90).

---

# Prérequis

1. Broker Mosquitto installé et lié à Home Assistant
2. Identifiant et mot de passe pour MQTT
3. Adresse IP du broker Mosquitto
4. SSID et mot de passe du réseau Wi-Fi

---

# Configuration

Ajoutez vos informations Wi-Fi et MQTT dans le fichier nommé `conf.example.h`, puis renommez-le en `conf.h`.

Si vous avez une deuxième zone et/ou l’eau chaude sanitaire sur votre modèle de chaudière, vous pouvez modifier les variables `sensorZ2` et `sensorecs` à `true`.

```cpp
// Configuration Wi-Fi
const char* ssid = "votre_ssid_wifi";  // Mettre votre SSID Wi-Fi
const char* password = "votre_mot_de_passe_wifi";  // Mettre votre mot de passe Wi-Fi

// Définition de l’adresse du broker MQTT
const char* mqttServer = "192.168.XXX.XXX";  // Mettre l’IP du serveur MQTT
const int mqttPort = 1883;
const char* mqttUsername = "votre_utilisateur_mqtt";  // Mettre l’utilisateur MQTT
const char* mqttPassword = "votre_mot_de_passe_mqtt";  // Mettre votre mot de passe MQTT

// Activation capteur Zone 2
const bool sensorZ2 = false;
// Activation eau chaude sanitaire
const bool sensorecs = false;
```

Vous pouvez maintenant flasher votre appareil Heltec.

---

# Liaison du capteur de température extérieure via MQTT

Une fois que le programme a créé tous les capteurs, et avant de lancer l'association d'un capteur extérieur émulé avec la chaudière, vous devez lier une température extérieure depuis Home Assistant au nouveau topic MQTT créé par ce programme.

1. Dans Home Assistant, créez une automatisation via l’interface utilisateur et basculez vers la configuration YAML.
2. Ajoutez cette configuration YAML :

```yaml
alias: Température extérieure MQTT
trigger:
  - platform: time_pattern
    minutes: "/5"  # Fréquence d'exécution
condition: []
action:
  - service: mqtt.publish
    data:
      qos: "1"
      retain: true
      topic: homeassistant/sensor/frisquet/tempExterieure/state
      payload_template: "{{ states('sensor.nom_de_votre_capteur') }}"  # Remplacez par le nom de votre capteur
mode: single
```

Si vous n’avez pas de capteur pour la température extérieure, vous pouvez utiliser la température du module météo intégré de Home Assistant :

```yaml
payload_template: "{{ state_attr('weather.XXXXXX', 'temperature') }}"
```

3. Vérifiez si la température est correctement envoyée à l’ESP en regardant l’écran ou directement dans l’appareil dans Home Assistant.

---

# Association du capteur de température extérieure

1. Sur la chaudière, accédez au menu de configuration, modifiez le mode de régulation actuel et sélectionnez la ligne « température ambiante + extérieure ».
2. Si possible, Calculez la « pente » en fonction de votre région et altitude et entrez cette valeur lorsque demandé.
3. Appuyez sur OK jusqu’à ce que l’écran demande d’associer le capteur extérieur.
4. Dans Home Assistant, activez l’interrupteur mentionnant « ass. sonde ».
5. La chaudière devrait indiquer que le capteur extérieur est associé, et le bouton « ass. sonde » devrait se désactiver automatiquement.

---

# Association du Frisquet Connect émulée

1. Sur la chaudière, accédez au menu de configuration et lancez l'association Frisquet Connect.
2. Appuyez sur OK jusqu’à ce que l’écran demande d’associer la Frisquet Connect.
3. Dans Home Assistant, activez l’interrupteur mentionnant « ass. connect ».
4. La chaudière devrait indiquer que la Frisquet Connect est associée, et le bouton « ass. connect » devrait se désactiver automatiquement.

**Note :** Avant de pouvoir changer le mode de la chaudière, vous devez d'abord changer le mode avec le satellite après chaque redémarrage de l'ESP ou remplacer directement le tableau de bytes `TxByteArrConMod` par votre configuration actuelle.

---

# Liaison de la consommation de gaz au tableau de bord énergétique de Home Assistant

La chaudière n’envoie que les données de consommation de gaz de la veille, et cette valeur n’est pas cumulative. Vous devez donc ajouter une automatisation dans Home Assistant pour réinitialiser quotidiennement cette valeur et fournir un capteur précis.

Exemple YAML pour l’automatisation :

```yaml
alias: Mise à jour consommation gaz
trigger:
  - platform: time_pattern
    hours: "00"
    minutes: "00"
    seconds: "00"
condition: []
action:
  - service: mqtt.publish
    data:
      qos: "1"
      retain: false
      topic: homeassistant/sensor/frisquet/consogaz-ch/state
      payload: "0"
mode: single
```
Si votre chaudière gère également le chauffage de l’eau sanitaire, ajoutez cette partie :
```yaml
  - service: mqtt.publish
    data:
      qos: "1"
      retain: false
      topic: homeassistant/sensor/frisquet/consogaz-ecs/state
      payload: "0"
mode: single
```
# Ajustements

Après l'association initiale, l’ID réseau et l’ID du capteur extérieur sont affichés sur la première ligne de l’écran de l’Heltec ainsi que dans la console. Bien que ces données soient normalement stockées dans la mémoire NVS de l’ESP, il est conseillé de les sauvegarder pour éviter de recommencer en cas de mise à jour majeure écrasant cette mémoire.

    Si vous connaissez déjà votre ID réseau, vous pouvez le définir dans le fichier config.h avant de flasher votre Heltec. Cela ne changera pas lors de l'association avec le capteur extérieur.
    Si vous avez un ID de capteur extérieur provenant d’une association précédente, vous pouvez également l’ajouter dans le fichier config.h. Assurez-vous que la température extérieure est correctement liée au topic MQTT approprié.

Si la chaudière n’affiche pas le message « OK » lors de l’association, il est probable que l’ESP soit trop éloigné de la chaudière. L’Heltec peut recevoir des trames sur de très longues distances, mais son émission de trames est moins puissante.

Example de flux nodered pour changer de mode en fonction de la presence de mobile
```yaml
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

