import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  RefreshControl,
  Alert,
  Animated,
} from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { MaterialCommunityIcons } from '@expo/vector-icons';
import { Audio } from 'expo-av';
import * as Haptics from 'expo-haptics';
import * as Notifications from 'expo-notifications';
import * as Device from 'expo-device';
import { C, API_BASE, WS_URL, type Slave, type StatusData, type WsMessage } from './constants';
import SlaveModal from './SlaveModal';

// ── Configure notification handler ──
try {
  Notifications.setNotificationHandler({
    handleNotification: async () => ({
      shouldShowAlert: true,
      shouldPlaySound: true,
      shouldSetBadge: true,
    }),
  });
} catch (e) {
  // Suppress missing native module errors gracefully inside Expo Go
}

type Props = { onLogout: () => void };

export default function DashboardScreen({ onLogout }: Props) {
  // ── State ──
  const [slaves, setSlaves] = useState<Slave[]>([]);
  const [status, setStatus] = useState<StatusData>({ mode: 0 });
  const [refreshing, setRefreshing] = useState(false);
  const [modalSlave, setModalSlave] = useState<Slave | null>(null);
  const [modalMode, setModalMode] = useState<'approve' | 'edit'>('approve');
  const [modalLoading, setModalLoading] = useState(false);

  const sirenRef = useRef<Audio.Sound | null>(null);
  const wsRef = useRef<WebSocket | null>(null);
  const alertPulse = useRef(new Animated.Value(0)).current;

  // ── Derived ──
  const pending = slaves.filter(s => !s.approved);
  const approved = slaves.filter(s => s.approved);
  const onlineCount = slaves.filter(s => s.online).length;
  const alertCount = slaves.filter(s => s.alertActive).length;
  const isMasterOnline = status?.isMasterOnline ?? false;

  // ── Alert pulse animation ──
  useEffect(() => {
    if (alertCount > 0) {
      const anim = Animated.loop(
        Animated.sequence([
          Animated.timing(alertPulse, { toValue: 1, duration: 800, useNativeDriver: true }),
          Animated.timing(alertPulse, { toValue: 0, duration: 800, useNativeDriver: true }),
        ])
      );
      anim.start();
      return () => anim.stop();
    } else {
      alertPulse.setValue(0);
    }
  }, [alertCount]);

  // ── Siren setup ──
  useEffect(() => {
    const loadSiren = async () => {
      try {
        await Audio.setAudioModeAsync({
          allowsRecordingIOS: false,
          playsInSilentModeIOS: true,
          staysActiveInBackground: true,
          shouldDuckAndroid: false,
        });
        const { sound } = await Audio.Sound.createAsync(
          { uri: 'https://raw.githubusercontent.com/Anis-Aouda/Zinedine-Audio/main/siren.mp3' },
          { isLooping: true, shouldPlay: false }
        );
        sirenRef.current = sound;
      } catch (e) {
        console.log('Siren load failed:', e);
      }
    };
    loadSiren();
    return () => {
      sirenRef.current?.unloadAsync();
    };
  }, []);

  // ── Notification permissions ──
  useEffect(() => {
    const setup = async () => {
      try {
        if (!Device.isDevice) return;
        const { status: existing } = await Notifications.getPermissionsAsync();
        if (existing !== 'granted') {
          await Notifications.requestPermissionsAsync();
        }
      } catch (e) {
        console.log('Skipping push notifications setup in Expo Go', e);
      }
    };
    setup();
  }, []);

  // ── Data fetching ──
  const fetchSlaves = useCallback(async () => {
    try {
      const res = await fetch(`${API_BASE}/api/slaves?all=1`, { credentials: 'include' });
      if (res.ok) {
        const data = await res.json();
        if (Array.isArray(data)) setSlaves(data);
      }
    } catch (e) {
      console.log('Fetch slaves error:', e);
    }
  }, []);

  const fetchStatus = useCallback(async () => {
    try {
      const res = await fetch(`${API_BASE}/api/status`, { credentials: 'include' });
      if (res.ok) {
        const data = await res.json();
        setStatus(data);
      }
    } catch (e) {
      console.log('Fetch status error:', e);
    }
  }, []);

  const refreshAll = useCallback(async () => {
    setRefreshing(true);
    await Promise.all([fetchSlaves(), fetchStatus()]);
    setRefreshing(false);
  }, [fetchSlaves, fetchStatus]);

  // ── Initial load + polling ──
  useEffect(() => {
    fetchSlaves();
    fetchStatus();
    const interval = setInterval(() => {
      fetchSlaves();
      fetchStatus();
    }, 10000);
    return () => clearInterval(interval);
  }, [fetchSlaves, fetchStatus]);

  // ── WebSocket ──
  useEffect(() => {
    let alive = true;

    const connect = () => {
      if (!alive) return;
      const ws = new WebSocket(WS_URL);
      wsRef.current = ws;

      ws.onopen = () => console.log('WS Connected');

      ws.onmessage = (event) => {
        try {
          const msg: WsMessage = JSON.parse(event.data);
          // Refresh data on any update
          fetchSlaves();
          fetchStatus();

          if (msg.type === 'ALERT') {
            handleAlertReceived(msg.payload?.slaveId || 'Unknown');
          }
          if (msg.type === 'CLEAR') {
            stopSiren();
          }
        } catch (e) {
          console.log('WS parse error:', e);
        }
      };

      ws.onclose = () => {
        console.log('WS disconnected, reconnecting...');
        setTimeout(connect, 3000);
      };

      ws.onerror = (e) => console.log('WS error:', e);
    };

    connect();

    return () => {
      alive = false;
      wsRef.current?.close();
    };
  }, []);

  // ── Alert handling ──
  const handleAlertReceived = async (slaveId: string) => {
    // 1. Play siren
    try {
      await sirenRef.current?.playAsync();
    } catch {}

    // 2. Haptic vibration
    try {
      await Haptics.notificationAsync(Haptics.NotificationFeedbackType.Error);
    } catch {}

    // 3. Push notification
    try {
      await Notifications.scheduleNotificationAsync({
        content: {
          title: '🚨 EMERGENCY ALERT',
          body: `Patient Alert from ${slaveId}`,
          sound: true,
          priority: Notifications.AndroidNotificationPriority.MAX,
        },
        trigger: null,
      });
    } catch {}
  };

  const stopSiren = async () => {
    try {
      await sirenRef.current?.stopAsync();
      await sirenRef.current?.setPositionAsync(0);
    } catch {}
  };

  // ── API mutations ──
  const apiRequest = async (method: string, path: string, body?: any) => {
    const res = await fetch(`${API_BASE}${path}`, {
      method,
      headers: body ? { 'Content-Type': 'application/json' } : {},
      body: body ? JSON.stringify(body) : undefined,
      credentials: 'include',
    });
    if (!res.ok) {
      const text = await res.text();
      throw new Error(text || res.statusText);
    }
    return res.json();
  };

  const handleApprove = async (name: string, bed: string, room: string) => {
    if (!modalSlave) return;
    setModalLoading(true);
    try {
      await apiRequest('POST', `/api/approve/${modalSlave.slaveId}`, { patientName: name, bed, room });
      setModalSlave(null);
      fetchSlaves();
    } catch (e: any) {
      Alert.alert('Failed', e.message);
    } finally {
      setModalLoading(false);
    }
  };

  const handleUpdate = async (name: string, bed: string, room: string) => {
    if (!modalSlave) return;
    setModalLoading(true);
    try {
      await apiRequest('PUT', `/api/slaves/${modalSlave.slaveId}`, { patientName: name, bed, room });
      setModalSlave(null);
      fetchSlaves();
    } catch (e: any) {
      Alert.alert('Failed', e.message);
    } finally {
      setModalLoading(false);
    }
  };

  const handleDelete = (slaveId: string) => {
    Alert.alert('Confirm', `Delete ${slaveId}?`, [
      { text: 'Cancel', style: 'cancel' },
      {
        text: 'Delete',
        style: 'destructive',
        onPress: async () => {
          try {
            await apiRequest('DELETE', `/api/slaves/${slaveId}`);
            fetchSlaves();
          } catch (e: any) {
            Alert.alert('Failed', e.message);
          }
        },
      },
    ]);
  };

  const handleClearAlert = async (slaveId: string) => {
    try {
      await apiRequest('POST', `/api/clearAlert/${slaveId}`);
      stopSiren();
      fetchSlaves();
    } catch (e: any) {
      Alert.alert('Failed', e.message);
    }
  };

  const handleLogout = async () => {
    try {
      await apiRequest('POST', '/api/admin/logout');
    } catch {}
    stopSiren();
    onLogout();
  };

  // ── Helpers ──
  const formatTime = (t: string | number | null | undefined) => {
    if (!t) return 'Never';
    const ms = typeof t === 'string' ? new Date(t).getTime() : t;
    const sec = Math.floor((Date.now() - ms) / 1000);
    if (sec < 5) return 'Just now';
    if (sec < 60) return sec + 's ago';
    if (sec < 3600) return Math.floor(sec / 60) + 'm ago';
    return Math.floor(sec / 3600) + 'h ago';
  };

  // ── Render ──
  return (
    <SafeAreaView style={s.safe} edges={['top']}>
      {/* ═══ HEADER ═══ */}
      <View style={s.header}>
        <View style={s.headerLeft}>
          <View style={s.headerIcon}>
            <MaterialCommunityIcons name="shield-check" size={24} color={C.emerald} />
          </View>
          <View>
            <Text style={s.headerTitle}>Security Command</Text>
            <Text style={s.headerSub}>ADMINISTRATION</Text>
          </View>
        </View>
        <View style={s.headerRight}>
          <View style={[s.statusBadge, isMasterOnline ? s.statusOnline : s.statusOffline]}>
            <View style={[s.statusDot, isMasterOnline ? s.dotOnline : s.dotOffline]} />
            <MaterialCommunityIcons
              name="refresh"
              size={12}
              color={isMasterOnline ? C.emerald : C.red}
            />
            <Text style={[s.statusText, { color: isMasterOnline ? C.emerald : C.red }]}>
              {isMasterOnline ? 'CONNECTED' : 'OFFLINE'}
            </Text>
          </View>
          <TouchableOpacity style={s.logoutBtn} onPress={handleLogout} activeOpacity={0.7}>
            <MaterialCommunityIcons name="logout" size={14} color={C.slate400} />
            <Text style={s.logoutText}>Exit</Text>
          </TouchableOpacity>
        </View>
      </View>

      <ScrollView
        style={s.scroll}
        contentContainerStyle={s.scrollContent}
        refreshControl={
          <RefreshControl
            refreshing={refreshing}
            onRefresh={refreshAll}
            tintColor={C.emerald}
            colors={[C.emerald]}
            progressBackgroundColor={C.bg}
          />
        }
      >
        {/* ═══ STATS GRID ═══ */}
        <View style={s.statsGrid}>
          {[
            { icon: '📡', val: approved.length, lbl: 'Total', bg: '#ffffff0a' },
            { icon: '🟢', val: onlineCount, lbl: 'Online', bg: '#10b9811a' },
            { icon: '🚨', val: alertCount, lbl: 'Alerts', bg: '#ef44441a' },
            { icon: '⏳', val: pending.length, lbl: 'Pending', bg: '#f59e0b1a' },
          ].map(item => (
            <View key={item.lbl} style={s.statCard}>
              <View style={[s.statIcon, { backgroundColor: item.bg }]}>
                <Text style={s.statEmoji}>{item.icon}</Text>
              </View>
              <View>
                <Text style={s.statVal}>{item.val}</Text>
                <Text style={s.statLbl}>{item.lbl}</Text>
              </View>
            </View>
          ))}
        </View>

        {/* ═══ DISCOVERY QUEUE (Pending) ═══ */}
        {pending.length > 0 && (
          <View style={s.section}>
            <View style={s.sectionHeader}>
              <Text style={s.sectionEmoji}>⚠️</Text>
              <Text style={s.sectionTitle}>Discovery Queue</Text>
              <View style={s.badge}>
                <Text style={s.badgeText}>{pending.length} NEW</Text>
              </View>
            </View>
            {pending.map(slave => (
              <View key={slave.slaveId} style={s.pendingCard}>
                <View>
                  <Text style={s.pendingId}>{slave.slaveId}</Text>
                  <Text style={s.pendingSub}>Signal Detected</Text>
                </View>
                <View style={s.pendingActions}>
                  <TouchableOpacity
                    style={s.approveBtn}
                    onPress={() => { setModalMode('approve'); setModalSlave(slave); }}
                    activeOpacity={0.8}
                  >
                    <Text style={s.approveBtnText}>Authorize</Text>
                  </TouchableOpacity>
                  <TouchableOpacity
                    style={s.deleteMiniBtn}
                    onPress={() => handleDelete(slave.slaveId)}
                    activeOpacity={0.8}
                  >
                    <MaterialCommunityIcons name="delete-outline" size={14} color={C.red} />
                  </TouchableOpacity>
                </View>
              </View>
            ))}
          </View>
        )}

        {/* ═══ ACTIVE INFRASTRUCTURE ═══ */}
        <View style={s.section}>
          <View style={s.sectionHeader}>
            <Text style={s.sectionEmoji}>✅</Text>
            <Text style={s.sectionTitle}>Active Infrastructure</Text>
            <View style={s.badge}>
              <Text style={s.badgeText}>{approved.length} NODES</Text>
            </View>
          </View>

          {approved.length === 0 ? (
            <View style={s.emptyCard}>
              <Text style={s.emptyEmoji}>📡</Text>
              <Text style={s.emptyText}>
                Scanning for new hardware...{'\n'}Connect a slave device to begin monitoring.
              </Text>
            </View>
          ) : (
            approved.map(d => {
              const isAlert = d.alertActive;
              const isOnline = d.online;
              const badgeLabel = isAlert ? 'EMERGENCY' : isOnline ? 'LINKED' : 'OFFLINE';
              const statusLabel = isAlert ? 'ALERT ACTIVE' : isOnline ? 'SYSTEM STABLE' : 'LINK DOWN';

              return (
                <View
                  key={d.slaveId}
                  style={[
                    s.deviceCard,
                    isAlert && s.deviceAlert,
                    !isOnline && !isAlert && s.deviceOffline,
                  ]}
                >
                  {/* Alert pulse overlay */}
                  {isAlert && (
                    <Animated.View
                      style={[s.alertOverlay, { opacity: alertPulse }]}
                      pointerEvents="none"
                    />
                  )}

                  {/* Header row */}
                  <View style={s.deviceHeader}>
                    <View>
                      <Text style={s.deviceName}>{d.patientName || 'Unnamed'}</Text>
                      <Text style={s.deviceId}>ID: {d.slaveId}</Text>
                    </View>
                    <View style={[
                      s.deviceBadge,
                      isAlert ? s.badgeEmergency : isOnline ? s.badgeLinked : s.badgeOff,
                    ]}>
                      <Text style={[
                        s.deviceBadgeText,
                        isAlert ? s.badgeEmergencyText : isOnline ? s.badgeLinkedText : s.badgeOffText,
                      ]}>
                        {badgeLabel}
                      </Text>
                    </View>
                  </View>

                  {/* Bed / Room */}
                  <View style={s.infoGrid}>
                    <View style={s.infoBox}>
                      <Text style={s.infoLabel}>BED</Text>
                      <Text style={s.infoVal}>{d.bed || '-'}</Text>
                    </View>
                    <View style={s.infoBox}>
                      <Text style={s.infoLabel}>ROOM</Text>
                      <Text style={s.infoVal}>{d.room || '-'}</Text>
                    </View>
                  </View>

                  {/* Action buttons */}
                  <View style={s.deviceActions}>
                    <TouchableOpacity
                      style={s.modifyBtn}
                      onPress={() => { setModalMode('edit'); setModalSlave(d); }}
                      activeOpacity={0.7}
                    >
                      <MaterialCommunityIcons name="pencil-outline" size={12} color={C.slate400} />
                      <Text style={s.modifyBtnText}>Modify</Text>
                    </TouchableOpacity>
                    {isAlert && (
                      <TouchableOpacity
                        style={s.silenceBtn}
                        onPress={() => handleClearAlert(d.slaveId)}
                        activeOpacity={0.7}
                      >
                        <MaterialCommunityIcons name="bell-off-outline" size={12} color={C.red} />
                        <Text style={s.silenceBtnText}>Silence</Text>
                      </TouchableOpacity>
                    )}
                    <TouchableOpacity
                      style={s.deleteBtn}
                      onPress={() => handleDelete(d.slaveId)}
                      activeOpacity={0.7}
                    >
                      <MaterialCommunityIcons name="delete-outline" size={12} color={C.red} />
                    </TouchableOpacity>
                  </View>

                  {/* Footer */}
                  <View style={s.deviceFooter}>
                    <View style={s.footerStatus}>
                      <View style={[
                        s.footerDot,
                        isAlert ? s.dotAlert : isOnline ? s.dotOnline : s.dotOff,
                      ]} />
                      <Text style={s.footerText}>{statusLabel}</Text>
                    </View>
                    <Text style={s.footerText}>🕐 {formatTime(d.lastAlertTime)}</Text>
                  </View>
                </View>
              );
            })
          )}
        </View>

        <View style={{ height: 40 }} />
      </ScrollView>

      {/* ═══ MODAL ═══ */}
      <SlaveModal
        visible={!!modalSlave}
        title={modalMode === 'approve'
          ? `Node Authorization: ${modalSlave?.slaveId}`
          : `Modify: ${modalSlave?.slaveId}`}
        onClose={() => setModalSlave(null)}
        onSubmit={modalMode === 'approve' ? handleApprove : handleUpdate}
        loading={modalLoading}
        defaults={modalMode === 'edit' ? modalSlave ?? undefined : undefined}
      />
    </SafeAreaView>
  );
}

