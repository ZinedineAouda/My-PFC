import React, { useState } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  ActivityIndicator,
  Alert,
  KeyboardAvoidingView,
  Platform,
} from 'react-native';
import { MaterialCommunityIcons } from '@expo/vector-icons';
import { C, API_BASE } from './constants';

export default function LoginScreen({ onLogin }: Props) {
  const [user, setUser] = useState('');
  const [pass, setPass] = useState('');
  const [server, setServer] = useState(API_BASE); // Default from constants
  const [loading, setLoading] = useState(false);
  const [showServer, setShowServer] = useState(false);

  const handleLogin = async () => {
    if (!user || !pass) {
      Alert.alert('Error', 'Please enter username and password');
      return;
    }
    setLoading(true);
    try {
      const baseUrl = server.endsWith('/') ? server.slice(0, -1) : server;
      const res = await fetch(`${baseUrl}/api/admin/login`, {
        method: 'POST',
        headers: { 
          'Content-Type': 'application/json',
          'X-Admin-Token': 'admin1234'
        },
        credentials: 'include',
        body: JSON.stringify({ username: user, password: pass }),
      });
      if (!res.ok) throw new Error('Invalid credentials');
      onLogin(baseUrl); 
    } catch (e: any) {
      Alert.alert('Network Error', 'Could not reach server. Check URL and connection.');
    } finally {
      setLoading(false);
    }
  };

  return (
    <KeyboardAvoidingView
      style={s.container}
      behavior={Platform.OS === 'ios' ? 'padding' : undefined}
    >
      <View style={s.card}>
        {/* Icon */}
        <View style={s.iconWrap}>
          <MaterialCommunityIcons name="shield-check" size={32} color={C.emerald} />
        </View>

        {/* Title */}
        <Text style={s.title}>Security Command</Text>
        <Text style={s.subtitle}>ADMINISTRATOR ACCESS</Text>

        {/* Server Config Toggle */}
        <TouchableOpacity 
          style={s.serverToggle} 
          onPress={() => setShowServer(!showServer)}
          activeOpacity={0.6}
        >
          <MaterialCommunityIcons name="server-network" size={14} color={showServer ? C.emerald : C.slate500} />
          <Text style={[s.serverToggleText, showServer && {color: C.emerald}]}>
            {showServer ? 'HIDE SERVER SETTINGS' : 'CONFIGURE SERVER'}
          </Text>
        </TouchableOpacity>

        {showServer && (
          <View style={s.serverBox}>
            <Text style={s.label}>API ENDPOINT</Text>
            <TextInput
              style={s.input}
              value={server}
              onChangeText={setServer}
              placeholder="https://..."
              placeholderTextColor={C.slate600}
              autoCapitalize="none"
              autoCorrect={false}
            />
          </View>
        )}

        {/* Username */}
        <Text style={s.label}>USERNAME</Text>
        <TextInput
          style={s.input}
          value={user}
          onChangeText={setUser}
          placeholder="admin"
          placeholderTextColor={C.slate600}
          autoCapitalize="none"
          autoCorrect={false}
        />

        {/* Password */}
        <Text style={s.label}>PASSWORD</Text>
        <TextInput
          style={s.input}
          value={pass}
          onChangeText={setPass}
          placeholder="••••••••"
          placeholderTextColor={C.slate600}
          secureTextEntry
        />

        {/* Submit */}
        <TouchableOpacity
          style={[s.button, loading && s.buttonDisabled]}
          onPress={handleLogin}
          disabled={loading}
          activeOpacity={0.8}
        >
          {loading ? (
            <ActivityIndicator color={C.white} size="small" />
          ) : (
            <View style={s.buttonContent}>
              <Text style={s.buttonText}>Establish Secure Link</Text>
              <MaterialCommunityIcons name="login" size={16} color={C.white} />
            </View>
          )}
        </TouchableOpacity>
      </View>
    </KeyboardAvoidingView>
  );
}

const s = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: C.bg,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 20,
  },
  card: {
    width: '100%',
    maxWidth: 420,
    backgroundColor: '#0f172acc',
    borderWidth: 1,
    borderColor: C.border,
    borderRadius: 24,
    paddingHorizontal: 32,
    paddingVertical: 40,
  },
  iconWrap: {
    width: 64,
    height: 64,
    backgroundColor: C.emeraldDim,
    borderWidth: 1,
    borderColor: C.emeraldBorder,
    borderRadius: 18,
    alignItems: 'center',
    justifyContent: 'center',
    alignSelf: 'center',
    marginBottom: 12,
  },
  title: {
    fontSize: 24,
    fontWeight: '800',
    color: C.white,
    textAlign: 'center',
    letterSpacing: -0.5,
  },
  subtitle: {
    fontSize: 11,
    color: '#10b98199',
    textAlign: 'center',
    marginTop: 4,
    marginBottom: 24,
    fontWeight: '700',
    letterSpacing: 2,
  },
  serverToggle: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    marginBottom: 16,
    padding: 8,
    backgroundColor: '#ffffff05',
    borderRadius: 10,
    borderWidth: 1,
    borderColor: C.borderLight,
  },
  serverToggleText: {
    fontSize: 9,
    fontWeight: '800',
    color: C.slate500,
    letterSpacing: 1,
  },
  serverBox: {
    marginBottom: 16,
    padding: 12,
    backgroundColor: '#ffffff03',
    borderRadius: 12,
    borderWidth: 1,
    borderColor: '#ffffff0a',
  },
  label: {
    fontSize: 10,
    color: C.slate500,
    fontWeight: '700',
    letterSpacing: 1.5,
    marginBottom: 4,
  },
  input: {
    width: '100%',
    paddingHorizontal: 14,
    paddingVertical: 12,
    backgroundColor: C.bgInput,
    borderWidth: 1,
    borderColor: C.border,
    borderRadius: 12,
    color: C.white,
    fontSize: 14,
    marginBottom: 12,
  },
  button: {
    width: '100%',
    paddingVertical: 14,
    backgroundColor: C.emerald,
    borderRadius: 12,
    alignItems: 'center',
    justifyContent: 'center',
    marginTop: 8,
  },
  buttonDisabled: {
    opacity: 0.3,
  },
  buttonContent: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  buttonText: {
    color: C.white,
    fontSize: 14,
    fontWeight: '700',
  },
});
