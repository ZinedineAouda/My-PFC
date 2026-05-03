import type { Device, InsertDevice, UpdateDevice, ApproveDevice } from "@shared/schema";
import { devices, systemSettings } from "@shared/schema";
import { db } from "./db";
import { eq, sql, not, inArray } from "drizzle-orm";

export interface IStorage {
  getAllDevices(): Promise<Device[]>;
  getApprovedDevices(): Promise<Device[]>;
  getDevice(deviceId: string): Promise<Device | undefined>;
  addDevice(data: InsertDevice): Promise<Device>;
  approveDevice(deviceId: string, data: ApproveDevice): Promise<Device | undefined>;
  updateDevice(deviceId: string, data: UpdateDevice): Promise<Device | undefined>;
  deleteDevice(deviceId: string): Promise<boolean>;
  registerDevice(deviceId: string): Promise<Device>;
  triggerAlert(deviceId: string): Promise<{ success: boolean; device?: Device; reason?: string }>;
  clearAlert(deviceId: string): Promise<boolean>;
  getMode(): Promise<number>;
  setMode(mode: number): Promise<void>;
  isSetupDone(): Promise<boolean>;
  updateControllerHeartbeat(mode?: number, uptime?: number, rssi?: number, wifiError?: string): Promise<void>;
  isControllerOnline(): Promise<boolean>;
  syncFromController(devices: any[], stats?: { mode?: number, uptime?: number, rssi?: number, wifiError?: string }): Promise<void>;
  queueCommand(command: string, params?: string): Promise<void>;
  getAndClearCommand(): Promise<{ command: string; params: string } | null>;
  getSettings(): Promise<any>;
  reset(): Promise<void>;
}

export class DatabaseStorage implements IStorage {
  // ─── Internal Utility: Upsert Singleton Settings ───────────────────
  public async getSettings() {
    const [row] = await db.select().from(systemSettings).where(eq(systemSettings.id, 1));
    if (!row) {
      // Create default row if missing
      const [newRow] = await db.insert(systemSettings).values({ id: 1 }).returning();
      return newRow;
    }
    return row;
  }

  async getAllDevices(): Promise<Device[]> {
    return await db.select().from(devices);
  }

  async getApprovedDevices(): Promise<Device[]> {
    return await db.select().from(devices).where(eq(devices.approved, true));
  }

  async getDevice(deviceId: string): Promise<Device | undefined> {
    const [device] = await db.select().from(devices).where(eq(devices.deviceId, deviceId));
    return device;
  }

  async addDevice(data: InsertDevice): Promise<Device> {
    const [device] = await db.insert(devices).values({
      ...data,
      patientName: data.patientName || "",
      bed: data.bed || "",
      room: data.room || "",
      alertActive: false,
      registered: false,
      approved: false,
      online: false,
      lastUpdatedAt: Date.now(),
    }).returning();
    return device;
  }

  async approveDevice(deviceId: string, data: ApproveDevice): Promise<Device | undefined> {
    const [device] = await db.update(devices)
      .set({ ...data, approved: true, lastUpdatedAt: Date.now() })
      .where(eq(devices.deviceId, deviceId))
      .returning();
    return device;
  }

  async updateDevice(deviceId: string, data: UpdateDevice): Promise<Device | undefined> {
    const [device] = await db.update(devices)
      .set({ ...data, lastUpdatedAt: Date.now() })
      .where(eq(devices.deviceId, deviceId))
      .returning();
    return device;
  }

  async deleteDevice(deviceId: string): Promise<boolean> {
    const [existing] = await db.select().from(devices).where(eq(devices.deviceId, deviceId));
    if (!existing) return false;

    // First delete from DB
    await db.delete(devices).where(eq(devices.deviceId, deviceId));
    
    // Then queue command for Controller to delete locally
    // This completes the bidirectional sync: Cloud -> Local propagation
    await this.queueCommand("REMOVE_DEVICE", deviceId);
    
    return true;
  }

