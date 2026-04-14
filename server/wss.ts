import { WebSocketServer, WebSocket } from "ws";
import type { Server } from "http";
import { log } from "./index";
import { storage } from "./storage";

export type WsMessage = {
  type: "ALERT" | "REGISTER" | "UPDATE" | "DELETE" | "MASTER_STATUS" | "FULL_STATE";
  payload: any;
};

let wss: WebSocketServer;

export function initWss(server: Server) {
  wss = new WebSocketServer({ server, path: "/ws" });

  wss.on("connection", async (ws) => {
    log("Client connected", "WS");

    // Send full state snapshot on connect
    const fullState: WsMessage = {
      type: "FULL_STATE",
      payload: {
        devices: await storage.getAllSlaves(),
        masterOnline: await storage.isMasterOnline(),
        mode: await storage.getMode(),
      },
    };
    ws.send(JSON.stringify(fullState));

    ws.on("message", async (data) => {
      try {
        const msg: WsMessage = JSON.parse(data.toString());
        log(`Received ${msg.type}`, "WS");

        if (msg.type === "REGISTER") {
          await storage.registerSlave(msg.payload.slaveId);
          await broadcastAllDevices();
        } else if (msg.type === "ALERT") {
          await storage.triggerAlert(msg.payload.slaveId);
          await broadcastAllDevices();
        } else if (msg.type === "UPDATE") {
          if (Array.isArray(msg.payload)) {
            await storage.syncFromMaster(msg.payload);
            await broadcastAllDevices();
          }
        }
      } catch (err) {
        log(`WS Message Error: ${err}`, "WS");
      }
    });

    ws.on("error", (err) => log(`Error: ${err.message}`, "WS"));

    // Heartbeat to prevent Railway from killing idle connections
    const interval = setInterval(() => {
      if (ws.readyState === WebSocket.OPEN) {
        ws.ping();
      }
    }, 25000);

    ws.on("close", () => {
      clearInterval(interval);
      log("Client disconnected", "WS");
    });
  });

  return wss;
}

export function broadcast(msg: WsMessage) {
  if (!wss) return;

  const data = JSON.stringify(msg);
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
}

export async function broadcastDeviceUpdate(slaveId: string) {
  const slave = await storage.getSlave(slaveId);
  if (slave) {
    broadcast({ type: "UPDATE", payload: slave });
  }
}

export async function broadcastAllDevices() {
  broadcast({
    type: "FULL_STATE",
    payload: {
      devices: await storage.getAllSlaves(),
      masterOnline: await storage.isMasterOnline(),
      mode: await storage.getMode(),
    },
  });
}
