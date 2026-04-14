import { Pool } from 'pg';
import { drizzle } from 'drizzle-orm/node-postgres';
import * as schema from "@shared/schema";

if (!process.env.DATABASE_URL) {
  // If no DATABASE_URL, the app will fall back to MemStorage in storage.ts
  // but we export a dummy db to keep types happy if needed.
}

export const pool = new Pool({ 
  connectionString: process.env.DATABASE_URL,
  ssl: process.env.NODE_ENV === "production" ? { rejectUnauthorized: false } : false
});

export const db = drizzle(pool, { schema });
