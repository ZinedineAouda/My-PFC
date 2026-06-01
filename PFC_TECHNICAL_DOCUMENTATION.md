# Projet de Fin de Cycle (PFC) — Documentation Technique Complète
## Système d'Alerte Médicale pour Patients Hospitalisés (Hospital Patient Alarm System)

---

## 1. Introduction Générale

Le présent projet consiste en la conception et la réalisation d'un **système d'alerte médicale connecté (Hospital Patient Alarm System)**. Ce système permet aux patients hospitalisés d'émettre des alertes d'urgence depuis leur lit vers un tableau de bord surveillé par le personnel soignant (infirmiers/médecins) ou via leurs appareils mobiles.

### 1.1 Problématique
Dans les établissements hospitaliers, la réactivité lors d'un appel d'urgence est un facteur critique. Les systèmes traditionnels filaires sont coûteux à installer, difficiles à maintenir et manquent de flexibilité. Les systèmes purement basés sur le cloud peuvent faillir en cas de perte de connexion Internet locale.

### 1.2 Solution Proposée
Pour répondre à cette double problématique de fiabilité et de mobilité, nous proposons un système hybride :
1. **Un réseau local robuste** basé sur le protocole **MQTT** où un contrôleur central **ESP32-S3** gère en local des terminaux d'appel d'urgence **ESP8266**. Ce réseau fonctionne de manière 100% autonome pour déclencher des alarmes sonores physiques.
2. **Une Plateforme Cloud Monolithique (Railway)** qui combine le backend Node.js, un broker MQTT (Aedes) intégré, une base de données PostgreSQL et sert le tableau de bord React (SPA).
3. **Une application Mobile Native (Capacitor)** pour Android, qui enveloppe dynamiquement la plateforme cloud et permet aux infirmiers de recevoir les alertes sur leur smartphone (vibrations et interface en temps réel) sans nécessiter de recompilation constante de l'APK.

---

## 2. Architecture Globale du Système

Le système a récemment évolué vers une architecture **Cloud Monolithique centralisée**, tout en conservant la résilience locale via la passerelle ESP32.

```
+------------------------------------------------------------+
|                     CLOUD PLATFORM (Railway)               |
|   - Backend : Node.js (Express)                            |
|   - Broker MQTT Cloud : Aedes (Intégré dans Node.js)       |
|   - Base de Données : PostgreSQL                           |
|   - Serveur Web : Hébergement statique de l'UI React       |
+-----------------------^------------------------------------+
                        | API HTTPS & WebSockets (WSS) & MQTT TCP (Port 1883)
+-----------------------v------------------------------------+
|                 ESP32-S3 CONTROLLER (Passerelle)           |
|   - Passerelle IoT Locale                                  |
|   - Buzzer d'alerte physique local                         |
|   - Connexion Wi-Fi au réseau de l'hôpital                 |
+------^-------------^-------------^-------------------------+
       | MQTT (Local / Cloud)
+------v------+ +----v------+ +---v-------+
|   ESP8266   | |   ESP8266   | |   ESP8266   |
|  Chambre 1  | |  Chambre 2  | |  Chambre N  |
+-------------+ +-----------+ +-----------+
```

### 2.1 Les Terminaux Patients (ESP8266)
Chaque lit de patient est équipé d'un boîtier doté d'un bouton poussoir relié à un module Wi-Fi low-cost **ESP8266**. Ce module communique en Wi-Fi avec le contrôleur et/ou directement avec le broker MQTT Cloud (Aedes) hébergé sur Railway.

### 2.2 Le Contrôleur Local (ESP32-S3)
Le cœur logique matériel du réseau hospitalier est un **ESP32-S3**. Il cumule plusieurs rôles :
* **Générateur d'alerte physique** via un avertisseur sonore (buzzer actif) connecté à une broche GPIO.
* **Passerelle IoT** qui peut servir de relais pour la synchronisation locale vers le cloud.

### 2.3 Le Serveur Cloud Monolithique (Node.js sur Railway)
Un serveur Node.js unique exécutant Express traite la totalité du trafic :
* **Broker MQTT (Aedes)** : Intégré directement dans le même processus Node.js. Il reçoit les connexions TCP sur le port 1883 des appareils IoT ESP32/ESP8266 et relaye les données en interne vers le reste de l'application.
* **API & WebSockets** : Sert l'API REST sécurisée, ainsi que le flux WebSocket en temps réel vers les terminaux web des infirmiers.
* **Base de données** : Persiste l'état du système via **PostgreSQL** géré via l'ORM **Drizzle**.
* **Serveur Statique** : Diffuse l'application frontend React compilée (`dist/public`).