  async registerDevice(deviceId: string): Promise<Device> {
    const [existing] = await db.select().from(devices).where(eq(devices.deviceId, deviceId));
    if (existing) {
      const [updated] = await db.update(devices)
        .set({ registered: true, online: true, lastSeen: Date.now() })
        .where(eq(devices.deviceId, deviceId))
        .returning();
      return updated;
    }
    const [inserted] = await db.insert(devices).values({
      deviceId,
      registered: true,
      online: true,
      lastSeen: Date.now(),
      patientName: "",
      bed: "",
      room: "",
      alertActive: false,
      approved: false,
      lastUpdatedAt: Date.now(),
    }).returning();
    return inserted;
  }

  async triggerAlert(deviceId: string): Promise<{ success: boolean; device?: Device; reason?: string }> {
    const device = await this.getDevice(deviceId);
    if (!device) return { success: false, reason: "unknown device" };
    if (!device.approved) return { success: false, reason: "not approved" };
    if (device.alertActive) return { success: false, device, reason: "alert already active" };

    const [updated] = await db.update(devices)
      .set({ 
        alertActive: true, 
        lastAlertTime: new Date().toISOString(),
        lastSeen: Date.now(),
        online: true,
        lastUpdatedAt: Date.now()
      })
      .where(eq(devices.deviceId, deviceId))
      .returning();
    return { success: true, device: updated };
  }

  async clearAlert(deviceId: string): Promise<boolean> {
    const [updated] = await db.update(devices)
      .set({ alertActive: false, lastUpdatedAt: Date.now() })
      .where(eq(devices.deviceId, deviceId))
      .returning();
    
    if (updated) {
      // Critical: Queue a command so the Controller knows to turn off the physical Device LED
      await this.queueCommand("clear_alert", deviceId);
    }
    return !!updated;
  }

  async getMode(): Promise<number> {
    const settings = await this.getSettings();
    return settings.wifiMode;
  }

  async setMode(mode: number): Promise<void> {
    await this.getSettings(); // ensure row exists
    await db.update(systemSettings)
      .set({ wifiMode: mode, pendingCommand: "CHANGE_MODE", commandParams: mode.toString() })
      .where(eq(systemSettings.id, 1));
  }

  async isSetupDone(): Promise<boolean> {
    const mode = await this.getMode();
    return mode !== 0;
  }

  async updateControllerHeartbeat(mode?: number, uptime?: number, rssi?: number, wifiError?: string): Promise<void> {
    const now = Date.now();
    const update: any = { controllerLastSeen: now };
    if (mode !== undefined) update.wifiMode = mode;
    if (uptime !== undefined) update.controllerUptime = uptime;
    if (rssi !== undefined) update.controllerRSSI = rssi;
    if (wifiError !== undefined) update.controllerWifiError = wifiError;

    try {
      await db.insert(systemSettings)
        .values({ 
          id: 1, 
          controllerLastSeen: now,
          wifiMode: mode !== undefined ? mode : 1,
          controllerUptime: uptime,
          controllerRSSI: rssi,
          controllerWifiError: wifiError
        })
        .onConflictDoUpdate({
          target: systemSettings.id,
          set: update
        });
    } catch (err) {
      console.warn("[DB] Failed to update Controller heartbeat (schema mismatch?):", err);
      // Fallback: minimal update of just the 'last seen' timestamp if possible
      try {
        await db.update(systemSettings)
          .set({ controllerLastSeen: now })
          .where(eq(systemSettings.id, 1));
      } catch (e) {
        console.error("[DB] Critical: Heartbeat fallback failed:", e);
      }
    }
  }

  async isControllerOnline(): Promise<boolean> {
    const settings = await this.getSettings();
    const lastSeen = settings.controllerLastSeen;
    if (!lastSeen) return false;
    
    // 12 second timeout for "Ultra-Live" feel (ESP32 pings every 5s)
    const lastSeenNum = typeof lastSeen === 'string' ? parseInt(lastSeen) : Number(lastSeen);
    return Date.now() - lastSeenNum < 12000;
  }

