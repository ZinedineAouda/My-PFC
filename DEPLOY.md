# Deployment Guide

Deploy the Hospital Patient Alarm System web server to Railway or Vercel for the **online mode** (ESP32 forwards data to your hosted server).

## Environment Variables

Set these on whichever platform you use:

| Variable | Required | Description |
|----------|----------|-------------|
| `SESSION_SECRET` | Yes | Random string for session encryption (e.g. `openssl rand -hex 32`) |
| `DEVICE_API_KEY` | Yes | API key your ESP32 will use to authenticate (any random string) |
| `PORT` | No | Server port (Railway sets this automatically, default 5000) |

---

## Option 1: Railway (Recommended)

Railway runs your app as a persistent server — perfect for this app since it uses in-memory storage and sessions.

### Steps

1. **Push code to GitHub**
   - Create a GitHub repo and push this project's code
   - Make sure to include all files except `esp32/` (optional, not needed for the web server)

2. **Create a Railway project**
   - Go to [railway.app](https://railway.app) and sign in
   - Click "New Project" → "Deploy from GitHub repo"
   - Select your repository

3. **Set environment variables**
   - Go to your service → Variables tab
   - Add `SESSION_SECRET` (any random string)
   - Add `DEVICE_API_KEY` (the key you'll put in your ESP32 firmware)

4. **Deploy**
   - Railway will automatically detect the `Dockerfile` and build
   - It will also detect `railway.toml` for health check settings
   - Your app will be available at a `*.up.railway.app` URL

5. **Get your URL**
   - Go to Settings → Networking → Generate Domain
   - Copy the URL (e.g. `https://your-app.up.railway.app`)

6. **Update ESP32 firmware**
   - Open `esp32/online/master/master.ino`
   - Set `serverURL` to your Railway URL
   - Set `deviceKey` to match your `DEVICE_API_KEY`
   - Flash to your ESP32

### Railway Notes
- Free tier available with limited hours
- App stays running 24/7 on paid plans
- Data persists as long as the server is running (resets on redeploy)

---

## Option 2: Vercel

Vercel deploys the frontend as static files and the API as serverless functions.

### Important Limitation
Vercel is serverless — each API request may run on a different instance. This means:
- In-memory storage resets on cold starts (after ~5-15 minutes of inactivity)
- Sessions may not persist between requests
- **For a production hospital system, Railway is recommended over Vercel**

### Steps

1. **Push code to GitHub**
   - Create a GitHub repo and push this project's code

2. **Create a Vercel project**
   - Go to [vercel.com](https://vercel.com) and sign in
   - Click "Add New" → "Project" → Import your GitHub repo

3. **Configure build settings**
   - Vercel auto-detects settings from `vercel.json`
   - The API runs as a serverless function (all `/api/*` requests route to a single Express handler)
   - The frontend is served as static files with SPA fallback

4. **Set environment variables**
   - Go to Settings → Environment Variables
   - Add `SESSION_SECRET` and `DEVICE_API_KEY`

5. **Deploy**
   - Click Deploy
   - Your app will be at `*.vercel.app`

6. **Update ESP32 firmware**
   - Same as Railway — update `serverURL` and `deviceKey` in the online master firmware

---

## Build Commands Reference

| Command | Use |
|---------|-----|
| `npx tsx script/build-external.ts` | Build for external platforms (Railway/Vercel) |
| `npm run build` | Build for Replit (uses Replit-specific plugins) |
| `npm start` | Start production server (after build) |

The external build script (`script/build-external.ts`) uses `vite.config.external.ts` which excludes Replit-specific plugins that aren't available on other platforms.

---

## Verifying Deployment

After deploying, test that everything works:

```bash
# Check server is running
curl https://YOUR-APP-URL/api/status

# Test device registration (replace YOUR_KEY with your DEVICE_API_KEY)
curl -X POST https://YOUR-APP-URL/api/register \
  -H "Content-Type: application/json" \
  -H "X-Device-Key: YOUR_KEY" \
  -d '{"slaveId": "test-device"}'

# Open dashboard in browser
open https://YOUR-APP-URL

# Open admin panel (login: admin / admin1234)
open https://YOUR-APP-URL/admin
```

---

## File Structure (What Gets Deployed)

```
server/          ← Backend Express API
client/          ← Frontend React app
shared/          ← Shared types and schemas
dist/            ← Built output (created by build command)
  ├── public/    ← Built frontend static files
  └── index.cjs  ← Built backend server bundle
api/             ← Vercel serverless function wrapper
Dockerfile       ← Railway Docker build config
railway.toml     ← Railway deployment settings
vercel.json      ← Vercel deployment settings
```

The `esp32/` folder contains firmware for your physical devices and is not part of the web deployment.
