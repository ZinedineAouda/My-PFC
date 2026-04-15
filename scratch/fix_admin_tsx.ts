import { readFileSync, writeFileSync } from 'fs';

const filePath = 'c:/Users/zined/Documents/GitHub/My-PFC/client/src/pages/admin.tsx';
const content = readFileSync(filePath);

// Convert to string and replace common corruption patterns
let text = content.toString('utf8');

// The specific corruption around line 188
const badPart = /}\s*0 text-white font-bold rounded-2xl shadow-lg shadow-blue-500\/30">[\s\S]*?function LoginForm\(\{ onLogin \}: \{ onLogin: \(\) => void \}\) \{/g;

const replacement = `}

// ═════════════════════════════════════════════════════════════
//  LOGIN FORM — Light Mode
// ═════════════════════════════════════════════════════════════
function LoginForm({ onLogin }: { onLogin: () => void }) {`;

text = text.replace(badPart, replacement);

// Clean up non-printable characters or weird gems
text = text.replace(/[^\x00-\x7F]/g, (char) => {
    // Keep some valid Unicode if needed, but here we probably only need ASCII for TSX + some box drawing
    if (char === '═') return char;
    return ''; 
});

writeFileSync(filePath, text, 'utf8');
console.log('Fixed admin.tsx');
