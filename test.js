const PJSIP = require('./lib');

// Create PJSIP instance
const pjsip = new PJSIP.PJSIP();

// Real SIP configuration - Replace with your actual SIP details
const sipConfig = {
    aor: "sip:1314@pbx.sheerbit.com",           // Your SIP address
    registrar: "sip:pbx.sheerbit.com:5060",        // Your registrar server
    username: "1314",                     // Your SIP username
    password: "MOPhb9mK4VZn",                     // Your SIP password
    proxy: "sip:devproxy.celloip.com"                                      // Optional proxy server
};

// Event handlers
pjsip.on('initialized', (data) => {
    console.log('✅ PJSIP initialized:', data);
    
    // Add account after initialization
    try {
        const accountId = pjsip.addAccount(sipConfig);
        
        console.log('✅ Account added with ID:', accountId);
        
        // Register the account
        pjsip.registerAccount(accountId);
        
    } catch (error) {
        console.error('❌ Error adding account:', error.message);
    }
});

pjsip.on('accountAdded', (data) => {
    console.log('✅ Account added:', data);
});

pjsip.on('registrationStarted', (data) => {
    console.log('🔄 Registration started for account:', data.accountId);
});

// Add registration success/failure handlers
pjsip.on('registered', (aor) => {
    console.log('✅ Registration successful for:', aor);
});

pjsip.on('registerFailed', (aor) => {
    console.log('❌ Registration failed for:', aor);
});

pjsip.on('unregistered', (aor) => {
    console.log('📤 Unregistered:', aor);
});

pjsip.on('error', (error) => {
    console.error('❌ PJSIP Error:', error);
});

// Initialize PJSIP
console.log('🚀 Initializing PJSIP...');
pjsip.init().then((result) => {
    console.log('✅ Initialization result:', result);
}).catch((error) => {
    console.error('❌ Initialization failed:', error);
});

// Keep the process running to see registration events
setTimeout(() => {
    console.log('\n📊 Current accounts:');
    const accounts = pjsip.getAccounts();
    accounts.forEach(account => {
        console.log(`  Account ${account.id}: ${account.aor} (Registered: ${account.isRegistered})`);
    });
    
    // Get detailed account info
    if (accounts.length > 0) {
        const accountId = accounts[0].id;
        try {
            const info = pjsip.getAccountInfo(accountId);
            console.log('\n📋 Account details:', info);
        } catch (error) {
            console.error('❌ Error getting account info:', error.message);
        }
    }
}, 3000);

// Graceful shutdown after 10 seconds
setTimeout(() => {
    console.log('\n🛑 Shutting down PJSIP...');
    pjsip.shutdown().then((result) => {
        console.log('✅ Shutdown result:', result);
        process.exit(0);
    }).catch((error) => {
        console.error('❌ Shutdown failed:', error);
        process.exit(1);
    });
}, 10000);

// Handle process termination
process.on('SIGINT', () => {
    console.log('\n🛑 Received SIGINT, shutting down...');
    pjsip.shutdown().then(() => {
        process.exit(0);
    });
});

process.on('SIGTERM', () => {
    console.log('\n🛑 Received SIGTERM, shutting down...');
    pjsip.shutdown().then(() => {
        process.exit(0);
    });
});