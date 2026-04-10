import type { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
  appId: 'com.pfc.alarm',
  appName: 'PFC Hospital Alarm',
  webDir: 'dist/public',
  server: {
    // Point the WebView directly at the live Railway server.
    // This eliminates the need for fresh local web builds — the APK
    // always shows the exact same UI as the website.
    url: 'https://my-pfc-production.up.railway.app',
    cleartext: true,
    allowNavigation: ['my-pfc-production.up.railway.app'],
  },
  plugins: {
    LocalNotifications: {
      smallIcon: 'ic_launcher',
      iconColor: '#10b981',
    },
  },
};

export default config;
