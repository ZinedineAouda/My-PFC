// ═══════════════════════════════════════════════════════════════
//  CONSTANTS — API config, theme colors, types
// ═══════════════════════════════════════════════════════════════

export const API_BASE = 'https://my-pfc-production.up.railway.app';
export const WS_URL = 'wss://my-pfc-production.up.railway.app/ws';

// ── Theme Colors (matching the website exactly) ──
export const C = {
  bg:            '#070b14',
  bgCard:        '#0f172acc',   // rgba(15,23,42,0.8)
  bgCardDim:     '#0f172a66',   // rgba(15,23,42,0.4)
  bgInput:       '#ffffff0a',   // rgba(255,255,255,0.04)

  emerald:       '#10b981',
  emeraldDim:    '#10b98126',   // rgba(16,185,129,0.15)
  emeraldBorder: '#10b98133',   // rgba(16,185,129,0.2)
  emeraldGlow:   '#10b98140',

  red:           '#ef4444',
  redDim:        '#ef44441a',   // rgba(239,68,68,0.1)
  redBorder:     '#ef44441a',
  redPulse:      '#ef44440d',

  amber:         '#f59e0b',
  amberDim:      '#f59e0b1a',

  white:         '#ffffff',
  slate100:      '#f1f5f9',
  slate400:      '#94a3b8',
  slate500:      '#64748b',
  slate600:      '#475569',

  border:        '#ffffff1a',   // rgba(255,255,255,0.1)
  borderLight:   '#ffffff0d',   // rgba(255,255,255,0.05)
};

// ── Patient device type (matches shared/schema.ts) ──
export type Device = {
  deviceId: string;
  patientName: string;
  bed: string;
  room: string;
  alertActive: boolean;
  lastAlertTime: string | number | null;
  registered: boolean;
  approved: boolean;
  online: boolean;
  lastSeen: number | null;
};

export type StatusData = {
  mode: number;
  isControllerOnline?: boolean;
};

export type WsMessage = {
  type: 'ALERT' | 'REGISTER' | 'UPDATE' | 'DELETE' | 'CONTROLLER_STATUS' | 'FULL_STATE' | 'CLEAR';
  payload: any;
};
