# Hospital Alarm System — Hosting Guide

This folder walks you through deploying the Hospital Alarm System online so it can be accessed from anywhere.

## Architecture Overview

```
[ESP32 Master]
    │
    │  HTTP POST (alerts, heartbeats, registration)
    ▼
[Railway — Full Stack]   ◄──── [Admin / Staff Browser]
    │  (Backend API + Frontend Dashboard)
    │
    └─ https://your-app.up.railway.app
```

> Railway hosts everything: the Express backend AND the React frontend are served from the same URL. No separate frontend hosting needed.

---

## Step-by-Step Guides

### 1. [Deploy on Railway](./railway.md)
Deploy the full-stack app (backend + frontend) on Railway. This is the only hosting step needed.

### 2. [Configure ESP32 for Online Mode](./esp32-online-setup.md)
After hosting, flash and configure your ESP32 master to point to your hosted Railway server.

---

## Quick Checklist

- [ ] Code pushed to a GitHub repository
- [ ] App deployed on Railway with correct environment variables
- [ ] ESP32 flashed with `online/master/master.ino`
- [ ] ESP32 configured with Railway URL and device API key
- [ ] Test: open `https://your-app.up.railway.app` → dashboard loads
- [ ] Test: open `https://your-app.up.railway.app/admin` → login with `admin / admin1234`
