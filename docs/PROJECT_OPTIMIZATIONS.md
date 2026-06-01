# Évolutions et Optimisations du Projet (PFC)

Ce document récapitule les optimisations techniques, logicielles et matérielles apportées au **Système d'Alerte Médicale pour Patients Hospitalisés** depuis sa version initiale. Ces informations sont structurées pour être directement intégrées dans les chapitres *Réalisation*, *Implémentation* ou *Résultats* de votre mémoire.

---

## 1. Optimisation de l'Architecture Serveur
### Migration de Vercel (Serverless) vers un Monolithe sur Railway

* **État Initial :** L'application frontend React était déployée sur Vercel et communiquait avec des fonctions API ou des services tiers.
* **Problématique :** Les architectures *Serverless* (comme Vercel) imposent des temps d'exécution courts (timeouts) et ne prennent pas en charge les protocoles persistants comme les WebSockets ou les connexions TCP MQTT continues, indispensables pour un système d'alerte médicale en temps réel.
* **Solution d'Optimisation :** Migration vers une **architecture cloud monolithique centralisée hébergée sur Railway**. L'ensemble de la logique est exécuté au sein d'un processus Node.js unique :
  - **Serveur API (Express) :** Traitement des requêtes administratives rapides.
  - **Serveur de WebSockets (WSS) :** Maintien d'un canal bidirectionnel persistant pour l'UI des infirmiers.
  - **Courtier MQTT Aedes intégré :** Réception et traitement direct du trafic TCP (port 1883) provenant des modules IoT.
* **Bilan :** Réduction du temps de latence de transmission des alarmes à **moins de 100 ms** et élimination complète des déconnexions intempestives.

---

## 2. Optimisation Mobile
### Migration de React Native vers Capacitor (WebView Dynamique)

* **État Initial :** L'application mobile destinée aux infirmiers était développée avec React Native.
* **Problématique :** En milieu hospitalier, toute modification de l'interface ou correction de bug critique nécessitait la recompilation complète d'un fichier APK et sa redistribution manuelle sur les terminaux Android des soignants.
* **Solution d'Optimisation :** Migration vers **Capacitor**. L'application mobile se comporte comme une coquille native encapsulant l'application React servie en production par Railway.
* **Bilan :**
  - **Mise à jour Over-The-Air (OTA) :** Les mises à jour de l'UI et des fonctionnalités sont appliquées instantanément dès la mise en ligne du serveur cloud, sans nécessiter d'action de l'utilisateur ou de recompilation.
  - **Intégration Native :** Conservation de l'accès aux API natives du smartphone (contrôle matériel du vibreur pour les alertes urgentes et notifications).

---

## 3. Résilience et Tolérance aux Pannes
### Implémentation d'une File d'Attente de Commandes Hors-Ligne (Offline Queue)

* **Problématique :** Une coupure Wi-Fi temporaire ou une instabilité réseau pouvait entraîner la perte de paquets critiques (tels que la commande d'acquittement d'une alarme par un soignant).
* **Solution d'Optimisation :** Ajout d'une file d'attente d'arrière-plan résiliente dans le module MQTT (`server/mqtt.ts`).
* **Bilan :** En cas de déconnexion, les paquets non acquittés sont mis en mémoire tampon. Dès que la connexion est rétablie, le système effectue une **synchronisation silencieuse en arrière-plan** avec la base de données PostgreSQL, garantissant zéro perte d'information médicale.

---

## 4. Flexibilité du Déploiement Hospitalier
### Architecture Réseau Adaptative à 4 Modes

* **Problématique :** Les infrastructures réseau des hôpitaux varient grandement : certains services interdisent l'accès internet pour des raisons de sécurité, d'autres disposent de routeurs d'entreprise complexes, tandis que les cliniques de campagne n'ont aucun réseau disponible.
* **Solution d'Optimisation :** Développement d'une architecture réseau adaptative gérée par le contrôleur ESP32-S3 :
  1. **Mode 1 : Local Access Point (AP Only) :** L'ESP32-S3 crée son propre réseau Wi-Fi sécurisé nommé `HospitalAlarm`. Aucune infrastructure externe ou connexion internet n'est requise.
  2. **Mode 2 : Local Station (STA Only) :** Le contrôleur et les terminaux se connectent au réseau Wi-Fi local de l'hôpital.
  3. **Mode 3 : Redundant Hybrid (AP + STA) :** L'ESP32-S3 se connecte au Wi-Fi de l'hôpital tout en propageant son réseau local de secours. Les boîtiers patients basculent automatiquement sur le réseau de secours en cas de coupure de la ligne principale.
  4. **Mode 4 : Cloud Synchronization (Online) :** Synchronisation bidirectionnelle complète avec le serveur Cloud Railway et la base de données PostgreSQL.
* **Bilan :** Résilience totale en cas de panne internet. L'alarme physique (buzzer local connecté au contrôleur) fonctionne de manière autonome en local.

---

## 5. Optimisation Énergétique et Matérielle du Nœud Patient (ESP8266)
### Prolongement de la Durée de Vie de la Batterie

* **Problématique :** Les boîtiers d'appel d'urgence portables fonctionnent sur batterie Li-ion 3.7V rechargeable et doivent assurer un service continu prolongé.
* **Solution d'Optimisation :**
  - **Mode Sommeil Profond (Deep Sleep) :** Le module ESP8266 est maintenu dans un état de veille maximale. Son courant de repos est de l'ordre de quelques microampères. Le processeur n'est réveillé que lors de l'appui physique sur le bouton SOS via une interruption matérielle (`falling edge`).
  - **Anti-rebond Logique et Matériel :** Une routine de validation de 50 millisecondes élimine les fausses alertes générées par le bruit mécanique du bouton poussoir.
  - **Visualisation Active-Low :** Programmation de la LED d'état intégrée sur la broche GPIO 2 en mode logique basse pour consommer moins de courant.

---

## 6. Optimisation de la Couche de Données
### Utilisation de Drizzle ORM et Pool de Connexion

* **Solution d'Optimisation :** Migration vers l'ORM léger **Drizzle** en remplacement d'ORM plus lourds, configuré avec un gestionnaire de connexions PostgreSQL (Connection Pooling) et protocole SSL crypté.
* **Bilan :** 
  - Réduction de l'empreinte mémoire du serveur Node.js.
  - Prévention de la saturation de la base de données grâce à un recyclage optimal des connexions inactives (`Pool Timeout 15s`).
