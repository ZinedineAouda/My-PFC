# Hospital Patient Alarm System

## Overview
A web-based control panel for a hospital patient alarm system designed for ESP32-S3 IoT devices. The system monitors patient call buttons and displays real-time alerts on a dashboard. Supports 3 network modes: AP, STA, and AP+STA hybrid.

Two versions available:
- **Offline** - ESP32 master hosts everything locally, no internet needed
- **Online** - ESP32 master forwards data to this hosted web server for remote monitoring

## Architecture
- **Frontend:** React + TypeScript with Vite, Tailwind CSS, shadcn/ui components
- **Backend:** Express.js with session-based authentication + device API key auth for ESP32
- **Storage:** In-memory (intentional - data resets on restart, matches ESP32 deployment target)
- **Routing:** wouter for client-side routing

## ESP32 Code Structure
- `esp32/offline/master/master.ino` - Offline master (hosts dashboard + API locally)
- `esp32/offline/slave/slave.ino` - Slave (identical in both versions)
- `esp32/online/master/master.ino` - Online master (forwards data to hosted server)
- `esp32/online/slave/slave.ino` - Slave (identical copy)
- `esp32/master/master.ino` - Original offline master (kept for reference)
- `esp32/slave/slave.ino` - Original slave (kept for reference)
- `esp32/README.md` - Full hardware setup and usage guide

## Pages
- `/` - Public dashboard showing approved patient devices with real-time alert indicators (polls every 2 seconds)
- `/admin` - Admin panel with pending device approval and device management
- `/setup` - Network mode selection wizard (AP/STA/AP+STA)

## ESP32 Network Modes
- **AP Mode** - Master creates own Wi-Fi, all devices connect directly (offline only)
- **STA Mode** - All devices join existing hospital Wi-Fi
- **AP+STA Mode** - Hybrid: master creates AP and joins hospital Wi-Fi simultaneously

## ESP32 Device Flow
1. Master boots -> shows setup wizard to select network mode
2. Slave boots -> creates temporary AP "SlaveSetup-XXXX" with web page for Wi-Fi config
3. Slave connects to network and auto-registers with master as "pending"
4. Admin approves pending devices and enters patient info
5. Approved devices appear on dashboard, alerts work via one-shot system

## Online Mode Flow
1. ESP32 master receives slave registration/alert locally
2. Master forwards the event to hosted server via HTTP with X-Device-Key header
3. Hosted dashboard updates in real-time (polling every 2 seconds)
4. Admin manages devices from hosted admin panel (/admin)

## API Endpoints
- `GET /api/slaves` - List devices (?approved=1 for dashboard, ?all=1 for admin)
- `GET /api/scan` - Scan nearby Wi-Fi networks
- `GET /api/status` - System status and mode info
- `POST /api/setup` - Set network mode
- `POST /api/connect-wifi` - Connect to hospital Wi-Fi
- `POST /api/register` - Slave auto-registration (device key auth)
- `POST /api/alert` - Trigger alert (device key auth, one-shot)
- `POST /api/heartbeat` - Update slave lastSeen status (device key auth)
- `POST /api/approve/:id` - Approve pending device with patient info (admin)
- `PUT /api/slaves/:id` - Update device info (admin)
- `DELETE /api/slaves/:id` - Remove device (admin)
- `POST /api/clearAlert/:id` - Clear active alert (admin or device key)

## Authentication
- **Admin:** Session-based, login at /admin with admin/admin1234
- **Device (ESP32):** X-Device-Key header, validated against DEVICE_API_KEY env var. If env var not set, device endpoints are open (for offline/testing).

## Environment Variables
- `SESSION_SECRET` - Express session secret
- `DEVICE_API_KEY` - API key for ESP32 device authentication (online mode)

## Key Files
- `shared/schema.ts` - Zod schemas and TypeScript types
- `server/storage.ts` - In-memory storage with CRUD operations
- `server/routes.ts` - Express routes with session auth + device key middleware
- `client/src/pages/dashboard.tsx` - Public monitoring dashboard
- `client/src/pages/admin.tsx` - Admin panel

## Recent Changes
- 2026-02-24: Added online mode - ESP32 forwards data to hosted server, device API key auth, reorganized ESP32 code into offline/ and online/ folders, added heartbeat endpoint
- 2026-02-21: Major rewrite - added 3 network modes (AP/STA/AP+STA), setup wizard, slave web config page, auto-detection with admin approval flow, redesigned UI with modern effects
- 2026-02-21: Initial MVP build with dashboard, admin panel, REST API, seed data
