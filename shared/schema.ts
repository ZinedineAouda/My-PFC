import { z } from "zod";
import { pgTable, text, boolean, integer, timestamp, bigint } from "drizzle-orm/pg-core";
import { createInsertSchema, createSelectSchema } from "drizzle-zod";

// ─── Drizzle Database Table ─────────────────────────────────────────
export const devices = pgTable("devices", {
  deviceId: text("device_id").primaryKey(),
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
  controllerLastSeen: bigint("controller_last_seen", { mode: "number" }),
  controllerUptime: integer("controller_uptime"),
  controllerRSSI: integer("controller_rssi"),
  controllerWifiError: text("controller_wifi_error"),
  wifiMode: integer("wifi_mode").notNull().default(1),
  pendingCommand: text("pending_command"),
  commandParams: text("command_params"),
});

// ─── Device Device Schema (Zod) ──────────────────────────────────────
export const deviceSchema = z.object({
  deviceId: z.string().min(1),
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

export const insertDeviceSchema = z.object({
  deviceId: z.string().min(1, "Device ID is required"),
  patientName: z.string().optional().default(""),
  bed: z.string().optional().default(""),
  room: z.string().optional().default(""),
});

export const approveDeviceSchema = z.object({
  patientName: z.string().min(1, "Patient name is required"),
  bed: z.string().min(1, "Bed number is required"),
  room: z.string().min(1, "Room is required"),
});

export const updateDeviceSchema = z.object({
  patientName: z.string().min(1).optional(),
  bed: z.string().min(1).optional(),
  room: z.string().min(1).optional(),
});

export const loginSchema = z.object({
  username: z.string().min(1, "Username is required"),
  password: z.string().min(1, "Password is required"),
});

export const alertRequestSchema = z.object({
  deviceId: z.string().min(1),
});

export const registerRequestSchema = z.object({
  deviceId: z.string().min(1),
});

export const setupSchema = z.object({
  mode: z.number().min(1).max(4),
});

export const connectWifiSchema = z.object({
  ssid: z.string().min(1),
  password: z.string().optional().default(""),
});

// Controller sync: array of device states from ESP32 + controller stats
export const controllerSyncSchema = z.object({
  uptime: z.number().optional(),
  rssi: z.number().optional(),
  mode: z.number().optional(),
  wifiError: z.string().optional(),
  devices: z.array(z.object({
    deviceId: z.string(),
    patientName: z.string().optional().default(""),
    bed: z.string().optional().default(""),
    room: z.string().optional().default(""),
    alertActive: z.boolean().optional().default(false),
    approved: z.boolean().optional().default(false),
    online: z.boolean().optional().default(false),
    lastUpdatedAt: z.number().optional(),
  })),
});

export type Device = z.infer<typeof deviceSchema>;
export type InsertDevice = z.infer<typeof insertDeviceSchema>;
export type ApproveDevice = z.infer<typeof approveDeviceSchema>;
export type UpdateDevice = z.infer<typeof updateDeviceSchema>;
export type LoginData = z.infer<typeof loginSchema>;
export type ControllerSync = z.infer<typeof controllerSyncSchema>;
