import { z } from "zod";

export const slaveSchema = z.object({
  slaveId: z.string().min(1),
  patientName: z.string().min(1),
  bed: z.string().min(1),
  room: z.string().min(1),
  alertActive: z.boolean().default(false),
  lastAlertTime: z.string().nullable().default(null),
  registered: z.boolean().default(false),
});

export const insertSlaveSchema = z.object({
  slaveId: z.string().min(1, "Slave ID is required"),
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

export type Slave = z.infer<typeof slaveSchema>;
export type InsertSlave = z.infer<typeof insertSlaveSchema>;
export type UpdateSlave = z.infer<typeof updateSlaveSchema>;
export type LoginData = z.infer<typeof loginSchema>;
