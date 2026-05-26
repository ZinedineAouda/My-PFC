# Projet de Fin de Cycle (PFC) — Documentation Technique Complète
## Système d'Alerte Médicale pour Patients Hospitalisés (Hospital Patient Alarm System)

---

## 1. Introduction Générale

Le présent projet consiste en la conception et la réalisation d'un **système d'alerte médicale connecté (Hospital Patient Alarm System)**. Ce système permet aux patients hospitalisés d'émettre des alertes d'urgence depuis leur lit vers un tableau de bord surveillé par le personnel soignant (infirmiers/médecins).

### 1.1 Problématique
Dans les établissements hospitaliers, la réactivité lors d'un appel d'urgence est un facteur critique. Les systèmes traditionnels filaires sont coûteux à installer, difficiles à maintenir et manquent de flexibilité. Les systèmes purement basés sur le cloud peuvent faillir en cas de perte de connexion Internet.

### 1.2 Solution Proposée
Pour répondre à cette double problématique de fiabilité et de mobilité, nous proposons un système hybride à deux niveaux :
1. **Un réseau local robuste** basé sur le protocole **MQTT** où un contrôleur central **ESP32-S3** embarque son propre courtier (broker) MQTT et gère en local des terminaux d'appel d'urgence **ESP8266**. Ce réseau fonctionne de manière 100% autonome, même sans Internet.
2. **Une synchronisation Cloud temps réel** bidirectionnelle via le protocole **MQTT/MQTTS** (Railway) qui permet de centraliser les alertes sur une base de données PostgreSQL, de suivre l'état de l'infrastructure à distance, et de gérer les patients via une interface Web d'administration sécurisée ou une application mobile native (Capacitor).

---

## 2. Architecture Globale du Système

Le système est segmenté en trois grandes couches : les terminaux clients (Devices), le contrôleur de passerelle local (Controller), et la plateforme de surveillance Cloud (Server + Dashboards).

```
 ┌────────────────────────────────────────────────────────────┐
 │                     CLOUD PLATFORM                         │
 │   - Backend : Node.js (Express) sur Railway                │
 │   - Base de Données : PostgreSQL                           │
 │   - Broker MQTT Cloud : Embarqué Aedes (Port 1883/8883)    │
 │   - Frontend : Tableau de Bord React sur Vercel            │
 └───────────────────────▲────────────────────────────────────┘
                         │ MQTT / MQTTS (Sécurisé & Temps Réel)
 ┌───────────────────────┴────────────────────────────────────┐
 │                 ESP32-S3 CONTROLLER (Passerelle)           │
 │   - Broker MQTT local (PicoMQTT sur port 1883)             │
 │   - Serveur Web local (AsyncWebServer sur port 80)          │
 │   - Client MQTT Cloud (PicoMQTT::Client)                   │
 │   - Tableau de bord autonome (HTML/JS en PROGMEM)          │
 └──────▲─────────────▲─────────────▲─────────────────────────┘
         │ MQTT        │ MQTT        │ MQTT (Wi-Fi Local)
 ┌──────┴──────┐ ┌────┴──────┐ ┌───┴───────┐
 │   ESP8266   │ │   ESP8266   │ │   ESP8266   │
 │  Chambre 1  │ │  Chambre 2  │ │  Chambre N  │
 └─────────────┘ └───────────┘ └───────────┘
```

### 2.1 Les Terminaux Patients (ESP8266)
Chaque lit de patient est équipé d'un boîtier doté d'un bouton poussoir relié à un module Wi-Fi low-cost **ESP8266**. Ce module communique en Wi-Fi local avec le contrôleur central via le protocole MQTT (QoS 0 pour les battements de cœur, QoS 0/1 pour les alertes).

### 2.2 Le Contrôleur Local (ESP32-S3)
Le cœur logique du réseau local est un **ESP32-S3**. Il cumule plusieurs rôles :
* **Point d'Accès Wi-Fi (SoftAP)** ou client Wi-Fi connecté au réseau de l'hôpital.
* **Broker MQTT local** (via la bibliothèque PicoMQTT) pour orchestrer les terminaux.
* **Serveur Web asynchrone** hébergeant un tableau de bord local (embarqué dans la mémoire flash `PROGMEM`) accessible via IP locale (`192.168.4.1` ou DHCP).
* **Générateur d'alerte physique** via un avertisseur sonore (buzzer actif) connecté à une broche GPIO.
* **Passerelle IoT temps réel** (via `PicoMQTT::Client`) qui synchronise l'état local avec le serveur Cloud par MQTT/MQTTS (avec chiffrement optionnel TLS/MQTTS sur le port 8883).

