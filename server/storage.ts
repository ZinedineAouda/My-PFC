import type { Slave, InsertSlave, UpdateSlave, ApproveSlave } from "@shared/schema";
import { slaves, systemSettings } from "@shared/schema";
import { db } from "./db";
import { eq, sql } from "drizzle-orm";

export interface IStorage {
  getAllSlaves(): Promise<Slave[]>;
  getApprovedSlaves(): Promise<Slave[]>;
  getSlave(slaveId: string): Promise<Slave | undefined>;
  addSlave(data: InsertSlave): Promise<Slave>;
  approveSlave(slaveId: string, data: ApproveSlave): Promise<Slave | undefined>;
  updateSlave(slaveId: string, data: UpdateSlave): Promise<Slave | undefined>;
  deleteSlave(slaveId: string): Promise<boolean>;
  registerSlave(slaveId: string): Promise<Slave>;
  triggerAlert(slaveId: string): Promise<{ success: boolean; slave?: Slave; reason?: string }>;
  clearAlert(slaveId: string): Promise<boolean>;
  getMode(): Promise<number>;
  setMode(mode: number): Promise<void>;
  isSetupDone(): Promise<boolean>;
  updateMasterHeartbeat(mode?: number, uptime?: number, rssi?: number, wifiError?: string): Promise<void>;
  isMasterOnline(): Promise<boolean>;
  syncFromMaster(slaves: any[], stats?: { mode?: number, uptime?: number, rssi?: number, wifiError?: string }): Promise<void>;
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

  async getAllSlaves(): Promise<Slave[]> {
    return await db.select().from(slaves);
  }

  async getApprovedSlaves(): Promise<Slave[]> {
    return await db.select().from(slaves).where(eq(slaves.approved, true));
  }

  async getSlave(slaveId: string): Promise<Slave | undefined> {
    const [slave] = await db.select().from(slaves).where(eq(slaves.slaveId, slaveId));
    return slave;
  }

