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
  triggerAlert(slaveId: string): { success: boolean; slave?: Slave; reason?: string };
  clearAlert(slaveId: string): boolean;
  getMode(): number;
  setMode(mode: number): void;
  isSetupDone(): boolean;
  updateMasterHeartbeat(): void;
  isMasterOnline(): boolean;
  syncFromMaster(slaves: Array<{
    slaveId: string;
    patientName?: string;
    bed?: string;
    room?: string;
    alertActive?: boolean;
    approved?: boolean;
    online?: boolean;
  }>): void;
}

export class MemStorage implements IStorage {
  private slaves: Map<string, Slave>;
  private wifiMode: number;
  private setupComplete: boolean;
  private masterLastSeen: number | null;

  constructor() {
    this.slaves = new Map();
    this.wifiMode = 0;
    this.setupComplete = false;
    this.masterLastSeen = null;
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
      online: false,
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
      slave.online = true;
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
      online: true,
      lastSeen: Date.now(),
    };
    this.slaves.set(slaveId, slave);
    return slave;
  }

  triggerAlert(slaveId: string): { success: boolean; slave?: Slave; reason?: string } {
    const slave = this.slaves.get(slaveId);
    if (!slave) return { success: false, reason: "unknown slave" };
    if (!slave.approved) return { success: false, reason: "not approved" };
    if (slave.alertActive) return { success: false, slave, reason: "alert already active" };
    slave.alertActive = true;
    slave.lastAlertTime = new Date().toISOString();
    slave.lastSeen = Date.now();
    slave.online = true;
    this.slaves.set(slaveId, slave);
    return { success: true, slave };
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

  private alertClearedLocally: Set<string> = new Set();

  updateMasterHeartbeat(): void {
    this.masterLastSeen = Date.now();
  }

  isMasterOnline(): boolean {
    if (!this.masterLastSeen) return false;
    // 60s window — tolerates intermittent TLS handshake failures
    return Date.now() - this.masterLastSeen < 60000;
  }

  // ── Bidirectional sync from ESP32 master ──────────────────
  syncFromMaster(incoming: Array<{
    slaveId: string;
    patientName?: string;
    bed?: string;
    room?: string;
    alertActive?: boolean;
    approved?: boolean;
    online?: boolean;
  }>): void {
    this.updateMasterHeartbeat();

    for (const remote of incoming) {
      const local = this.slaves.get(remote.slaveId);
      if (local) {
        // Update online/alert state from master
        if (remote.online !== undefined) local.online = remote.online;

        // ── Alert sync: CLEARS flow bidirectionally, activations DON'T ──
        // New alerts reach the cloud via the /api/alert endpoint.
        // Sync only propagates clears to prevent the loop:
        //   cloud clears → ESP32 re-sends true → cloud re-activates → forever
        if (remote.alertActive !== undefined) {
          if (!remote.alertActive && local.alertActive) {
            // ESP32 cleared an alert → clear on cloud too
            local.alertActive = false;
            this.alertClearedLocally.delete(remote.slaveId);
          }
          // If master says alert active BUT cloud already cleared it: ignore
          // The cloud response will tell the ESP32 to clear on next sync
        }

        // Accept approval from master (bidirectional: local dashboard can approve)
        if (remote.approved && !local.approved) {
          local.approved = true;
        }

        // Update metadata if provided by master
        if (remote.patientName) local.patientName = remote.patientName;
        if (remote.bed) local.bed = remote.bed;
        if (remote.room) local.room = remote.room;

        local.lastSeen = Date.now();
        this.slaves.set(remote.slaveId, local);
      } else {
        // New device discovered by master — auto-register
        const newSlave: Slave = {
          slaveId: remote.slaveId,
          patientName: remote.patientName || "",
          bed: remote.bed || "",
          room: remote.room || "",
          alertActive: remote.alertActive || false,
          lastAlertTime: remote.alertActive ? new Date().toISOString() : null,
          registered: true,
          approved: remote.approved || false,
          online: remote.online || false,
          lastSeen: Date.now(),
        };
        this.slaves.set(remote.slaveId, newSlave);
      }
    }
  }
}

export const storage = new MemStorage();
