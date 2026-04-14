import { Pool } from 'pg';
import { drizzle } from 'drizzle-orm/node-postgres';
import * as schema from "@shared/schema";

const databaseUrl = process.env.DATABASE_URL;

if (!databaseUrl && process.env.NODE_ENV === "production") {
  console.warn("[CRITICAL] DATABASE_URL is missing! Server will start but database features will be disabled. Please link a PostgreSQL database in Railway.");
}

// Use a placeholder for local development if DB is not available
const connectionString = databaseUrl || "postgres://localhost:5432/postgres";

export const pool = new Pool({ 
  connectionString,
  ssl: process.env.NODE_ENV === "production" ? { rejectUnauthorized: false } : false,
  max: 20,
  idleTimeoutMillis: 30000,
  connectionTimeoutMillis: 5000, // Increase slightly for slower cloud startups
});

// Event listener to prevent pool-level unhandled errors from crashing the app
pool.on('error', (err) => {
  console.error('[DB] Unexpected error on idle client:', err);
});

export const db = drizzle(pool, { schema });