### 2.4 Le Dashboard Web & Mobile (Capacitor)
Développé en **React 18** avec **Tailwind CSS** et **Radix UI**, ce tableau de bord affiche en temps réel les appels de patients grâce à une connexion **WebSocket**.
Pour la version mobile, **Capacitor** a remplacé React Native. Il agit comme un conteneur Web (WebView) ultra-léger et performant qui charge l'URL de production Railway. Cette approche permet de déployer de nouvelles mises à jour UI instantanément, sans jamais nécessiter la reconstruction ou la redistribution de l'APK (fichier `.apk` Android).

---

## 3. Spécifications du Matériel (Hardware)

### 3.1 Schéma des Connexions & Broches (Pin Mapping)

#### A. Contrôleur Central (ESP32-S3 DevKitC-1)
Le contrôleur n'a besoin que d'un buzzer pour l'alarme sonore physique locale.

| Broche ESP32-S3 | Composant | Rôle | Description |
|:---:|:---:|:---:|---|
| **GPIO 4** | Buzzer Actif (+) | Sortie Numérique | Activé (HIGH) lors d'une alerte, désactivé (LOW) sinon. |
| **GND** | Buzzer Actif (-) | Masse | Référence 0V |
| **USB-C** | Port Série / Alim | Alimentation + Debug | 5V / Communication Série à 115200 Bauds |

#### B. Terminal Bouton Patient (ESP8266 NodeMCU)
Chaque boîtier patient possède un bouton-poussoir pour déclencher l'alerte et une LED d'état.

| Broche ESP8266 | Composant | Mode de Configuration | Description |
|:---:|:---:|:---:|---|
| **GPIO 0 (D3)** | Bouton Poussoir | `INPUT_PULLUP` | Connecté au GND lors de l'appui (Actif à l'état BAS). |
| **GPIO 2 (D4)** | LED intégrée | `OUTPUT` | Active à l'état BAS (Active LOW). Indique le statut. |

### 3.2 Comportement Lumineux de la LED du Terminal (ESP8266)
La LED du terminal donne un retour visuel instantané sur son état :
* **Clignotement rapide (200ms)** : En cours de connexion au réseau Wi-Fi ou au courtier MQTT.
* **Clignotement lent (500ms)** : Connecté au réseau, mais en attente d'approbation administrative sur le tableau de bord.
* **Allumée Fixe** : Alerte active (le patient a appuyé sur le bouton).
* **Éteinte** : Connecté, approuvé par l'administrateur, système au repos.

---

## 4. Protocoles de Communication

Le projet exploite trois protocoles distincts adaptés à chaque niveau de l'architecture :

```
[Terminal ESP8266] <== MQTT (TCP 1883) ==> [Aedes Broker (Node.js Railway)] <== WebSockets (WSS) ==> [Dashboard Client/Capacitor]
```

### 4.1 Protocole MQTT (Réseau IoT)
Le protocole MQTT (Message Queuing Telemetry Transport) est utilisé par les appareils IoT pour communiquer avec le serveur Railway de manière extrêmement réactive.
Le client Node.js backend s'abonne lui-même au broker intégré pour interagir avec la base de données PostgreSQL.

* **Pings de Maintien** : Les périphériques envoient des pings réguliers (`controller/+/ping`) pour informer le serveur de leur santé (RSSI, uptime).
* **Alertes** : Un appui sur le bouton publie instantanément sur le topic `controller/{deviceId}/alert`, ce qui déclenche une insertion en base de données et l'émission immédiate d'un message WebSocket vers l'UI des infirmiers.

