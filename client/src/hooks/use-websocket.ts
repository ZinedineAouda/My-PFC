import { useEffect, useRef, useState } from "react";

export type WsMessage = {
  type: "ALERT" | "REGISTER" | "UPDATE" | "DELETE" | "MASTER_STATUS";
  payload: any;
};

export function useWebsocket(onMessage?: (msg: WsMessage) => void) {
  const [connected, setConnected] = useState(false);
  const ws = useRef<WebSocket | null>(null);

  useEffect(() => {
    const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
    const host = window.location.host;
    const url = `${protocol}//${host}/ws`;

    const connect = () => {
      ws.current = new WebSocket(url);

      ws.current.onopen = () => {
        setConnected(true);
        console.log("WS Connected");
      };

      ws.current.onmessage = (event) => {
        try {
          const msg: WsMessage = JSON.parse(event.data);
          onMessage?.(msg);
        } catch (err) {
          console.error("WS Parse Error", err);
        }
      };

      ws.current.onclose = () => {
        setConnected(false);
        console.log("WS Disconnected, retrying...");
        setTimeout(connect, 3000);
      };

      ws.current.onerror = (err) => {
        console.error("WS Error", err);
      };
    };

    connect();

    return () => {
      ws.current?.close();
    };
  }, []);

  return { connected };
}
