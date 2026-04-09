import { WebSocketServer, WebSocket } from "ws";
import type { Server } from "http";
import { log } from "./index";

export type WsMessage = {
  type: "ALERT" | "REGISTER" | "UPDATE" | "DELETE" | "MASTER_STATUS";
  payload: any;
};

let wss: WebSocketServer;

export function initWss(server: Server) {
  wss = new WebSocketServer({ server, path: "/ws" });

  wss.on("connection", (ws) => {
    log("New client connected", "WS");

    ws.on("error", (err) => log(`Error: ${err.message}`, "WS"));

    // Heartbeat to prevent Railway from killing the connection
    const interval = setInterval(() => {
      if (ws.readyState === WebSocket.OPEN) {
        ws.ping();
      }
    }, 30000);

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