### 4.2 API REST HTTPS
En supplément du MQTT, le serveur Express expose des endpoints REST classiques pour les fonctionnalités administratives (approbation de périphériques, acquittement manuel d'alertes via HTTP) sécurisés par `X-Admin-Token` ou cookies de session.

### 4.3 Protocole WebSocket (Server ↔ Dashboards)
Pour garantir une réactivité instantanée (<100ms) sur les écrans de surveillance du personnel médical (Mobile ou Web), l'application React maintient une connexion **WebSocket (WSS)** permanente.

---

## 5. Structure du Code & Base de Données

Le projet utilise une architecture moderne basée sur TypeScript avec un monorepo structuré.

### 5.1 Arborescence du Code

```
My-PFC/
├── server/               # Backend Node.js / Express / WebSockets / Aedes MQTT
│   ├── index.ts          # Point d'entrée de l'application & initialisation de la base
│   ├── routes.ts         # Définition des endpoints REST HTTP & Middleware
│   ├── wss.ts            # Gestion du serveur de WebSockets
│   ├── mqtt.ts           # Logique du broker MQTT intégré (Aedes) et gestion des queues de commandes
│   ├── storage.ts        # Logique d'accès aux données (PostgreSQL via Drizzle ORM)
│   ├── db.ts             # Configuration du pool de connexion PostgreSQL
│   └── static.ts         # Serveur de fichiers statiques pour le SPA React
├── shared/               # Code partagé entre le frontend et le backend
│   └── schema.ts         # Modèles de base de données Drizzle et schémas de validation Zod
├── client/               # Application Frontend React (Servie par Node.js)
│   ├── src/
│   │   ├── App.tsx       # Routage (wouter) et initialisation
│   │   ├── components/   # Composants réutilisables (DeviceCard, SetupWizard, AlertBanner)
│   │   └── hooks/        # Hooks personnalisés (useAlertSound, use-mobile)
│   └── vite.config.ts    # Build frontend
├── android/              # Application Mobile (Capacitor WebView)
│   └── capacitor.config.ts # Pointe l'application mobile vers https://my-pfc-production.up.railway.app
└── esp32/                # Firmware des microcontrôleurs (C++)
```

### 5.2 Base de Données : Schémas des Tables

Le système utilise PostgreSQL sur Railway avec un paramétrage optimisé pour le Cloud (Pool Timeout 15s, SSL configuré). L'accès s'effectue via Drizzle ORM avec des schémas validés en amont par **Zod**.

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
| `last_updated_at`|`BIGINT` | `Unix ms` | Timestamp de modification. |

#### Table 2 : `system_settings`
Cette table stocke l'état matériel du contrôleur central ainsi que les commandes en attente d'envoi.

| Nom de Colonne | Type SQL | Valeur par défaut | Description |
|---|---|---|---|
| `id` | `INTEGER` | `PRIMARY KEY (1)` | Clé primaire unique forcée à 1. |
| `controller_last_seen` | `BIGINT` | `NULL` | Timestamp Unix de la dernière synchronisation de l'ESP32. |
| `controller_uptime` | `INTEGER` | `0` | Temps de fonctionnement en secondes du contrôleur. |
| `controller_rssi` | `INTEGER` | `0` | Force du signal Wi-Fi (en dBm). |
| `controller_wifi_error` | `TEXT` | `NULL` | Libellé d'erreur de connexion Wi-Fi. |
| `wifi_mode` | `INTEGER` | `1` | Mode réseau configuré (1 à 4). |
| `pending_command` | `TEXT` | `NULL` | Type de commande en attente. |
| `command_params` | `TEXT` | `NULL` | Paramètres complémentaires pour la commande. |

---

## 6. Sécurité et Avantages de la Nouvelle Architecture Monolithique

1. **Mises à Jour Dynamiques Mobiles (Capacitor)** :
   La migration de React Native vers Capacitor WebView signifie que la logique applicative de l'application mobile est intégralement gérée par le backend Railway. Toute modification d'interface, correction de bug ou nouvelle fonctionnalité est répercutée instantanément sur l'application des infirmiers sans qu'ils n'aient à mettre à jour leur fichier APK. L'application mobile se comporte comme une coquille native (avec support des vibrations et notifications locales) contenant le tableau de bord web.
2. **Déploiement Simplifié et Fiabilité (Monolith Railway)** :
   Vercel, n'étant adapté qu'aux fonctions Serverless, souffrait de limitations critiques concernant les temps d'exécution (timeouts) et les WebSockets. Héberger l'intégralité du code (Frontend statique, Express API, Aedes MQTT) au sein d'un seul processus Node.js sur Railway garantit une réactivité MQTT et WebSocket ininterrompue.
3. **Queue de Commandes Déconnectées** :
   Le nouveau module MQTT interne (`server/mqtt.ts`) intègre un mécanisme intelligent de mise en file d'attente. Si la communication entre le backend MQTT client et le broker s'interrompt temporairement, les commandes vitales d'acquittement d'alarme sont mises en attente et re-émises immédiatement lors de la reconnexion, assurant zéro perte d'information médicale critique.
4. **Authentification Hybride** :
   L'accès à l'API est sécurisé doublement : les requêtes de terminaux IoT utilisent un token statique d'appareil (`X-Device-Key` / `X-Admin-Token`), tandis que le panneau d'administration des soignants emploie le framework Passport.js avec des cookies cryptés.
