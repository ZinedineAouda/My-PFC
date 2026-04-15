import pg from 'pg';
const { Client } = pg;

async function main() {
  const c = new Client(process.env.DATABASE_URL);
  await c.connect();
  
  // Seed the system_settings row if it doesn't exist
  const result = await c.query(
    `INSERT INTO system_settings (id, wifi_mode) VALUES (1, 4) ON CONFLICT (id) DO NOTHING RETURNING *`
  );
  
  if (result.rows.length > 0) {
    console.log('CREATED system_settings row:', JSON.stringify(result.rows[0]));
  } else {
    console.log('system_settings row already exists');
  }

  // Verify
  const check = await c.query("SELECT * FROM system_settings");
  console.log('SETTINGS NOW:', JSON.stringify(check.rows, null, 2));

  await c.end();
}

main().catch(e => { console.error('ERROR:', e.message); process.exit(1); });
