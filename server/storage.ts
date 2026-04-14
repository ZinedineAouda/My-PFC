import type { Slave, InsertSlave, UpdateSlave, ApproveSlave } from "@shared/schema";
import { slaves } from "@shared/schema";
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
  reset(): Promise<void>;
}

export class DatabaseStorage implements IStorage {
  private masterLastSeen: number | null = null;
  private wifiMode: number = 0;

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
    }).returning();
    return slave;
  }

  async approveSlave(slaveId: string, data: ApproveSlave): Promise<Slave | undefined> {
    const [slave] = await db.update(slaves)
      .set({ ...data, approved: true })
      .where(eq(slaves.slaveId, slaveId))
      .returning();
    return slave;
  }

  async updateSlave(slaveId: string, data: UpdateSlave): Promise<Slave | undefined> {
    const [slave] = await db.update(slaves)
      .set(data)
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
        online: true
      })
      .where(eq(slaves.slaveId, slaveId))
      .returning();
    return { success: true, slave: updated };
  }

  async clearAlert(slaveId: string): Promise<boolean> {
    const [updated] = await db.update(slaves)
      .set({ alertActive: false })
      .where(eq(slaves.slaveId, slaveId))
      .returning();
    return !!updated;
  }

  async getMode(): Promise<number> {
    return this.wifiMode;
  }

  async setMode(mode: number): Promise<void> {
    this.wifiMode = mode;
  }

  async isSetupDone(): Promise<boolean> {
    return this.wifiMode !== 0;
  }

  async updateMasterHeartbeat(): Promise<void> {
    this.masterLastSeen = Date.now();
  }

  async isMasterOnline(): Promise<boolean> {
    if (!this.masterLastSeen) return false;
    return Date.now() - this.masterLastSeen < 60000;
  }

  async syncFromMaster(incoming: Array<any>): Promise<void> {
    await this.updateMasterHeartbeat();
    
    // First, set all current slaves to offline (we'll re-enable ones that appear in incoming)
    await db.update(slaves).set({ online: false });

    for (const remote of incoming) {
      const [existing] = await db.select().from(slaves).where(eq(slaves.slaveId, remote.slaveId));
      
      if (existing) {
        await db.update(slaves)
          .set({
            online: true,
            alertActive: remote.alertActive !== undefined ? (remote.alertActive && existing.alertActive) || remote.alertActive : existing.alertActive,
            approved: remote.approved || existing.approved,
            patientName: remote.patientName || existing.patientName,
            bed: remote.bed || existing.bed,
            room: remote.room || existing.room,
            lastSeen: Date.now()
          })
          .where(eq(slaves.slaveId, remote.slaveId));
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
        });
      }
    }
  }

  async reset(): Promise<void> {
    await db.delete(slaves);
    this.wifiMode = 0;
  }
}

export const storage = new DatabaseStorage();
