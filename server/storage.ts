import type { Slave, InsertSlave, UpdateSlave } from "@shared/schema";

export interface IStorage {
  getAllSlaves(): Slave[];
  getSlave(slaveId: string): Slave | undefined;
  addSlave(data: InsertSlave): Slave;
  updateSlave(slaveId: string, data: UpdateSlave): Slave | undefined;
  deleteSlave(slaveId: string): boolean;
  registerSlave(slaveId: string): boolean;
  triggerAlert(slaveId: string): { success: boolean; reason?: string };
  clearAlert(slaveId: string): boolean;
}

export class MemStorage implements IStorage {
  private slaves: Map<string, Slave>;

  constructor() {
    this.slaves = new Map();
  }

  getAllSlaves(): Slave[] {
    return Array.from(this.slaves.values());
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
      patientName: data.patientName,
      bed: data.bed,
      room: data.room,
      alertActive: false,
      lastAlertTime: null,
      registered: false,
    };
    this.slaves.set(data.slaveId, slave);
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

  registerSlave(slaveId: string): boolean {
    const slave = this.slaves.get(slaveId);
    if (!slave) return false;
    slave.registered = true;
    this.slaves.set(slaveId, slave);
    return true;
  }

  triggerAlert(slaveId: string): { success: boolean; reason?: string } {
    const slave = this.slaves.get(slaveId);
    if (!slave) return { success: false, reason: "unknown slave" };
    if (slave.alertActive) return { success: false, reason: "alert already active" };

    slave.alertActive = true;
    slave.lastAlertTime = new Date().toISOString();
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

  seed() {
    const seedData: InsertSlave[] = [
      { slaveId: "s1", patientName: "Maria Garcia", bed: "1A", room: "101" },
      { slaveId: "s2", patientName: "James Wilson", bed: "2B", room: "102" },
      { slaveId: "s3", patientName: "Sarah Chen", bed: "3A", room: "201" },
      { slaveId: "s4", patientName: "Robert Johnson", bed: "4C", room: "202" },
      { slaveId: "s5", patientName: "Emily Davis", bed: "5B", room: "301" },
    ];

    for (const data of seedData) {
      if (!this.slaves.has(data.slaveId)) {
        this.addSlave(data);
        const slave = this.slaves.get(data.slaveId)!;
        slave.registered = true;
        this.slaves.set(data.slaveId, slave);
      }
    }

    const s2 = this.slaves.get("s2");
    if (s2 && !s2.alertActive) {
      this.triggerAlert("s2");
    }
  }
}

export const storage = new MemStorage();
