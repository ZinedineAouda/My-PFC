import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, ActivityIndicator } from 'react-native';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { StatusBar } from 'expo-status-bar';
import LoginScreen from './src/LoginScreen';
import DashboardScreen from './src/DashboardScreen';
import { API_BASE, C } from './src/constants';

export default function App() {
  const [loggedIn, setLoggedIn] = useState(false);
  const [checking, setChecking] = useState(true);
  const [baseUrl, setBaseUrl] = useState(API_BASE);

  // Check existing session on mount
  useEffect(() => {
    const checkSession = async () => {
      try {
        const res = await fetch(`${baseUrl}/api/admin/session`, {
          headers: { 'X-Admin-Token': 'admin1234' },
          credentials: 'include',
        });
        if (res.ok) {
          const data = await res.json();
          if (data.authenticated) setLoggedIn(true);
        }
      } catch (e) {
        console.log('Session check failed', e);
      } finally {
        setChecking(false);
      }
    };
    checkSession();
  }, [baseUrl]);

  if (checking) {
    return (
      <View style={s.loading}>
        <ActivityIndicator size="large" color={C.emerald} />
        <Text style={s.loadingText}>STABILIZING_HANDSHAKE...</Text>
      </View>
    );
  }

  const handleLogin = (url: string) => {
    setBaseUrl(url);
    setLoggedIn(true);
  };

  return (
    <SafeAreaProvider>
      <StatusBar style="light" backgroundColor={C.bg} />
      {loggedIn ? (
        <DashboardScreen 
          baseUrl={baseUrl} 
          onLogout={() => setLoggedIn(false)} 
        />
      ) : (
        <LoginScreen onLogin={handleLogin} />
      )}
    </SafeAreaProvider>
  );
}

const s = StyleSheet.create({
  loading: {
    flex: 1,
    backgroundColor: C.bg,
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    color: C.emerald,
    fontFamily: 'monospace',
    fontSize: 14,
    marginTop: 16,
  },
});