// ═══════════════════════════════════════════════════════════════
//  STYLES — Pixel-perfect match with website
// ═══════════════════════════════════════════════════════════════
const s = StyleSheet.create({
  safe: { flex: 1, backgroundColor: C.bg },

  // ── Header ──
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: C.borderLight,
    backgroundColor: '#070b14cc',
  },
  headerLeft: { flexDirection: 'row', alignItems: 'center', gap: 10 },
  headerIcon: {
    width: 44,
    height: 44,
    backgroundColor: '#0f172acc',
    borderWidth: 1,
    borderColor: C.border,
    borderRadius: 14,
    alignItems: 'center',
    justifyContent: 'center',
  },
  headerTitle: { fontSize: 16, fontWeight: '800', color: C.white, letterSpacing: -0.3 },
  headerSub: { fontSize: 9, color: '#10b98180', fontWeight: '700', letterSpacing: 1.5 },
  headerRight: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  statusBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
    paddingHorizontal: 10,
    paddingVertical: 6,
    borderRadius: 12,
    borderWidth: 1,
  },
  statusOnline: { backgroundColor: '#10b98114', borderColor: '#10b98126' },
  statusOffline: { backgroundColor: '#ef444414', borderColor: '#ef444426' },
  statusDot: { width: 6, height: 6, borderRadius: 3 },
  dotOnline: { backgroundColor: C.emerald },
  dotOffline: { backgroundColor: C.red },
  dotOff: { backgroundColor: C.slate600 },
  dotAlert: { backgroundColor: C.red },
  statusText: { fontSize: 9, fontWeight: '700', letterSpacing: 1 },
  logoutBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
    paddingHorizontal: 10,
    paddingVertical: 6,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: C.border,
  },
  logoutText: { fontSize: 11, fontWeight: '700', color: C.slate400 },

  // ── Scroll ──
  scroll: { flex: 1 },
  scrollContent: { paddingHorizontal: 16, paddingTop: 16 },

  // ── Stats Grid ──
  statsGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 10,
    marginBottom: 16,
  },
  statCard: {
    flex: 1,
    minWidth: '45%',
    backgroundColor: '#0f172acc',
    borderWidth: 1,
    borderColor: C.border,
    borderRadius: 14,
    paddingHorizontal: 14,
    paddingVertical: 12,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 10,
  },
  statIcon: {
    width: 40,
    height: 40,
    borderRadius: 12,
    alignItems: 'center',
    justifyContent: 'center',
  },
  statEmoji: { fontSize: 18 },
  statVal: { fontSize: 20, fontWeight: '800', color: C.white },
  statLbl: { fontSize: 9, color: C.slate500, fontWeight: '700', letterSpacing: 1, textTransform: 'uppercase' },

  // ── Section ──
  section: { marginBottom: 20 },
  sectionHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    marginBottom: 10,
  },
  sectionEmoji: { fontSize: 14 },
  sectionTitle: { fontSize: 14, fontWeight: '700', color: C.white },
  badge: {
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 8,
    backgroundColor: C.bgInput,
    borderWidth: 1,
    borderColor: C.border,
  },
  badgeText: { fontSize: 10, fontWeight: '700', color: C.slate400 },

  // ── Pending Cards ──
  pendingCard: {
    backgroundColor: '#0f172a66',
    borderWidth: 1,
    borderColor: C.borderLight,
    borderLeftWidth: 3,
    borderLeftColor: C.amber,
    borderRadius: 14,
    paddingHorizontal: 16,
    paddingVertical: 12,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  pendingId: { fontSize: 14, fontWeight: '700', color: C.amber, fontFamily: 'monospace' },
  pendingSub: { fontSize: 10, color: C.slate500, fontWeight: '700', letterSpacing: 1, marginTop: 2 },
  pendingActions: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  approveBtn: {
    paddingHorizontal: 14,
    paddingVertical: 8,
    backgroundColor: C.emerald,
    borderRadius: 12,
  },
  approveBtnText: { fontSize: 11, fontWeight: '700', color: C.white },
  deleteMiniBtn: {
    paddingHorizontal: 10,
    paddingVertical: 8,
    backgroundColor: C.redDim,
    borderWidth: 1,
    borderColor: C.redBorder,
    borderRadius: 12,
  },

  // ── Device Cards ──
  deviceCard: {
    backgroundColor: '#0f172acc',
    borderWidth: 1,
    borderColor: C.border,
    borderRadius: 16,
    padding: 16,
    marginBottom: 12,
    overflow: 'hidden',
  },
  deviceAlert: {
    borderColor: '#ef444466',
    backgroundColor: '#ef44440d',
  },
  deviceOffline: { opacity: 0.5 },
  alertOverlay: {
    ...StyleSheet.absoluteFillObject,
    backgroundColor: '#ef44440a',
  },

  deviceHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
    marginBottom: 12,
  },
  deviceName: { fontSize: 16, fontWeight: '800', color: C.white, letterSpacing: -0.3 },
  deviceId: { fontSize: 10, color: C.slate500, fontWeight: '700', letterSpacing: 1, marginTop: 2 },

  deviceBadge: { paddingHorizontal: 10, paddingVertical: 4, borderRadius: 8 },
  badgeEmergency: { backgroundColor: C.red },
  badgeLinked: { backgroundColor: C.bgInput },
  badgeOff: { backgroundColor: '#ffffff08' },
  deviceBadgeText: { fontSize: 10, fontWeight: '700', letterSpacing: 1 },
  badgeEmergencyText: { color: C.white },
  badgeLinkedText: { color: C.slate500 },
  badgeOffText: { color: C.slate600 },

  // ── Info boxes ──
  infoGrid: { flexDirection: 'row', gap: 8, marginBottom: 12 },
  infoBox: {
    flex: 1,
    backgroundColor: '#ffffff05',
    borderWidth: 1,
    borderColor: C.borderLight,
    borderRadius: 12,
    padding: 10,
  },
  infoLabel: { fontSize: 9, color: C.slate500, fontWeight: '700', letterSpacing: 1, marginBottom: 2 },
  infoVal: { fontSize: 14, fontWeight: '700', color: C.white },

  // ── Action buttons ──
  deviceActions: { flexDirection: 'row', gap: 6, marginBottom: 12 },
  modifyBtn: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 4,
    paddingVertical: 8,
    backgroundColor: C.bgInput,
    borderWidth: 1,
    borderColor: C.border,
    borderRadius: 8,
  },
  modifyBtnText: { fontSize: 10, fontWeight: '700', color: C.slate400 },
  silenceBtn: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 4,
    paddingVertical: 8,
    backgroundColor: '#ef444414',
    borderWidth: 1,
    borderColor: '#ef44441f',
    borderRadius: 8,
  },
  silenceBtnText: { fontSize: 10, fontWeight: '700', color: C.red },
  deleteBtn: {
    paddingHorizontal: 10,
    paddingVertical: 8,
    backgroundColor: C.redDim,
    borderWidth: 1,
    borderColor: C.redBorder,
    borderRadius: 8,
  },

  // ── Device footer ──
  deviceFooter: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingTop: 10,
    borderTopWidth: 1,
    borderTopColor: C.borderLight,
  },
  footerStatus: { flexDirection: 'row', alignItems: 'center', gap: 6 },
  footerDot: { width: 6, height: 6, borderRadius: 3 },
  footerText: { fontSize: 9, fontWeight: '700', color: C.slate500, letterSpacing: 1 },

  // ── Empty state ──
  emptyCard: {
    paddingVertical: 48,
    borderWidth: 1,
    borderStyle: 'dashed',
    borderColor: C.border,
    borderRadius: 16,
    alignItems: 'center',
  },
  emptyEmoji: { fontSize: 32, opacity: 0.3, marginBottom: 10 },
  emptyText: { fontSize: 14, color: C.slate500, textAlign: 'center', lineHeight: 20 },
});
