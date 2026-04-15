import { z } from "zod";
import { pgTable, text, boolean, integer, timestamp, bigint } from "drizzle-orm/pg-core";
import { createInsertSchema, createSelectSchema } from "drizzle-zod";

// ─── Drizzle Database Table ─────────────────────────────────────────
export const slaves = pgTable("slaves", {
  slaveId: text("slave_id").primaryKey(),
  patientName: text("patient_name").notNull().default(""),
  bed: text("bed").notNull().default(""),
  room: text("room").notNull().default(""),
  alertActive: boolean("alert_active").notNull().default(false),
  lastAlertTime: text("last_alert_time"),
  registered: boolean("registered").notNull().default(false),
  approved: boolean("approved").notNull().default(false),
  online: boolean("online").notNull().default(false),
  lastSeen: bigint("last_seen", { mode: "number" }),
  lastUpdatedAt: bigint("last_updated_at", { mode: "number" }),
});

// ─── System Settings Table (Global Persistence) ─────────────────────
export const systemSettings = pgTable("system_settings", {
  id: integer("id").primaryKey().default(1),
  masterLastSeen: bigint("master_last_seen", { mode: "number" }),
  masterUptime: integer("master_uptime"),
  masterRSSI: integer("master_rssi"),
  wifiMode: integer("wifi_mode").notNull().default(1),
  pendingCommand: text("pending_command"),
  commandParams: text("command_params"),
});

// ─── Slave Device Schema (Zod) ──────────────────────────────────────
export const slaveSchema = z.object({
  slaveId: z.string().min(1),
  patientName: z.string().default(""),
  bed: z.string().default(""),
  room: z.string().default(""),
  alertActive: z.boolean().default(false),
  lastAlertTime: z.string().nullable().default(null),
  registered: z.boolean().default(false),
  approved: z.boolean().default(false),
  online: z.boolean().default(false),
  lastSeen: z.number().nullable().default(null),
  lastUpdatedAt: z.number().nullable().default(null),
});

export const insertSlaveSchema = z.object({
  slaveId: z.string().min(1, "Slave ID is required"),
  patientName: z.string().optional().default(""),
  bed: z.string().optional().default(""),
  room: z.string().optional().default(""),
});

export const approveSlaveSchema = z.object({
  patientName: z.string().min(1, "Patient name is required"),
  bed: z.string().min(1, "Bed number is required"),
  room: z.string().min(1, "Room is required"),
});

export const updateSlaveSchema = z.object({
  patientName: z.string().min(1).optional(),
  bed: z.string().min(1).optional(),
  room: z.string().min(1).optional(),
});

export const loginSchema = z.object({
  username: z.string().min(1, "Username is required"),
  password: z.string().min(1, "Password is required"),
});

export const alertRequestSchema = z.object({
  slaveId: z.string().min(1),
});

export const registerRequestSchema = z.object({
  slaveId: z.string().min(1),
});

export const setupSchema = z.object({
  mode: z.number().min(1).max(4),
});

export const connectWifiSchema = z.object({
  ssid: z.string().min(1),
  password: z.string().optional().default(""),
});

// Master sync: array of slave states from ESP32 + master stats
export const masterSyncSchema = z.object({
  uptime: z.number().optional(),
  rssi: z.number().optional(),
  mode: z.number().optional(),
  slaves: z.array(z.object({
    slaveId: z.string(),
    patientName: z.string().optional().default(""),
    bed: z.string().optional().default(""),
    room: z.string().optional().default(""),
    alertActive: z.boolean().optional().default(false),
    approved: z.boolean().optional().default(false),
    online: z.boolean().optional().default(false),
    lastUpdatedAt: z.number().optional(),
  })),
});

export type Slave = z.infer<typeof slaveSchema>;
export type InsertSlave = z.infer<typeof insertSlaveSchema>;
export type ApproveSlave = z.infer<typeof approveSlaveSchema>;
export type UpdateSlave = z.infer<typeof updateSlaveSchema>;
export type LoginData = z.infer<typeof loginSchema>;
export type MasterSync = z.infer<typeof masterSyncSchema>;
