# Deployment Guide — Split Setup

Deploy the Hospital Patient Alarm System as a **split deployment**:
- **Railway** — Backend API (receives ESP32 data, handles authentication)
- **Vercel** — Frontend dashboard and admin panel (static React app)

---

## Step 1: Push Code to GitHub

Create a GitHub repository and push all the project code. Both Railway and Vercel will deploy from this same repo.

---

## Step 2: Deploy Backend on Railway

Railway runs your API server 24/7 — it receives data from ESP32 devices and serves the REST API.

### Setup

1. Go to [railway.app](https://railway.app) and sign in
2. Click **"New Project"** → **"Deploy from GitHub repo"**
3. Select your repository
4. Railway auto-detects the `Dockerfile` and builds

### Environment Variables

Go to your service → **Variables** tab and add:

| Variable | Value | Description |
|----------|-------|-------------|
| `SESSION_SECRET` | Any random string | Session encryption key |
| `DEVICE_API_KEY` | Any random string | ESP32 authentication key |
| `FRONTEND_URL` | `https://your-app.vercel.app` | Your Vercel frontend URL (for CORS) |

### Get Your Backend URL

1. Go to **Settings** → **Networking** → **Generate Domain**
2. Copy the URL (e.g. `https://your-app.up.railway.app`)
3. You'll need this URL for both Vercel and ESP32 setup

---

## Step 3: Deploy Frontend on Vercel

Vercel serves the React dashboard and admin panel as a fast static site.

### Setup

1. Go to [vercel.com](https://vercel.com) and sign in
2. Click **"Add New"** → **"Project"** → Import your GitHub repo
3. Vercel auto-detects settings from `vercel.json`

### Environment Variables

Go to **Settings** → **Environment Variables** and add:

| Variable | Value | Description |
|----------|-------|-------------|
| `VITE_API_URL` | `https://your-app.up.railway.app` | Your Railway backend URL |

**Important:** The variable must start with `VITE_` to be available in the frontend.

### Deploy

Click **Deploy**. Your frontend will be at `https://your-app.vercel.app`.

---

## Step 4: Connect Railway and Vercel

After both are deployed, you need to link them:

1. **On Railway:** Add `FRONTEND_URL` variable with your Vercel URL
   - Example: `https://your-app.vercel.app`
   - This allows the backend to accept requests from your frontend (CORS)
   - If you have multiple frontends, separate URLs with commas

2. **On Vercel:** Add `VITE_API_URL` variable with your Railway URL
   - Example: `https://your-app.up.railway.app`
   - This tells the frontend where to send API requests
   - **Redeploy** Vercel after adding this variable (the variable is baked into the build)

---

## Step 5: Configure ESP32

Open `esp32/online/master/master.ino` and update:

```cpp
char serverURL[128] = "https://your-app.up.railway.app";  // Railway backend URL
char deviceKey[64] = "your-device-api-key";                // Must match DEVICE_API_KEY on Railway
```

Flash to your ESP32-S3 and it will send data directly to the Railway backend.

---

## How It Works

```
[ESP32 Slaves] → [ESP32 Master] → [Railway Backend API]
                                          ↑
                                   [Vercel Frontend] ← [Browser/Phone]
```

- ESP32 master sends device data to Railway backend via HTTP
- Users open the Vercel frontend in their browser
- Frontend fetches data from Railway backend via API calls
- Railway handles all authentication, device registration, and alerts
- Vercel serves the fast, globally-distributed dashboard

---

## Verifying Deployment

### Test Backend (Railway)

```bash
# Check server status
curl https://YOUR-RAILWAY-URL/api/status

# Test device registration
curl -X POST https://YOUR-RAILWAY-URL/api/register \
  -H "Content-Type: application/json" \
  -H "X-Device-Key: YOUR_KEY" \
  -d '{"slaveId": "test-device"}'
```

### Test Frontend (Vercel)

1. Open `https://YOUR-VERCEL-URL` — Dashboard should load
2. Open `https://YOUR-VERCEL-URL/admin` — Login with admin/admin1234
3. Approve test devices and verify they appear on dashboard

---

## Environment Variables Summary

### Railway (Backend)

| Variable | Required | Description |
|----------|----------|-------------|
| `SESSION_SECRET` | Yes | Random string for session encryption |
| `DEVICE_API_KEY` | Yes | API key for ESP32 authentication |
| `FRONTEND_URL` | Yes | Vercel URL for CORS (e.g. `https://your-app.vercel.app`) |
| `PORT` | No | Server port (Railway sets this automatically) |

### Vercel (Frontend)

| Variable | Required | Description |
|----------|----------|-------------|
| `VITE_API_URL` | Yes | Railway backend URL (e.g. `https://your-app.up.railway.app`) |

---

## Troubleshooting

**Frontend can't reach backend:**
- Check `VITE_API_URL` is set correctly on Vercel (must include `https://`)
- Check `FRONTEND_URL` is set correctly on Railway (must match Vercel URL exactly)
- Redeploy Vercel after changing `VITE_API_URL` (it's a build-time variable)

**ESP32 can't connect:**
- Check `serverURL` in firmware matches your Railway URL
- Check `deviceKey` matches `DEVICE_API_KEY` on Railway
- Ensure ESP32 has internet access (STA or AP+STA mode)

**Admin login doesn't persist:**
- Cross-origin cookies require `secure: true` and `sameSite: none`
- Make sure Railway is serving over HTTPS (it does by default)
- Try in a different browser if cookies are blocked

**CORS errors:**
- Ensure `FRONTEND_URL` on Railway exactly matches the origin (including `https://`, no trailing slash)
- For multiple frontends: `FRONTEND_URL=https://app1.vercel.app,https://app2.vercel.app`