### 2.3 Le Serveur Cloud & Base de Données (Railway)
Un serveur Node.js exécutant Express intègre un courtier MQTT **Aedes** complet. Le client MQTT interne du serveur écoute les topics publiés par le contrôleur local et met à jour l'état du système (liste des périphériques, alertes actives, logs) dans une base de données relationnelle **PostgreSQL** via l'ORM **Drizzle**.

### 2.4 Le Dashboard Web & Mobile (Vercel / Capacitor)
Développé en **React 18** avec **Tailwind CSS** et **Radix UI**, ce tableau de bord affiche en temps réel les appels de patients grâce à une connexion **WebSocket** permanente avec le serveur de l'API. L'application est également encapsulée pour Android via **Capacitor**, permettant l'envoi de notifications locales push et l'usage de retours haptiques (vibrations) sur les téléphones des infirmiers.

---

## 3. Spécifications du Matériel (Hardware)

### 3.1 Schéma des Connexions & Broches (Pin Mapping)

#### A. Contrôleur Central (ESP32-S3 DevKitC-1)
Le contrôleur n'a besoin que d'un buzzer pour l'alarme sonore physique locale et d'une LED de statut.

| Broche ESP32-S3 | Composant | Rôle | Description |
|:---:|:---:|:---:|---|
| **GPIO 4** | Buzzer Actif (+) | Sortie Numérique | Activé (HIGH) lors d'une alerte, désactivé (LOW) sinon. |
| **GPIO 2** | LED de Statut | Sortie Numérique | Témoin visuel de l'état du contrôleur. |
| **GND** | Buzzer Actif (-) | Masse | Référence 0V |
| **USB-C** | Port Série / Alim | Alimentation + Debug | 5V / Communication Série à 115200 Bauds |

```
              ┌──────────────────┐
              │     ESP32-S3     │
              │                  │
    GPIO4 ───►│ (+)  Buzzer  (-) │◄─── GND
              └──────────────────┘
```

#### B. Terminal Bouton Patient (ESP8266 NodeMCU)
Chaque boîtier patient possède un bouton-poussoir pour déclencher l'alerte et une LED d'état.

| Broche ESP8266 | Composant | Mode de Configuration | Description |
|:---:|:---:|:---:|---|
| **GPIO 0 (D3)** | Bouton Poussoir | `INPUT_PULLUP` | Connecté au GND lors de l'appui (Actif à l'état BAS). |
| **GPIO 2 (D4)** | LED intégrée | `OUTPUT` | Active à l'état BAS (Active LOW). Indique le statut de connexion et d'alerte. |
| **GND** | Bouton Poussoir | Masse | Connecté au pôle opposé du bouton. |

```
              ┌──────────────────┐
              │   ESP8266 Node   │
              │                  │
    GPIO0 ───►│ [ Bouton-Poussoir ] ─── GND
              │                  │
    GPIO2 ───►│ [ LED Intégrée  ] ─── VCC (Interne)
              └──────────────────┘
```

### 3.2 Comportement Lumineux de la LED du Terminal (ESP8266)
La LED du terminal donne un retour visuel instantané sur son état :
* **Clignotement rapide (200ms)** : En cours de connexion au réseau Wi-Fi ou au courtier MQTT.
* **Clignotement lent (500ms)** : Connecté au réseau, mais en attente d'approbation administrative sur le tableau de bord.
* **Allumée Fixe** : Alerte active (le patient a appuyé sur le bouton).
* **Éteinte** : Connecté, approuvé par l'administrateur, système au repos.

---

## 4. Modes de Fonctionnement (Operating Modes)

Le système propose 4 modes opératoires sélectionnables lors de la configuration initiale (Setup Wizard) :