  async syncFromController(incoming: Array<any>, stats?: { mode?: number, uptime?: number, rssi?: number, wifiError?: string }): Promise<void> {
    await this.updateControllerHeartbeat(stats?.mode, stats?.uptime, stats?.rssi, stats?.wifiError);
    
    console.log(`[SYNC] Starting bidir sync for ${incoming.length} devices`);
    
    // Track incoming IDs to prune deleted nodes
    const incomingIds = incoming.map(s => s.deviceId);

    // First, set all current devices to offline (temporary state)
    try {
      await db.update(devices).set({ online: false });
    } catch (err) {
      console.error("[SYNC] Failed to set devices offline:", err);
    }

    for (const remote of incoming) {
      const [existing] = await db.select().from(devices).where(eq(devices.deviceId, remote.deviceId));
      
      if (existing) {
        const tRemote = remote.lastUpdatedAt || 0;
        const tLocal = existing.lastUpdatedAt || 0;

        if (tRemote >= tLocal) {
          // Controller wins (or timestamps equal = trust controller as authoritative source)
          await db.update(devices)
            .set({
              online: true,
              alertActive: remote.alertActive !== undefined ? remote.alertActive : existing.alertActive,
              approved: remote.approved !== undefined ? remote.approved : existing.approved,
              patientName: remote.patientName || existing.patientName,
              bed: remote.bed || existing.bed,
              room: remote.room || existing.room,
              lastSeen: Date.now(),
              lastUpdatedAt: tRemote > 0 ? tRemote : tLocal
            })
            .where(eq(devices.deviceId, remote.deviceId));
        } else {
          // Local DB has a newer manual change (e.g. admin just clicked 'clear')
          await db.update(devices)
            .set({
              online: true,
              lastSeen: Date.now()
            })
            .where(eq(devices.deviceId, remote.deviceId));
        }
      } else {
        await db.insert(devices).values({
          deviceId: remote.deviceId,
          patientName: remote.patientName || "",
          bed: remote.bed || "",
          room: remote.room || "",
          alertActive: remote.alertActive || false,
          approved: remote.approved || false,
          online: true,
          registered: true,
          lastSeen: Date.now(),
          lastUpdatedAt: remote.lastUpdatedAt || Date.now()
        });
      }
    }

    // PRUNING: Delete devices that are NOT in the incoming list
    if (incomingIds.length > 0) {
      const deletedCount = await db.delete(devices).where(not(inArray(devices.deviceId, incomingIds))).returning();
      if (deletedCount.length > 0) {
        console.log(`[SYNC] Pruned ${deletedCount.length} ghost devices from cloud DB`);
      }
    } else {
      // If incoming list is empty (e.g. Wipe Data just happened), clear all devices
      const deletedCount = await db.delete(devices).returning();
      if (deletedCount.length > 0) {
        console.log(`[SYNC] Cleared all ${deletedCount.length} devices (empty sync bundle)`);
      }
    }
  }

  async queueCommand(command: string, params: string = ""): Promise<void> {
    await this.getSettings(); // ensure exists
    await db.update(systemSettings)
      .set({ pendingCommand: command, commandParams: params })
      .where(eq(systemSettings.id, 1));
    console.log(`[STORAGE] Queued command for Controller: ${command} (${params})`);
  }

  async getAndClearCommand(): Promise<{ command: string; params: string } | null> {
    const settings = await this.getSettings();
    if (!settings.pendingCommand) return null;

    const cmd = { 
      command: settings.pendingCommand, 
      params: settings.commandParams || "" 
    };

    // Clear command immediately after fetching
    await db.update(systemSettings)
      .set({ pendingCommand: null, commandParams: null })
      .where(eq(systemSettings.id, 1));

    return cmd;
  }

  async reset(): Promise<void> {
    await db.delete(devices);
    await db.update(systemSettings)
      .set({ wifiMode: 0, controllerLastSeen: null, pendingCommand: null, commandParams: null })
      .where(eq(systemSettings.id, 1));
  }
}

export const storage = new DatabaseStorage();
