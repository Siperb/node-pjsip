const api = require('./lib');

// Test the new JavaScript wrapper with client's required API structure
console.log('🚀 Testing PJSIP JavaScript Wrapper (Client Requirements)');

// Set up callbacks using the client's single-assignment callback system
api.setCallbacks({
  onReady: () => console.log('✅ PJSIP Ready'),
  onError: (e) => console.error('❌ PJSIP Error:', e),
  onRegisterStart: (e) => console.log('📝 Registration started:', e),
  onRegisterSuccess: (e) => console.log('✅ Registration successful:', e),
  onRegisterFailed: (e) => console.log('❌ Registration failed:', e),
  onInvite: (e) => console.log('📞 Incoming INVITE:', e),
  onConnected: (e) => console.log('🎉 Call connected:', e),
  onBye: (e) => console.log('📴 Call ended:', e),
});

// Test initialization
console.log('🔧 Initializing PJSIP...');
const initResult = api.Init({ 
  logLevel: 4,
  localPort: 5060,
  transport: 'UDP',
  userAgent: 'PJSIP Node.js Wrapper',
  maxCalls: 10
});

if (!initResult.ok) {
  console.error('❌ Initialization failed:', initResult.message);
  process.exit(1);
}

console.log('✅ Initialization successful:', initResult.data);

// Test account registration
console.log('📝 Registering account...');
const registerResult = api.Register({
  username: "1113",
  password: "JJ5iYgLcermrMu",
  domain: "pbx.sheerbit.com",
  registrar: "sip:pbx.sheerbit.com:5060",
  displayName: "Test User",
  expires: 3600
});

if (!registerResult.ok) {
  console.error('❌ Registration failed:', registerResult.message);
  process.exit(1);
}

console.log('✅ Account registered with ID:', registerResult.data);

// Wait a bit for registration to complete
setTimeout(() => {
  // Test making a call
  console.log('📞 Making INVITE...');
  const inviteResult = api.Invite({
    to: "sip:8080@pbx.sheerbit.com",
    withAudio: true,
    withVideo: false,
    timeout: 30
  });

  if (!inviteResult.ok) {
    console.error('❌ INVITE failed:', inviteResult.message);
  } else {
    console.log('✅ INVITE sent with call ID:', inviteResult.data);
  }

  // Test other functions
  console.log('🔍 Testing utility functions...');
  
  const versionResult = api.GetVersion();
  console.log('📋 Version:', versionResult.ok ? versionResult.data : versionResult.message);
  
  const localIPResult = api.GetLocalIP();
  console.log('🌐 Local IP:', localIPResult.ok ? localIPResult.data : localIPResult.message);
  
  const portResult = api.GetBoundPort();
  console.log('🔌 Bound Port:', portResult.ok ? portResult.data : portResult.message);
  
  const regStatusResult = api.GetRegistrationStatus();
  console.log('📊 Registration Status:', regStatusResult.ok ? regStatusResult.data : regStatusResult.message);

  // Test diagnostics
  console.log('🔧 Binding Info:', api.getBindingInfo());

  // Clean shutdown after 10 seconds
  setTimeout(() => {
    console.log('🛑 Shutting down...');
    const shutdownResult = api.Shutdown();
    if (shutdownResult.ok) {
      console.log('✅ Shutdown successful');
    } else {
      console.error('❌ Shutdown failed:', shutdownResult.message);
    }
    process.exit(0);
  }, 10000);

}, 3000);

// Handle process termination
process.on('SIGINT', () => {
  console.log('\n🛑 Received SIGINT, shutting down...');
  const shutdownResult = api.Shutdown();
  if (shutdownResult.ok) {
    console.log('✅ Shutdown successful');
  } else {
    console.error('❌ Shutdown failed:', shutdownResult.message);
  }
  process.exit(0);
});
