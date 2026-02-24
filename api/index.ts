import express, { type Request, Response, NextFunction } from "express";
import { registerRoutes } from "../server/routes";
import { createServer } from "http";

declare module "http" {
  interface IncomingMessage {
    rawBody: unknown;
  }
}

const app = express();
const httpServer = createServer(app);

app.use(
  express.json({
    verify: (req, _res, buf) => {
      req.rawBody = buf;
    },
  }),
);
app.use(express.urlencoded({ extended: false }));

let initialized = false;

async function init() {
  if (initialized) return;
  await registerRoutes(httpServer, app);
  app.use((err: any, _req: Request, res: Response, next: NextFunction) => {
    const status = err.status || err.statusCode || 500;
    const message = err.message || "Internal Server Error";
    if (res.headersSent) return next(err);
    return res.status(status).json({ message });
  });
  initialized = true;
}

export default async function handler(req: any, res: any) {
  await init();
  app(req, res);
}
