import type { Slave, InsertSlave, UpdateSlave, ApproveSlave } from "@shared/schema";

export interface IStorage {
  getAllSlaves(): Slave[];
  getApprovedSlaves(): Slave[];
  getSlave(slaveId: string): Slave | undefined;
  addSlave(data: InsertSlave): Slave;
  approveSlave(slaveId: string, data: ApproveSlave): Slave | undefined;
  updateSlave(slaveId: string, data: UpdateSlave): Slave | undefined;
  deleteSlave(slaveId: string): boolean;
  registerSlave(slaveId: string): Slave;
  triggerAlert(slaveId: string): { success: boolean; reason?: string };
  clearAlert(slaveId: string): boolean;
  getMode(): number;
  setMode(mode: number): void;
  isSetupDone(): boolean;
}

export class MemStorage implements IStorage {
  private slaves: Map<string, Slave>;
  private wifiMode: number;
  private setupComplete: boolean;

  constructor() {
    this.slaves = new Map();
    this.wifiMode = 0;
    this.setupComplete = false;
  }

  getAllSlaves(): Slave[] {
    return Array.from(this.slaves.values());
  }

  getApprovedSlaves(): Slave[] {
    return Array.from(this.slaves.values()).filter(s => s.approved);
  }

  getSlave(slaveId: string): Slave | undefined {
    return this.slaves.get(slaveId);
  }

  addSlave(data: InsertSlave): Slave {
    if (this.slaves.has(data.slaveId)) {
      throw new Error("Slave ID already exists");
    }
    const slave: Slave = {
      slaveId: data.slaveId,
      patientName: data.patientName || "",
      bed: data.bed || "",
      room: data.room || "",
      alertActive: false,
      lastAlertTime: null,
      registered: false,
      approved: false,
      lastSeen: null,
    };
    this.slaves.set(data.slaveId, slave);
    return slave;
  }

  approveSlave(slaveId: string, data: ApproveSlave): Slave | undefined {
    const slave = this.slaves.get(slaveId);
    if (!slave) return undefined;
    slave.patientName = data.patientName;
    slave.bed = data.bed;
    slave.room = data.room;
    slave.approved = true;
    this.slaves.set(slaveId, slave);
    return slave;
  }

  updateSlave(slaveId: string, data: UpdateSlave): Slave | undefined {
    const slave = this.slaves.get(slaveId);
    if (!slave) return undefined;
    if (data.patientName !== undefined) slave.patientName = data.patientName;
    if (data.bed !== undefined) slave.bed = data.bed;
    if (data.room !== undefined) slave.room = data.room;
    this.slaves.set(slaveId, slave);
    return slave;
  }

  deleteSlave(slaveId: string): boolean {
    return this.slaves.delete(slaveId);
  }

  registerSlave(slaveId: string): Slave {
    let slave = this.slaves.get(slaveId);
    if (slave) {
      slave.registered = true;
      slave.lastSeen = Date.now();
      this.slaves.set(slaveId, slave);
      return slave;
    }
    slave = {
      slaveId,
      patientName: "",
      bed: "",
      room: "",
      alertActive: false,
      lastAlertTime: null,
      registered: true,
      approved: false,
      lastSeen: Date.now(),
    };
    this.slaves.set(slaveId, slave);
    return slave;
  }

  triggerAlert(slaveId: string): { success: boolean; reason?: string } {
    const slave = this.slaves.get(slaveId);
    if (!slave) return { success: false, reason: "unknown slave" };
    if (!slave.approved) return { success: false, reason: "not approved" };
    if (slave.alertActive) return { success: false, reason: "alert already active" };
    slave.alertActive = true;
    slave.lastAlertTime = new Date().toISOString();
    slave.lastSeen = Date.now();
    this.slaves.set(slaveId, slave);
    return { success: true };
  }

  clearAlert(slaveId: string): boolean {
    const slave = this.slaves.get(slaveId);
    if (!slave) return false;
    slave.alertActive = false;
    this.slaves.set(slaveId, slave);
    return true;
  }

  getMode(): number {
    return this.wifiMode;
  }

  setMode(mode: number): void {
    this.wifiMode = mode;
    this.setupComplete = true;
  }

  isSetupDone(): boolean {
    return this.setupComplete;
  }
}

export const storage = new MemStorage();
