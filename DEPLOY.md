# Guide de Déploiement — Monolithe Full-Stack sur Railway (MQTT inclus)

Ce guide décrit le processus étape par étape pour déployer le **Système d'Alerte Médicale pour Patients Hospitalisés (My-PFC)** en production.

Dans l'architecture moderne du système, tout est regroupé dans un **monolithe haute performance** déployé sur **Railway** :
1. Le **Serveur API Express** (Node.js/TypeScript)
2. Le **Tableau de bord de surveillance React** (servi statiquement par le serveur en production)
3. Le **Courtier MQTT Aedes** intégré (pour recevoir les alertes temps réel de l'ESP32)

---

## 📋 Prérequis de déploiement

Avant de commencer, assurez-vous d'avoir :
- Un compte [GitHub](https://github.com)
- Un compte [Railway](https://railway.app) (l'offre Hobby ou Pro convient parfaitement)
- Une base de données PostgreSQL provisionnée (sur Railway ou externe)
- Le code de votre projet poussé sur un dépôt GitHub public ou privé

---

## 🚀 Étape 1 : Pousser le code sur GitHub

Initialisez git et poussez votre code si ce n'est pas déjà fait :

```bash
git init
git add .
git commit -m "feat: migration temps réel MQTT"
git branch -M main
git remote add origin https://github.com/VOTRE_NOM/VOTRE_REPO.git
git push -u origin main
```

---

## 🛠️ Étape 2 : Déploiement sur Railway

Railway utilise le `Dockerfile` à la racine pour construire et exécuter automatiquement le projet.

### 2.1 Créer le projet et lier le dépôt
1. Connectez-vous sur [railway.app](https://railway.app).
2. Cliquez sur **"New Project"** → **"Deploy from GitHub repo"**.
3. Sélectionnez le dépôt de votre projet.
4. Laissez Railway détecter le `Dockerfile` et lancer le premier build.

### 2.2 Ajouter la base de données PostgreSQL (Recommandé)
Le système nécessite PostgreSQL pour stocker l'état des patients, des chambres et l'approbation des terminaux.
1. Sur le tableau de bord de votre projet Railway, cliquez sur **"New"** (bouton en haut à droite) → **"Database"** → **"Add PostgreSQL"**.
2. Railway va provisionner une instance de base de données PostgreSQL instantanément dans le même réseau privé.
3. Railway va automatiquement lier la variable `DATABASE_URL` à votre service d'application principal.

### 2.3 Variables d'environnement requises
Allez dans votre service principal (le service lié à votre dépôt GitHub) → Onglet **Variables**, et ajoutez les variables suivantes :

| Variable | Exemple de Valeur / Type | Rôle / Description |
|:---|:---|:---|
| `NODE_ENV` | `production` | Active les optimisations de production et le service statique du dashboard. |
| `SESSION_SECRET` | `un_secret_tres_long_et_securise_123` | Clé utilisée pour chiffrer les sessions de connexion administrateur. |
| `DEVICE_API_KEY` | `super` (ou votre clé sécurisée) | Clé secrète de sécurité partagée avec le contrôleur ESP32-S3. |
| `DATABASE_URL` | *Généré automatiquement par Railway* | URL de connexion à la base de données PostgreSQL (ex: `postgresql://...`). |
| `PORT` | `5000` *(géré par Railway)* | Port sur lequel le serveur Express écoute les requêtes HTTP. |

---

## 🔌 Étape 3 : Configuration du Courtier MQTT (Aedes)

Le contrôleur ESP32-S3 doit se connecter au serveur en MQTT (Mode 4). Deux approches de déploiement sont possibles pour le transport MQTT :

### 🚨 Option A : Utilisation du courtier MQTT intégré (Aedes) sur Railway (Recommandé avec Proxy TCP)
Par défaut, si vous ne spécifiez pas d'hôte externe, le serveur démarre un courtier MQTT intégré (Aedes) sur le port `1883`.

Pour que l'ESP32 puisse l'atteindre depuis l'extérieur de Railway :
1. Allez dans les **Settings** de votre service principal sur Railway.
2. Descendez jusqu'à la section **Networking**.
3. Cliquez sur **"Add TCP Port"** (Ajouter un port TCP).
4. Saisissez le port interne `1883`.
5. Railway va générer un domaine TCP externe (ex: `tcp://roundhouse.proxy.rlwy.net:25432`).
6. **Notez bien cette adresse** : c'est l'adresse que vous saisirez dans l'assistant de configuration de l'ESP32 (Hôte: `roundhouse.proxy.rlwy.net`, Port: `25432`).

### ☁️ Option B : Utilisation d'un courtier MQTT Cloud externe (HiveMQ, EMQX, Adafruit)
Si vous préférez ne pas exposer de port TCP sur Railway, vous pouvez utiliser un service de courtier MQTT managé gratuit (ex: HiveMQ Cloud, EMQX Serverless).

Ajoutez les variables d'environnement suivantes sur Railway pour connecter votre backend au courtier externe :

| Variable | Exemple de Valeur | Description |
|:---|:---|:---|
| `MQTT_BROKER_HOST` | `broker.hivemq.com` ou `xxxxxx.s1.eu.hivemq.cloud` | Hôte du courtier MQTT externe |
| `MQTT_BROKER_PORT` | `1883` ou `8883` (sécurisé) | Port de connexion (8883 pour MQTTS TLS) |
| `MQTT_BROKER_USER` | `votre_utilisateur` *(optionnel)* | Nom d'utilisateur MQTT |
| `MQTT_BROKER_PASS` | `votre_mot_de_passe` *(optionnel)* | Mot de passe de connexion MQTT |

*Si ces variables sont définies, le contrôleur ESP32-S3 et le serveur Railway se connecteront tous les deux à ce courtier externe en tant que clients. C'est l'approche la plus stable pour les déploiements de type production avec pare-feux stricts.*

---

## 📱 Étape 4 : Déploiement Mobile (Capacitor & APK)

L'application intègre **Capacitor** pour compiler le dashboard React en application mobile native Android.

### 4.1 Générer le Domaine Public Web
1. Dans l'onglet **Settings** → **Networking** de votre service principal Railway.
2. Cliquez sur **"Generate Domain"** (Générer un domaine public).
3. Vous obtiendrez une URL sécurisée (ex: `https://my-pfc-production.up.railway.app`).

### 4.2 Lier l'application mobile à la production
1. Ouvrez le fichier [capacitor.config.ts](file:///c:/Users/zined/Documents/GitHub/My-PFC/capacitor.config.ts).
2. Remplacez la propriété `url` par votre domaine de production Railway :
   ```typescript
   server: {
     url: 'https://votre-app.up.railway.app',
     cleartext: true,
     allowNavigation: ['votre-app.up.railway.app'],
   }
   ```
3. Exécutez la synchronisation et ouvrez Android Studio pour compiler votre APK :
   ```bash
   npx cap sync android
   npx cap open android
   ```

---

## 🔍 Étape 5 : Vérification du Déploiement

### 5.1 Vérifier la disponibilité de l'API
Ouvrez l'URL de votre application sur le point de terminaison `/health` (ex: `https://votre-app.up.railway.app/health`).
- **Attendu** : Un texte simple `"OK"` indiquant que le serveur Express fonctionne.

### 5.2 Accéder au Tableau de Bord
1. Ouvrez l'URL principale de votre déploiement Railway (ex: `https://votre-app.up.railway.app`).
2. Le tableau de bord de l'hôpital doit s'afficher magnifiquement avec la mention de connexion active.
3. Pour administrer : naviguez vers `/admin` (identifiants par défaut : `admin` / `admin1234`).

---

## 🛠️ Dépannage en Production

**Les alertes ne remontent pas sur le site web :**
- Vérifiez dans les logs Railway (onglet **Logs**) que le client MQTT du serveur est bien connecté : `Backend MQTT Client connected successfully!`.
- Si vous utilisez l'option A (Broker intégré), assurez-vous que le port TCP externe de Railway est correctement mappé sur le port interne `1883`.
- Si vous utilisez l'option B (Broker externe), vérifiez que l'ESP32 et le serveur Railway utilisent exactement les mêmes identifiants et le même hôte MQTT.

**Erreurs de connexion de base de données :**
- Vérifiez que la variable `DATABASE_URL` est présente. Si vous avez ajouté le module PostgreSQL après le premier déploiement, vous devrez peut-être déclencher manuellement un nouveau build de votre application (ou cliquer sur **Redeploy** dans Railway).
