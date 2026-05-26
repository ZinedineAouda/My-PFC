import { Aedes } from "aedes";
import { createServer } from "net";
import mqtt from "mqtt";
import { storage } from "./storage";
import { broadcast, broadcastDeviceUpdate, broadcastAllDevices } from "./wss";
import { log } from "./log";

let mqttClient: mqtt.MqttClient | null = null;
let aedesBroker: any = null;

// ── Pending command queue: holds commands when broker temporarily disconnects ──
interface PendingCommand {
  deviceKey: string;
  command: string;
  params: string;
}
const pendingCommandQueue: PendingCommand[] = [];
const MAX_PENDING_COMMANDS = 50;

export async function initMqtt() {
  const brokerHost = process.env.MQTT_BROKER_HOST;
  const brokerPort = parseInt(process.env.MQTT_BROKER_PORT || "1883", 10);
  const brokerUser = process.env.MQTT_BROKER_USER;
  const brokerPass = process.env.MQTT_BROKER_PASS;

  // Deterministic client ID — prevents orphaned sessions on Aedes broker restarts
  const clientId = `railway-backend-${process.env.DEVICE_API_KEY || "super"}`;

  if (!brokerHost) {
    // 1. Start embedded Aedes broker
    log("Starting embedded Aedes MQTT broker...", "MQTT");
    aedesBroker = await Aedes.createBroker();
    const server = createServer(aedesBroker.handle);
    server.listen(brokerPort, "0.0.0.0", () => {
      log(`Embedded MQTT Broker listening on port ${brokerPort}`, "MQTT");
    });

    // Connect backend client to local broker
    log("Connecting backend MQTT client to local broker...", "MQTT");
    mqttClient = mqtt.connect(`mqtt://127.0.0.1:${brokerPort}`, {
      clientId,
      keepalive: 30,              // Railway kills connections idle > 60s
      reconnectPeriod: 3000,      // Retry every 3s on disconnect
      connectTimeout: 10000,
    });
  } else {
    // 2. Connect to external broker (HiveMQ, EMQX, etc.)
    const protocol = brokerPort === 8883 || brokerPort === 8884 ? "mqtts" : "mqtt";
    const options: mqtt.IClientOptions = {
      clientId,
      port: brokerPort,
      username: brokerUser,
      password: brokerPass,
      keepalive: 30,
      reconnectPeriod: 5000,
      connectTimeout: 15000,
      rejectUnauthorized: false, // Sane default for self-signed certs on hobby brokers
    };
    log(`Connecting backend MQTT client to ${protocol}://${brokerHost}:${brokerPort}...`, "MQTT");
    mqttClient = mqtt.connect(`${protocol}://${brokerHost}`, options);
  }

  mqttClient.on("connect", () => {
    log("Backend MQTT Client connected successfully!", "MQTT");

    // Subscribe to all controller topics
    mqttClient?.subscribe("controller/+/ping", { qos: 0 });
    mqttClient?.subscribe("controller/+/sync", { qos: 1 });
    mqttClient?.subscribe("controller/+/alert", { qos: 1 });

    // Flush any commands that were queued while we were disconnected
    _flushPendingCommands();
  });

  mqttClient.on("reconnect", () => {
    log("MQTT Client reconnecting...", "MQTT");
  });

  mqttClient.on("offline", () => {
    log("MQTT Client went offline — commands will be queued", "MQTT");
  });

  mqttClient.on("message", async (topic, message) => {
    try {
      const topicParts = topic.split("/");
      const deviceKey = topicParts[1];
      const subTopic = topicParts[2];

      const payloadString = message.toString();
      const payload = JSON.parse(payloadString);

      log(`Received [${subTopic}] from controller '${deviceKey}'`, "MQTT");

      if (subTopic === "ping") {
        const { mode, uptime, rssi, wifiError } = payload;
        await storage.updateControllerHeartbeat(mode, uptime, rssi, wifiError);
        broadcast({
          type: "CONTROLLER_STATUS",
          payload: { online: true, uptime, rssi, wifiError },
        });
      }
      else if (subTopic === "sync") {
        const { mode, uptime, rssi, wifiError, devices } = payload;
        await storage.syncFromController(devices || [], { mode, uptime, rssi, wifiError });

        // Respond with the cloud's authoritative device list
        const allDevices = await storage.getAllDevices();
        mqttClient?.publish(
          `controller/${deviceKey}/sync_response`,
          JSON.stringify({ devices: allDevices }),
          { qos: 1 }
        );

        // Await to catch any DB errors in broadcastAllDevices
        await broadcastAllDevices();
      }
      else if (subTopic === "alert") {
        const { deviceId } = payload;
        if (deviceId) {
          const result = await storage.triggerAlert(deviceId);
          if (result.success) {
            broadcast({ type: "ALERT", payload: { deviceId } });
            await broadcastDeviceUpdate(deviceId);
          }
        }
      }
    } catch (err) {
      log(`Error handling MQTT message on topic ${topic}: ${err}`, "MQTT");
    }
  });

  mqttClient.on("error", (err) => {
    log(`MQTT Client error: ${err.message}`, "MQTT");
  });
}

// ── Publish a command to the controller, with queuing if disconnected ─────────
export function publishCommand(deviceKey: string, command: string, params: string = "") {
  const payload = JSON.stringify({ command, params });
  const topic = `controller/${deviceKey}/command`;

  if (mqttClient?.connected) {
    log(`Publishing command '${command}' to ${topic}`, "MQTT");
    mqttClient.publish(topic, payload, { qos: 1 });
  } else {
    // Queue the command to be sent when connection is restored
    if (pendingCommandQueue.length >= MAX_PENDING_COMMANDS) {
      log(`Command queue full — dropping oldest command`, "MQTT");
      pendingCommandQueue.shift();
    }
    pendingCommandQueue.push({ deviceKey, command, params });
    log(`MQTT disconnected — command '${command}' queued (${pendingCommandQueue.length} pending)`, "MQTT");
  }
}

// ── Flush all queued commands once the broker reconnects ──────────────────────
function _flushPendingCommands() {
  if (pendingCommandQueue.length === 0) return;
  log(`Flushing ${pendingCommandQueue.length} queued commands...`, "MQTT");
  while (pendingCommandQueue.length > 0) {
    const cmd = pendingCommandQueue.shift()!;
    const payload = JSON.stringify({ command: cmd.command, params: cmd.params });
    const topic = `controller/${cmd.deviceKey}/command`;
    mqttClient?.publish(topic, payload, { qos: 1 });
    log(`Flushed command '${cmd.command}' to ${topic}`, "MQTT");
  }
}