  async addSlave(data: InsertSlave): Promise<Slave> {
    const [slave] = await db.insert(slaves).values({
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
    return slave;
  }

  async approveSlave(slaveId: string, data: ApproveSlave): Promise<Slave | undefined> {
    const [slave] = await db.update(slaves)
      .set({ ...data, approved: true, lastUpdatedAt: Date.now() })
      .where(eq(slaves.slaveId, slaveId))
      .returning();
    return slave;
  }

  async updateSlave(slaveId: string, data: UpdateSlave): Promise<Slave | undefined> {
    const [slave] = await db.update(slaves)
      .set({ ...data, lastUpdatedAt: Date.now() })
      .where(eq(slaves.slaveId, slaveId))
      .returning();
    return slave;
  }

  async deleteSlave(slaveId: string): Promise<boolean> {
    await this.queueCommand("REMOVE_SLAVE", slaveId);
    const result = await db.delete(slaves).where(eq(slaves.slaveId, slaveId)).returning();
    return result.length > 0;
  }

  async registerSlave(slaveId: string): Promise<Slave> {
    const [existing] = await db.select().from(slaves).where(eq(slaves.slaveId, slaveId));
    if (existing) {
      const [updated] = await db.update(slaves)
        .set({ registered: true, online: true, lastSeen: Date.now() })
        .where(eq(slaves.slaveId, slaveId))
        .returning();
      return updated;
    }
    const [inserted] = await db.insert(slaves).values({
      slaveId,
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

  async triggerAlert(slaveId: string): Promise<{ success: boolean; slave?: Slave; reason?: string }> {
    const slave = await this.getSlave(slaveId);
    if (!slave) return { success: false, reason: "unknown slave" };
    if (!slave.approved) return { success: false, reason: "not approved" };
    if (slave.alertActive) return { success: false, slave, reason: "alert already active" };

    const [updated] = await db.update(slaves)
      .set({ 
        alertActive: true, 
        lastAlertTime: new Date().toISOString(),
        lastSeen: Date.now(),
        online: true,
        lastUpdatedAt: Date.now()
      })
      .where(eq(slaves.slaveId, slaveId))
      .returning();
    return { success: true, slave: updated };
  }

  async clearAlert(slaveId: string): Promise<boolean> {
    const [updated] = await db.update(slaves)
      .set({ alertActive: false, lastUpdatedAt: Date.now() })
      .where(eq(slaves.slaveId, slaveId))
      .returning();
    
    if (updated) {
      // Critical: Queue a command so the Master knows to turn off the physical Slave LED
      await this.queueCommand("clear_alert", slaveId);
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

  async updateMasterHeartbeat(mode?: number, uptime?: number, rssi?: number, wifiError?: string): Promise<void> {
    const now = Date.now();
    const update: any = { masterLastSeen: now };
    if (mode !== undefined) update.wifiMode = mode;
    if (uptime !== undefined) update.masterUptime = uptime;
    if (rssi !== undefined) update.masterRSSI = rssi;
    if (wifiError !== undefined) update.masterWifiError = wifiError;

    try {
      await db.insert(systemSettings)
        .values({ 
          id: 1, 
          masterLastSeen: now,
          wifiMode: mode !== undefined ? mode : 1,
          masterUptime: uptime,
          masterRSSI: rssi,
          masterWifiError: wifiError
        })
        .onConflictDoUpdate({
          target: systemSettings.id,
          set: update
        });
    } catch (err) {
      console.warn("[DB] Failed to update Master heartbeat (schema mismatch?):", err);
      // Fallback: minimal update of just the 'last seen' timestamp if possible
      try {
        await db.update(systemSettings)
          .set({ masterLastSeen: now })
          .where(eq(systemSettings.id, 1));
      } catch (e) {
        console.error("[DB] Critical: Heartbeat fallback failed:", e);
      }
    }
  }

  async isMasterOnline(): Promise<boolean> {
    const settings = await this.getSettings();
    const lastSeen = settings.masterLastSeen;
    if (!lastSeen) return false;
    
    // 12 second timeout for "Ultra-Live" feel (ESP32 pings every 5s)
    const lastSeenNum = typeof lastSeen === 'string' ? parseInt(lastSeen) : Number(lastSeen);
    return Date.now() - lastSeenNum < 12000;
  }

  async syncFromMaster(incoming: Array<any>, stats?: { mode?: number, uptime?: number, rssi?: number, wifiError?: string }): Promise<void> {
    await this.updateMasterHeartbeat(stats?.mode, stats?.uptime, stats?.rssi, stats?.wifiError);
    
    console.log(`[SYNC] Starting bidir sync for ${incoming.length} slaves`);
    
    // First, set all current slaves to offline
    try {
      await db.update(slaves).set({ online: false });
    } catch (err) {
      console.error("[SYNC] Failed to set slaves offline:", err);
    }

    for (const remote of incoming) {
      const [existing] = await db.select().from(slaves).where(eq(slaves.slaveId, remote.slaveId));
      
      if (existing) {
        const tRemote = remote.lastUpdatedAt || 0;
        const tLocal = existing.lastUpdatedAt || 0;

        if (tRemote >= tLocal) {
          // Master wins (or timestamps equal = trust master as authoritative source)
          await db.update(slaves)
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
            .where(eq(slaves.slaveId, remote.slaveId));
        } else {
          // Local DB has a newer manual change (e.g. admin just clicked 'clear')
          await db.update(slaves)
            .set({
              online: true,
              lastSeen: Date.now()
            })
            .where(eq(slaves.slaveId, remote.slaveId));
        }
      } else {
        await db.insert(slaves).values({
          slaveId: remote.slaveId,
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
  }

  async queueCommand(command: string, params: string = ""): Promise<void> {
    await this.getSettings(); // ensure exists
    await db.update(systemSettings)
      .set({ pendingCommand: command, commandParams: params })
      .where(eq(systemSettings.id, 1));
    console.log(`[STORAGE] Queued command for Master: ${command} (${params})`);
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
    await db.delete(slaves);
    await db.update(systemSettings)
      .set({ wifiMode: 0, masterLastSeen: null, pendingCommand: null, commandParams: null })
      .where(eq(systemSettings.id, 1));
  }
}

export const storage = new DatabaseStorage();
