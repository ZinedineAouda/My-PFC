# Hospital Patient Alarm System

## Overview
A web-based control panel for a hospital patient alarm system designed for ESP32-S3 IoT devices. The system monitors patient call buttons and displays real-time alerts on a dashboard.

## Architecture
- **Frontend:** React + TypeScript with Vite, Tailwind CSS, shadcn/ui components
- **Backend:** Express.js with session-based authentication
- **Storage:** In-memory (intentional - data resets on restart, matches ESP32 deployment target)
- **Routing:** wouter for client-side routing

## Pages
- `/` - Public dashboard showing patient devices with real-time alert indicators (polls every 2 seconds)
- `/admin` - Password-protected admin panel (username: `admin`, password: `admin1234`)

## API Endpoints
- `GET /api/slaves` - List all devices with alert status
- `POST /api/register` - ESP32 slave device registration
- `POST /api/alert` - ESP32 slave triggers an alert (one-shot: rejected if already active)
- `POST /api/slaves` (admin) - Add new device
- `PUT /api/slaves/:slaveId` (admin) - Update patient info
- `DELETE /api/slaves/:slaveId` (admin) - Remove device
- `POST /api/clearAlert/:slaveId` (admin) - Clear active alert
- `POST /api/admin/login` - Admin login
- `POST /api/admin/logout` - Admin logout
- `GET /api/admin/session` - Check admin session

## Key Files
- `shared/schema.ts` - Zod schemas and TypeScript types
- `server/storage.ts` - In-memory storage with CRUD operations and seed data
- `server/routes.ts` - Express routes with session auth middleware
- `client/src/pages/dashboard.tsx` - Public monitoring dashboard
- `client/src/pages/admin.tsx` - Admin panel with login, CRUD, alert management

## Recent Changes
- 2026-02-21: Initial MVP build with dashboard, admin panel, REST API, seed data
