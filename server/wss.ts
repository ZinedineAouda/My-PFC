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

  wss.on("connection", (ws) => {
    log("Client connected", "WS");

    // Send full state snapshot on connect
    const fullState: WsMessage = {
      type: "FULL_STATE",
      payload: {
        devices: storage.getAllSlaves(),
        masterOnline: storage.isMasterOnline(),
        mode: storage.getMode(),
      },
    };
    ws.send(JSON.stringify(fullState));

    ws.on("error", (err) => log(`Error: ${err.message}`, "WS"));

    // Heartbeat to prevent Railway from killing idle connections
    const interval = setInterval(() => {
      if (ws.readyState === WebSocket.OPEN) {
        ws.ping();
      }
    }, 25000);

    ws.on("message", (data) => {
      try {
        const msg: WsMessage = JSON.parse(data.toString());
        log(`Received ${msg.type}`, "WS");

        if (msg.type === "REGISTER") {
          // Master registering a new slave
          storage.registerSlave(msg.payload.slaveId);
          broadcastAllDevices();
        } else if (msg.type === "ALERT") {
          // Master reporting an alert
          storage.triggerAlert(msg.payload.slaveId);
          broadcastAllDevices();
        } else if (msg.type === "UPDATE") {
          // Master syncing full state
          if (Array.isArray(msg.payload)) {
            storage.syncFromMaster(msg.payload);
            broadcastAllDevices();
          }
        }
      } catch (err) {
        log(`WS Message Error: ${err}`, "WS");
      }
    });

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

export function broadcastDeviceUpdate(slaveId: string) {
  const slave = storage.getSlave(slaveId);
  if (slave) {
    broadcast({ type: "UPDATE", payload: slave });
  }
}

export function broadcastAllDevices() {
  broadcast({
    type: "FULL_STATE",
    payload: {
      devices: storage.getAllSlaves(),
      masterOnline: storage.isMasterOnline(),
      mode: storage.getMode(),
    },
  });
}
