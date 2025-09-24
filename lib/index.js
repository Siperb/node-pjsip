
const addon = require('../build/Release/node_pjsip.node');
const EventEmitter = require('events');

class PJSIP extends EventEmitter {
    constructor() {
        super();
        this.accounts = new Map();
        this.isInitialized = false;
    }

    // Initialize PJSIP stack
    init() {
        if (this.isInitialized) {
            return Promise.resolve("PJSIP already initialized");
        }

        try {
            const result = addon.Init();
            this.isInitialized = true;
            this.emit('initialized', result);
            return Promise.resolve(result);
        } catch (error) {
            this.emit('error', error);
            return Promise.reject(error);
        }
    }

    // Add a SIP account
    addAccount(config) {
        if (!this.isInitialized) {
            throw new Error("PJSIP not initialized. Call init() first.");
        }

        const requiredFields = ['aor', 'registrar', 'username', 'password'];
        for (const field of requiredFields) {
            if (!config[field]) {
                throw new Error(`Missing required field: ${field}`);
            }
        }

        try {
            const accountId = addon.addAccount(config);
            if (typeof accountId === 'number' && accountId >= 0) {
                this.accounts.set(accountId, {
                    id: accountId,
                    aor: config.aor,
                    registrar: config.registrar,
                    username: config.username,
                    proxy: config.proxy || '',
                    isRegistered: false
                });
                this.emit('accountAdded', { accountId, config });
                return accountId;
            } else {
                throw new Error("Failed to add account");
            }
        } catch (error) {
            this.emit('error', error);
            throw error;
        }
    }

    // Register an account
    registerAccount(accountId) {
        if (!this.accounts.has(accountId)) {
            throw new Error(`Account ${accountId} not found`);
        }

        try {
            const result = addon.registerAccount(accountId);
            this.emit('registrationStarted', { accountId });
            return Promise.resolve(result);
        } catch (error) {
            this.emit('error', error);
            return Promise.reject(error);
        }
    }

    // Unregister an account
    unregisterAccount(accountId) {
        if (!this.accounts.has(accountId)) {
            throw new Error(`Account ${accountId} not found`);
        }

        try {
            const result = addon.unregisterAccount(accountId);
            this.emit('unregistrationStarted', { accountId });
            return Promise.resolve(result);
        } catch (error) {
            this.emit('error', error);
            return Promise.reject(error);
        }
    }

    // Get account information
    getAccountInfo(accountId) {
        if (!this.accounts.has(accountId)) {
            throw new Error(`Account ${accountId} not found`);
        }

        try {
            const info = addon.getAccountInfo(accountId);
            if (typeof info === 'object') {
                // Update local account state
                const account = this.accounts.get(accountId);
                if (account) {
                    account.isRegistered = info.is_registered;
                }
                return info;
            } else {
                throw new Error("Failed to get account info");
            }
        } catch (error) {
            this.emit('error', error);
            throw error;
        }
    }

    // Remove an account
    removeAccount(accountId) {
        if (!this.accounts.has(accountId)) {
            throw new Error(`Account ${accountId} not found`);
        }

        try {
            const result = addon.removeAccount(accountId);
            this.accounts.delete(accountId);
            this.emit('accountRemoved', { accountId });
            return result;
        } catch (error) {
            this.emit('error', error);
            throw error;
        }
    }

    // Shutdown PJSIP stack
    shutdown() {
        if (!this.isInitialized) {
            return Promise.resolve("PJSIP not initialized");
        }

        try {
            const result = addon.shutdown();
            this.accounts.clear();
            this.isInitialized = false;
            this.emit('shutdown', result);
            return Promise.resolve(result);
        } catch (error) {
            this.emit('error', error);
            return Promise.reject(error);
        }
    }

    // Get all accounts
    getAccounts() {
        return Array.from(this.accounts.values());
    }

    // Check if account is registered
    isAccountRegistered(accountId) {
        const account = this.accounts.get(accountId);
        return account ? account.isRegistered : false;
    }
}

// Create singleton instance
const pjsip = new PJSIP();

// Export both the class and singleton instance
module.exports = {
    PJSIP,
    default: pjsip,
    // Legacy exports for backward compatibility
    Init: () => pjsip.init(),
    RegisterAccount: (config) => {
        const accountId = pjsip.addAccount(config);
        return pjsip.registerAccount(accountId);
    },
    Shutdown: () => pjsip.shutdown(),
    // New PJSIP-like API
    addAccount: (config) => pjsip.addAccount(config),
    registerAccount: (accountId) => pjsip.registerAccount(accountId),
    unregisterAccount: (accountId) => pjsip.unregisterAccount(accountId),
    getAccountInfo: (accountId) => pjsip.getAccountInfo(accountId),
    removeAccount: (accountId) => pjsip.removeAccount(accountId),
    getAccounts: () => pjsip.getAccounts(),
    isAccountRegistered: (accountId) => pjsip.isAccountRegistered(accountId)
};
