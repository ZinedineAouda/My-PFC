# Hospital Alarm System — Hosting Guide

This folder walks you through deploying the Hospital Alarm System online so it can be accessed from anywhere.

## Architecture Overview

```
[ESP32 Master]
    │
    │  HTTP POST (alerts, heartbeats, registration)
    ▼
[Railway — Backend API]   ◄──── [Admin / Staff Browser]
    │                                      │
    │  CORS + REST API                     │
    ▼                                      │
[Vercel — Frontend Dashboard] ◄───────────┘
```

---

## Step-by-Step Guides

### 1. [Deploy Backend on Railway](./railway.md)
The backend Express server receives data from the ESP32 and serves the REST API.

### 2. [Deploy Frontend on Vercel](./vercel.md)
The React dashboard is deployed as a static site and communicates with the Railway backend.

### 3. [Configure ESP32 for Online Mode](./esp32-online-setup.md)
After hosting, flash and configure your ESP32 master to point to your hosted server.

---

## Quick Checklist

- [ ] Code pushed to a GitHub repository
- [ ] Backend deployed on Railway with correct environment variables
- [ ] Frontend deployed on Vercel with `VITE_API_URL` set
- [ ] `FRONTEND_URL` set on Railway to your Vercel URL (for CORS)
- [ ] ESP32 flashed with `online/master/master.ino`
- [ ] ESP32 configured with server URL and device API key
- [ ] Test: open Vercel URL → dashboard loads
- [ ] Test: open Vercel URL `/admin` → login with `admin / admin1234`