| Mode | Nom | Description | Wi-Fi Requis | Liaison Cloud |
|:---:|---|---|:---:|:---:|
| **Mode 1** | **AP Only** (Point d'Accès local) | Le contrôleur crée son propre réseau Wi-Fi nommé `HospitalAlarm`. Tous les terminaux s'y connectent. Interface accessible sur `http://192.168.4.1`. Idéal pour une cellule isolée sans infrastructure réseau. | Non | Non |
| **Mode 2** | **STA Only** (Station locale) | Le contrôleur se connecte à un réseau Wi-Fi existant (ex: Wi-Fi de l'hôpital). Les terminaux s'y connectent aussi. L'interface locale est servie sur l'adresse IP fournie par le DHCP du réseau local. | Oui | Non |
| **Mode 3** | **AP + STA Hybrid** | Le contrôleur maintient son propre Point d'Accès (`HospitalAlarm`) tout en se connectant au réseau local de l'hôpital. Les terminaux peuvent se connecter indifféremment sur l'un des deux réseaux. | Oui | Non |
| **Mode 4** | **Online (Cloud Sync)** | Même fonctionnement que le Mode 3, mais le contrôleur se connecte en continu au Broker MQTT Cloud (Railway). Les alertes sont acquittables mondialement en temps réel via le Web. | Oui | **Oui (Temps Réel / MQTT)** |

---

## 5. Protocoles de Communication

Le projet exploite quatre protocoles distincts adaptés à chaque niveau de l'architecture :

```
[Terminal ESP8266] <======== MQTT Local ========> [ESP32 Controller] <======== MQTT/MQTTS Cloud ========> [Backend Railway] <======== WebSockets (WSS) ========> [Dashboard Client]
```

### 5.1 Protocole MQTT Local (Contrôleur ↔ Terminaux)
Le protocole MQTT (Message Queuing Telemetry Transport) est utilisé au niveau local sur le port TCP `1883`. C'est un protocole léger de type publication/abonnement (Pub/Sub).

#### Topics MQTT locaux implémentés :

1. **`device/{deviceId}/alert`** (Device → Controller)
   * **Déclencheur** : Appui sur le bouton d'alerte.
   * **Payload** :
     ```json
     {
       "status": "pressed",
       "deviceId": "device-4a2b9f",
       "timestamp": 1716768000
     }
     ```

2. **`device/{deviceId}/heartbeat`** (Device → Controller)
   * **Déclencheur** : Périodique (toutes les 10 secondes).
   * **Payload** :
     ```json
     {
       "deviceId": "device-4a2b9f",
       "uptime": 1284,
       "rssi": -65
     }
     ```

3. **`device/{deviceId}/status`** (Controller → Device)
   * **Option** : Retained (le terminal reçoit son statut immédiatement lors d'une reconnexion).
   * **Payload** :
     ```json
     {
       "approved": true,
       "patientName": "Jean Dupont",
       "bed": "Chambre 4 - Lit A",
       "room": "Cardiologie"
     }
     ```

4. **`device/{deviceId}/command`** (Controller → Device)
   * **Payload** (ex: acquittement de l'alerte) :
     ```json
     {
       "action": "clear_alert"
     }
     ```

---

### 5.2 Protocole MQTT / MQTTS Cloud (Controller ↔ Railway)
Lorsque le système tourne en **Mode 4 (Online)**, le contrôleur utilise `PicoMQTT::Client` pour maintenir une connexion active avec le broker Aedes exécuté par le serveur cloud. Pour des raisons d'optimisation mémoire (Heap), les chaînes de topics sont pré-calculées une seule fois lors de la connexion initiale.

#### Topics MQTT Cloud implémentés :

1. **`controller/{deviceKey}/ping`** (Controller → Cloud Broker)
   * **QoS** : 0 (Acceptation de perte ponctuelle, intervalle de 60 secondes).
   * **Payload** :
     ```json
     {
       "mode": 4,
       "uptime": 3600,
       "rssi": -55,
       "wifiError": "OK"
     }
     ```

2. **`controller/{deviceKey}/sync`** (Controller → Cloud Broker)
   * **QoS** : 1 (Livraison garantie, intervalle de 30 secondes ou sur événement).
   * **Payload** :
     ```json
     {
       "mode": 4,
       "uptime": 3600,
       "rssi": -55,
       "wifiError": "OK",
       "devices": [
         {
           "deviceId": "device-4a2b9f",
           "patientName": "Jean Dupont",
           "bed": "Lit A",
           "room": "Chambre 4",
           "alertActive": true,
           "approved": true,
           "online": true,
           "lastUpdatedAt": 1716768100
         }
       ]
     }
     ```

3. **`controller/{deviceKey}/sync_response`** (Cloud Broker → Controller)
   * **QoS** : 1.
   * **Déclencheur** : En réponse immédiate à la publication d'un message `sync`. Le serveur renvoie sa liste de périphériques faisant autorité (résolution de conflits basée sur `lastUpdatedAt`).

4. **`controller/{deviceKey}/alert`** (Controller → Cloud Broker)
   * **QoS** : 1.
   * **Déclencheur** : Émis instantanément sans throttling dès qu'un périphérique local déclenche une alerte.
   * **Payload** :
     ```json
     {
       "deviceId": "device-4a2b9f"
     }
     ```

5. **`controller/{deviceKey}/command`** (Cloud Broker → Controller)
   * **QoS** : 1.
   * **Déclencheur** : Commandes instantanées émises par le tableau de bord cloud.
   * **Payload** :
     ```json
     {
       "command": "clear_alert",
       "params": "device-4a2b9f"
     }
     ```
     *(Commandes supportées : `clear_alert`, `CLEAR_ALL_ALERTS`, `REMOVE_DEVICE`, `CHANGE_MODE`, `SYNC_NOW`)*

---

### 5.3 Protocole WebSocket (Server ↔ Dashboards)
Pour garantir une réactivité instantanée (<100ms) sur les écrans de surveillance du personnel médical, l'application React n'interroge pas le serveur en boucle (polling). Elle maintient une connexion **WebSocket (WSS)** permanente sur l'URL `wss://{domain}/ws`.

* **Pings de Maintien** : Un ping-pong automatisé toutes les 25 secondes évite les déconnexions pour inactivité imposées par l'infrastructure cloud (Railway).
* **Événements WebSocket Émis par le Serveur** :
  * `FULL_STATE` : Envoyé immédiatement à la connexion du client. Contient tous les terminaux et l'état global de la passerelle.
  * `ALERT` : Émis en temps réel dès qu'un patient active son alarme.
  * `UPDATE` : Émis lors d'une approbation ou d'une modification des données d'un patient.
  * `DELETE` : Émis lorsqu'un périphérique est supprimé.
  * `CONTROLLER_STATUS` : Met à jour la force du signal Wi-Fi et l'état en ligne/hors-ligne du contrôleur central.

---

## 6. Structure du Code & Base de Données

Le projet utilise une architecture moderne basée sur TypeScript avec un monorepo structuré.

### 6.1 Arborescence du Code

```
My-PFC/
├── server/               # Backend Node.js / Express / Aedes Broker / WebSockets
│   ├── index.ts          # Point d'entrée de l'application & initialisation
│   ├── mqtt.ts           # Initialisation Aedes Broker & Client MQTT de synchronisation
│   ├── log.ts            # Utilitaire de logs centralisé pour casser les dépendances circulaires
│   ├── routes.ts         # Définition des endpoints REST HTTP & Middleware
│   ├── wss.ts            # Gestion du serveur de WebSockets
│   ├── storage.ts        # Logique d'accès aux données (PostgreSQL via Drizzle ORM)
│   ├── db.ts             # Configuration du pool de connexion PostgreSQL
│   └── static.ts         # Serveur de fichiers statiques pour le SPA React
├── shared/               # Code partagé entre le frontend et le backend
│   └── schema.ts         # Modèles de base de données Drizzle et schémas de validation Zod
├── client/               # Application Frontend React
│   ├── src/
│   │   ├── App.tsx       # Routage (wouter) et initialisation des Query Providers
│   │   ├── main.tsx      # Point de montage React dans le DOM
│   │   ├── pages/        # Écrans principaux (Dashboard, AdminPanel, Login)
│   │   ├── components/   # Composants réutilisables (DeviceCard, SetupWizard, AlertBanner)
│   │   ├── hooks/        # Hooks personnalisés (useAlertSound, use-mobile)
│   │   └── contexts/     # Provider pour l'application mobile (mobile-context)
│   └── capacitor.config.ts # Configuration de la couche mobile native
└── esp32/                # Firmware des microcontrôleurs (C++)
    ├── controller/       # Code du contrôleur central ESP32-S3
    └── device/           # Code du boîtier patient ESP8266
```

---

### 6.2 Base de Données : Schémas des Tables

Le système utilise PostgreSQL. L'accès s'effectue via Drizzle ORM avec des schémas validés en amont par la bibliothèque **Zod**.

#### Table 1 : `devices`
Enregistre la liste des boîtiers d'alerte, les informations des patients affectés et l'état de l'alerte.

| Nom de Colonne | Type SQL | Rôle / Valeur par défaut | Description |
|---|---|---|---|
| `device_id` | `TEXT` | `PRIMARY KEY` | Identifiant unique généré à partir de l'adresse MAC du boîtier (ex: `device-2b8f41`). |
| `patient_name`| `TEXT` | `""` | Nom complet du patient occupant le lit. |
| `bed` | `TEXT` | `""` | Numéro du lit du patient. |
| `room` | `TEXT` | `""` | Numéro de la chambre ou du service médical. |
| `alert_active`| `BOOLEAN`| `false` | `true` si le patient a appuyé sur le bouton et que l'alerte n'a pas été acquittée. |
| `last_alert_time`|`TEXT`| `NULL` | Horodatage ISO du dernier déclenchement d'alarme. |
| `registered`  | `BOOLEAN`| `false` | Indique si le périphérique a contacté le contrôleur au moins une fois. |
| `approved`    | `BOOLEAN`| `false` | Indique si l'administrateur a validé l'appareil et lui a affecté un lit. |
| `online`      | `BOOLEAN`| `false` | Indique si le boîtier émet des pulsations (heartbeats) actives. |
| `last_seen`   | `BIGINT`  | `Unix ms` | Timestamp Unix de la dernière activité reçue du terminal. |
| `last_updated_at`|`BIGINT` | `Unix ms` | Utilisé pour la résolution de conflits lors de la synchro Cloud. |

#### Table 2 : `system_settings`
Cette table ne contient qu'une seule ligne (Singleton, `id = 1`) et stocke l'état matériel du contrôleur central ainsi que les commandes en attente d'envoi vers celui-ci.

| Nom de Colonne | Type SQL | Valeur par défaut | Description |
|---|---|---|---|
| `id` | `INTEGER` | `PRIMARY KEY (1)` | Clé primaire unique forcée à 1. |
| `controller_last_seen` | `BIGINT` | `NULL` | Timestamp Unix de la dernière synchronisation réussie de l'ESP32. |
| `controller_uptime` | `INTEGER` | `0` | Temps de fonctionnement en secondes du contrôleur local. |
| `controller_rssi` | `INTEGER` | `0` | Force du signal Wi-Fi (en dBm) capté par le contrôleur. |
| `controller_wifi_error` | `TEXT` | `NULL` | Libellé d'erreur si la connexion Wi-Fi de l'ESP32 échoue. |
| `wifi_mode` | `INTEGER` | `1` | Mode réseau configuré (1 à 4). |
| `pending_command` | `TEXT` | `NULL` | Type de commande en attente (ex: `clear_alert`, `REMOVE_DEVICE`, `CHANGE_MODE`). |
| `command_params` | `TEXT` | `NULL` | Paramètres complémentaires pour la commande (ex: l'ID de l'appareil à supprimer). |

---

## 7. Sécurité, Résilience et Performance du Système

Pour assurer un service critique de type médical, plusieurs mécanismes de protection ont été intégrés :

1. **Isolation locale en cas de panne Internet** :
   En mode 4, si la liaison Internet/Cloud est coupée, l'ESP32-S3 continue d'héberger localement son broker MQTT. Les infirmiers présents dans l'unité de soin peuvent toujours se connecter en Wi-Fi direct sur l'ESP32 (`192.168.4.1`) pour surveiller les alertes et les acquitter. Une fois la connexion Internet rétablie, la synchronisation avec le cloud se relance de manière transparente.
2. **File de commandes d'administration persistante (Command Queue)** :
   Le serveur intègre une file d'attente de commandes temporaire. Si le contrôleur local se déconnecte temporairement de la couverture 3G/4G/Wi-Fi du cloud, les commandes de coupure d'alarme ou de suppression d'équipements émises par le personnel distant ne sont pas perdues. Elles sont stockées au niveau de la passerelle serveur et automatiquement poussées vers l'ESP32 avec une priorité absolue (QoS 1) dès qu'il rétablit sa session.
3. **Protection contre la fragmentation mémoire (Heap)** :
   Sur l'ESP32-S3, le traitement de chaînes dynamiques répétées dans la boucle `loop()` peut provoquer une fragmentation du tas mémoire (Heap Fragmentation), entraînant des redémarrages intempestifs. L'architecture pré-cache l'ensemble des topics MQTT lors de l'initialisation, garantissant la stabilité à long terme de l'équipement (zéro allocation de chaîne volatile par trame).
4. **Détection de perte de liaison (Heartbeat & Timeout)** :
   * Si un terminal patient (ESP8266) n'envoie pas de pulsation pendant plus de **30 secondes**, le contrôleur local le marque comme hors-ligne (`online = false`) et coupe son voyant sur l'interface, avertissant le personnel d'un dysfonctionnement matériel potentiel.
   * Si le contrôleur local (ESP32-S3) n'a pas émis de ping pendant plus de **120 secondes**, le serveur cloud marque le contrôleur comme hors-ligne, ce qui déclenche un indicateur visuel rouge d'alerte système globale sur l'application Web générale.
5. **Sécurité des APIs** :
   * Les communications Cloud/ESP32 sont protégées par une clé API unique (`X-Device-Key` ou `DEVICE_API_KEY`) exigée lors du handshake MQTT.
   * L'accès au panneau d'administration nécessite une session authentifiée (gérée par Passport.js avec des cookies sécurisés `httpOnly` et configurés en mode `sameSite: "none"` pour le fonctionnement multi-plateforme Vercel-Railway).
