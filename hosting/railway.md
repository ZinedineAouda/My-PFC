# Deploy on Railway

Railway hosts your full-stack app — the Express backend AND the React frontend — all from one URL. The ESP32 sends data to the backend, and your browser accesses the dashboard from the same origin.

---

## Prerequisites

- A [GitHub](https://github.com) account
- Your project pushed to a GitHub repository
- A [Railway](https://railway.app) account (free tier works)

---

## Step 1 — Push Code to GitHub

If you haven't already:

```bash
git init
git add .
git commit -m "Initial commit"
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO.git
git push -u origin main
```

---

## Step 2 — Create a Railway Project

1. Go to [railway.app](https://railway.app) and sign in.
2. Click **"New Project"**.
3. Select **"Deploy from GitHub repo"**.
4. Choose your repository from the list.
5. Railway will detect the `Dockerfile` and start building automatically.

> **What gets deployed?** The Dockerfile builds both the React frontend and Express backend into a single image. Express serves the frontend as static files, so everything is available at one URL.

---

## Step 3 — Set Environment Variables

In your Railway project, go to your service → **Variables** tab, and add:

| Variable | Value | Notes |
|---|---|---|
| `SESSION_SECRET` | Any long random string | e.g. `openssl rand -hex 32` |
| `DEVICE_API_KEY` | Any random string | Must match what you put in the ESP32 firmware |

> **No `FRONTEND_URL` needed.** Since the frontend and backend are served from the same Railway URL, there is no cross-origin request — CORS is not required.

---

## Step 4 — Get Your Railway URL

1. In your Railway project, click your service.
2. Go to **Settings** → **Networking**.
3. Click **"Generate Domain"**.
4. Copy the URL — it will look like `https://your-app.up.railway.app`.
5. **Save this URL** — you'll need it for the ESP32 configuration.

---

## Step 5 — Verify the Deployment

Once deployed, test with:

```bash
# Check server is running
curl https://YOUR-RAILWAY-URL/api/status

# Expected response:
# {"mode":0,"setup":false,"masterIP":"localhost","slaves":0}
```

If you get a JSON response, the backend is working.

Then open your Railway URL in a browser — the dashboard should load.

---

## Step 6 — Test Device Registration

```bash
curl -X POST https://YOUR-RAILWAY-URL/api/register \
  -H "Content-Type: application/json" \
  -H "X-Device-Key: YOUR_DEVICE_API_KEY" \
  -d '{"slaveId": "test-device"}'

# Expected: {"success":true,"message":"Pending approval"}
```

---

## Next Step

→ [Configure ESP32 for Online Mode](./esp32-online-setup.md)
