# Hospital Patient Alarm System

## Overview
A web-based control panel for a hospital patient alarm system designed for ESP32-S3 IoT devices. The system monitors patient call buttons and displays real-time alerts on a dashboard. Supports 3 network modes: AP, STA, and AP+STA hybrid.

## Architecture
- **Frontend:** React + TypeScript with Vite, Tailwind CSS, shadcn/ui components
- **Backend:** Express.js with session-based authentication
- **Storage:** In-memory (intentional - data resets on restart, matches ESP32 deployment target)
- **Routing:** wouter for client-side routing
- **ESP32 Code:** Arduino .ino files in esp32/master/ and esp32/slave/

## Pages
- `/` - Public dashboard showing approved patient devices with real-time alert indicators (polls every 2 seconds)
- `/admin` - Admin panel with pending device approval and device management
- `/setup` - Network mode selection wizard (AP/STA/AP+STA)

## ESP32 Network Modes
- **AP Mode** - Master creates own Wi-Fi, all devices connect directly
- **STA Mode** - All devices join existing hospital Wi-Fi
- **AP+STA Mode** - Hybrid: master creates AP and joins hospital Wi-Fi simultaneously

## ESP32 Device Flow
1. Master boots -> shows setup wizard to select network mode
2. Slave boots -> creates temporary AP "SlaveSetup-XXXX" with web page for Wi-Fi config
3. Slave connects to network and auto-registers with master as "pending"
4. Admin approves pending devices and enters patient info
5. Approved devices appear on dashboard, alerts work via one-shot system

## API Endpoints
- `GET /api/slaves` - List devices (?approved=1 for dashboard, ?all=1 for admin)
- `GET /api/scan` - Scan nearby Wi-Fi networks
- `GET /api/status` - System status and mode info
- `POST /api/setup` - Set network mode
- `POST /api/connect-wifi` - Connect to hospital Wi-Fi
- `POST /api/register` - Slave auto-registration (creates pending device)
- `POST /api/alert` - Trigger alert (one-shot, only for approved devices)
- `POST /api/approve/:id` - Approve pending device with patient info
- `PUT /api/slaves/:id` - Update device info (admin)
- `DELETE /api/slaves/:id` - Remove device (admin)
- `POST /api/clearAlert/:id` - Clear active alert (admin)

## Key Files
- `shared/schema.ts` - Zod schemas and TypeScript types
- `server/storage.ts` - In-memory storage with CRUD operations
- `server/routes.ts` - Express routes with session auth middleware
- `client/src/pages/dashboard.tsx` - Public monitoring dashboard
- `client/src/pages/admin.tsx` - Admin panel
- `esp32/master/master.ino` - ESP32-S3 master firmware (AP + web server + API)
- `esp32/slave/slave.ino` - ESP32 slave firmware (button + setup web page)
- `esp32/README.md` - Hardware setup and usage guide

## Recent Changes
- 2026-02-21: Major rewrite - added 3 network modes (AP/STA/AP+STA), setup wizard, slave web config page, auto-detection with admin approval flow, redesigned UI with modern effects
- 2026-02-21: Initial MVP build with dashboard, admin panel, REST API, seed data
