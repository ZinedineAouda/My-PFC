import { Pool } from 'pg';
import { drizzle } from 'drizzle-orm/node-postgres';
import * as schema from "@shared/schema";

const databaseUrl = process.env.DATABASE_URL;

if (!databaseUrl && process.env.NODE_ENV === "production") {
  console.warn("[CRITICAL] DATABASE_URL is missing! Server will start but database features will be disabled. Please link a PostgreSQL database in Railway.");
}

// Use a placeholder for local development if DB is not available
const connectionString = databaseUrl || "postgres://localhost:5432/postgres";

const isProduction = process.env.NODE_ENV === "production";
const isInternal = connectionString.includes(".railway.internal") || connectionString.includes("localhost") || connectionString.includes("127.0.0.1");

export const pool = new Pool({ 
  connectionString,
  ssl: isProduction && !isInternal ? { rejectUnauthorized: false } : false,
  max: 20,
  idleTimeoutMillis: 30000,
  connectionTimeoutMillis: 15000, // Increased to 15s to prevent cloud cold-start connection timeouts
});

// Event listener to prevent pool-level unhandled errors from crashing the app
pool.on('error', (err) => {
  console.error('[DB] Unexpected error on idle client:', err);
});

export const db = drizzle(pool, { schema });
