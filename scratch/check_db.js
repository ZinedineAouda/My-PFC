import pg from 'pg';
const { Client } = pg;

async function main() {
  const c = new Client(process.env.DATABASE_URL);
  await c.connect();
  
  const res = await c.query("SELECT * FROM system_settings");
  console.log('SETTINGS:', JSON.stringify(res.rows, null, 2));
  
  const now = Date.now();
  const lastSeen = res.rows[0]?.master_last_seen;
  console.log(`\nCurrent time: ${now}`);
  console.log(`Last seen:    ${lastSeen}`);
  console.log(`Difference:   ${lastSeen ? now - Number(lastSeen) : 'N/A'}ms`);
  console.log(`Online?:      ${lastSeen ? (now - Number(lastSeen) < 60000) : false}`);

  const slavesRes = await c.query("SELECT * FROM slaves");
  console.log(`\nSLAVES (${slavesRes.rows.length}):`, JSON.stringify(slavesRes.rows, null, 2));

  await c.end();
}

main().catch(e => { console.error('ERROR:', e.message); process.exit(1); });
