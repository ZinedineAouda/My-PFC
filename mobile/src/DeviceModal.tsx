import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  Modal,
  Alert,
  KeyboardAvoidingView,
  Platform,
} from 'react-native';
import { C } from './constants';

type Props = {
  visible: boolean;
  title: string;
  onClose: () => void;
  onSubmit: (name: string, bed: string, room: string) => void;
  loading: boolean;
  defaults?: { patientName?: string; bed?: string; room?: string };
};

export default function DeviceModal({ visible, title, onClose, onSubmit, loading, defaults }: Props) {
  const [name, setName] = useState('');
  const [bed, setBed] = useState('');
  const [room, setRoom] = useState('');

  useEffect(() => {
    if (visible) {
      setName(defaults?.patientName || '');
      setBed(defaults?.bed || '');
      setRoom(defaults?.room || '');
    }
  }, [visible, defaults]);

  const handleSubmit = () => {
    if (!name || !bed || !room) {
      Alert.alert('Error', 'All fields are required');
      return;
    }
    onSubmit(name, bed, room);
  };

  return (
    <Modal visible={visible} transparent animationType="fade" onRequestClose={onClose}>
      <KeyboardAvoidingView
        style={s.overlay}
        behavior={Platform.OS === 'ios' ? 'padding' : undefined}
      >
        <TouchableOpacity style={s.overlay} activeOpacity={1} onPress={onClose}>
          <View style={s.card} onStartShouldSetResponder={() => true}>
            <Text style={s.title}>{title}</Text>

            <Text style={s.label}>SUBJECT NAME</Text>
            <TextInput
              style={s.input}
              value={name}
              onChangeText={setName}
              placeholder="e.g. John Doe"
              placeholderTextColor={C.slate600}
              autoFocus
            />

            <View style={s.row}>
              <View style={s.half}>
                <Text style={s.label}>BED</Text>
                <TextInput
                  style={s.input}
                  value={bed}
                  onChangeText={setBed}
                  placeholder="e.g. B-12"
                  placeholderTextColor={C.slate600}
                />
              </View>
              <View style={s.half}>
                <Text style={s.label}>ROOM</Text>
                <TextInput
                  style={s.input}
                  value={room}
                  onChangeText={setRoom}
                  placeholder="e.g. ICU-3"
                  placeholderTextColor={C.slate600}
                />
              </View>
            </View>

            <View style={s.actions}>
              <TouchableOpacity
                style={[s.btnSubmit, loading && s.disabled]}
                onPress={handleSubmit}
                disabled={loading}
                activeOpacity={0.8}
              >
                <Text style={s.btnSubmitText}>{loading ? 'Saving...' : 'Activate Device'}</Text>
              </TouchableOpacity>
              <TouchableOpacity style={s.btnCancel} onPress={onClose} activeOpacity={0.8}>
                <Text style={s.btnCancelText}>Cancel</Text>
              </TouchableOpacity>
            </View>
          </View>
        </TouchableOpacity>
      </KeyboardAvoidingView>
    </Modal>
  );
}

const s = StyleSheet.create({
  overlay: {
    flex: 1,
    backgroundColor: '#000000b3',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 16,
  },
  card: {
    width: '100%',
    maxWidth: 400,
    backgroundColor: '#0f172af2',
    borderWidth: 1,
    borderColor: C.border,
    borderRadius: 20,
    padding: 32,
  },
  title: {
    fontSize: 18,
    fontWeight: '800',
    color: C.white,
    marginBottom: 20,
  },
  label: {
    fontSize: 9,
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
  row: {
    flexDirection: 'row',
    gap: 12,
  },
  half: {
    flex: 1,
  },
  actions: {
    flexDirection: 'row',
    gap: 8,
    marginTop: 8,
  },
  btnSubmit: {
    flex: 1,
    paddingVertical: 12,
    backgroundColor: C.emerald,
    borderRadius: 12,
    alignItems: 'center',
  },
  disabled: {
    opacity: 0.3,
  },
  btnSubmitText: {
    color: C.white,
    fontSize: 14,
    fontWeight: '700',
  },
  btnCancel: {
    flex: 1,
    paddingVertical: 12,
    backgroundColor: C.bgInput,
    borderWidth: 1,
    borderColor: C.border,
    borderRadius: 12,
    alignItems: 'center',
  },
  btnCancelText: {
    color: C.slate400,
    fontSize: 14,
    fontWeight: '700',
  },
});
