# Deploy Frontend on Vercel

Vercel serves the React dashboard as a fast static site. It connects to your Railway backend for all data.

---

## Prerequisites

- Railway backend deployed and URL ready (see [railway.md](./railway.md))
- A [Vercel](https://vercel.com) account (free tier works)

---

## Step 1 — Import Your Repository

1. Go to [vercel.com](https://vercel.com) and sign in.
2. Click **"Add New"** → **"Project"**.
3. Click **"Import Git Repository"** and select your GitHub repo.
4. Vercel auto-detects settings from `vercel.json` — **do not change anything**.

---

## Step 2 — Set Environment Variables

Before clicking Deploy, go to **Environment Variables** and add:

| Variable | Value | Notes |
|---|---|---|
| `VITE_API_URL` | `https://YOUR-RAILWAY-URL.up.railway.app` | Must start with `https://`, no trailing slash |

> **Important:** This variable must start with `VITE_` to be exposed to the React frontend during build time.

---

## Step 3 — Deploy

Click **"Deploy"**. Vercel will build and publish the site.

Your frontend will be live at: `https://your-app.vercel.app`

---

## Step 4 — Link Railway and Vercel (CORS Fix)

After deploying on both platforms, you need to link them together:

**On Railway**, go to your service → Variables, and set:

```
FRONTEND_URL = https://your-app.vercel.app
```

Then **redeploy** on Railway (Railway will pick up the new env var on next deploy — you can trigger this by going to **Deployments** → **Redeploy**).

> **Why?** The backend uses `FRONTEND_URL` to allow CORS requests from your frontend. Without this, the browser will block all API calls.

---

## Step 5 — Verify Frontend

1. Open `https://your-app.vercel.app` → the dashboard should load (it will show 0 devices initially).
2. Open `https://your-app.vercel.app/admin` → login with:
   - **Username:** `admin`
   - **Password:** `admin1234`
3. If the admin panel loads, everything is working.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Dashboard shows no data | Check `VITE_API_URL` is set correctly. Redeploy if you changed it. |
| CORS errors in browser console | Check `FRONTEND_URL` on Railway matches your Vercel URL exactly (no trailing slash) |
| Admin login fails | Check Railway backend is running: `curl https://YOUR-RAILWAY-URL/api/status` |
| `VITE_API_URL` not working | Make sure variable name starts with `VITE_`, not just `API_URL` |

---

## Next Step

→ [Configure ESP32 for Online Mode](./esp32-online-setup.md)
