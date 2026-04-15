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
  updateMasterHeartbeat(): Promise<void>;
  isMasterOnline(): Promise<boolean>;
  syncFromMaster(slaves: Array<{
    slaveId: string;
    patientName?: string;
    bed?: string;
    room?: string;
    alertActive?: boolean;
    approved?: boolean;
    online?: boolean;
  }>): Promise<void>;
  queueCommand(command: string, params?: string): Promise<void>;
  getAndClearCommand(): Promise<{ command: string; params: string } | null>;
  reset(): Promise<void>;
}

export class DatabaseStorage implements IStorage {
  // ─── Internal Utility: Upsert Singleton Settings ───────────────────
  private async getSettings() {
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

  async updateMasterHeartbeat(): Promise<void> {
    const now = Date.now();
    await db.insert(systemSettings)
      .values({ id: 1, masterLastSeen: now })
      .onConflictDoUpdate({
        target: systemSettings.id,
        set: { masterLastSeen: now }
      });
  }

  async isMasterOnline(): Promise<boolean> {
    const settings = await this.getSettings();
    const lastSeen = settings.masterLastSeen;
    if (!lastSeen) return false;
    
    // Ensure we handle both number and possible string/bigint types from the driver
    const lastSeenNum = typeof lastSeen === 'string' ? parseInt(lastSeen) : Number(lastSeen);
    return Date.now() - lastSeenNum < 60000;
  }

  async syncFromMaster(incoming: Array<any>): Promise<void> {
    await this.updateMasterHeartbeat();
    
    // First, set all current slaves to offline (we'll re-enable ones that appear in incoming)
    await db.update(slaves).set({ online: false });

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
      .set({ wifiMode: 0, masterLastSeen: null, pendingCommand: "WIPE_DATA", commandParams: "" })
      .where(eq(systemSettings.id, 1));
  }
}

export const storage = new DatabaseStorage();
