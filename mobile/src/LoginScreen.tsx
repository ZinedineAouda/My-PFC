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

type Props = {
  onLogin: () => void;
};

export default function LoginScreen({ onLogin }: Props) {
  const [user, setUser] = useState('');
  const [pass, setPass] = useState('');
  const [loading, setLoading] = useState(false);

  const handleLogin = async () => {
    if (!user || !pass) {
      Alert.alert('Error', 'Please enter username and password');
      return;
    }
    setLoading(true);
    try {
      const res = await fetch(`${API_BASE}/api/admin/login`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        credentials: 'include',
        body: JSON.stringify({ username: user, password: pass }),
      });
      if (!res.ok) throw new Error('Invalid credentials');
      onLogin();
    } catch {
      Alert.alert('Login Failed', 'Invalid credentials');
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
    padding: 40,
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
    marginBottom: 32,
    fontWeight: '700',
    letterSpacing: 2,
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
