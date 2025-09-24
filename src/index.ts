import { EventEmitter } from 'events';
import { load } from './native';

// Native addon interface
interface NativePJSIP {
  init(): boolean;
  shutdown(): boolean;
  addAccount(aor: string, registrar: string, username: string, password: string, proxy?: string): number;
  removeAccount(accId: number): boolean;
  getAccountInfo(accId: number): AccountInfo | null;
  getAccounts(): AccountInfo[];
  registerAccount(accId: number): boolean;
  unregisterAccount(accId: number): boolean;
  makeCall(accId: number, destination: string): boolean;
  answerCall(accId: number, callId: string, fromTag: string, toTag: string, cseqNum: number): boolean;
  hangupCall(accId: number, callId: string, fromTag: string, toTag: string, cseqNum: number): boolean;
  getVersion(): string;
  getLocalIP(): string;
  getBoundPort(): number;
}

// Account information interface
export interface AccountInfo {
  id: number;
  aor: string;
  registrar: string;
  username: string;
  isRegistered: boolean;
  regState: number;
}

// Call information interface
export interface CallInfo {
  callId: string;
  from: string;
  to: string;
  state: string;
  mediaState: string;
}

// PJSIP wrapper class (similar to baresip-node)
export class PJSIP extends EventEmitter {
  private native: NativePJSIP;
  private isInitialized: boolean = false;

  constructor() {
    super();
    this.native = load();
  }

  /**
   * Initialize PJSIP library
   */
  async init(): Promise<boolean> {
    if (this.isInitialized) {
      return true;
    }

    try {
      const result = this.native.init();
      if (result) {
        this.isInitialized = true;
        this.emit('initialized');
      }
      return result;
    } catch (error) {
      this.emit('error', error);
      return false;
    }
  }

  /**
   * Shutdown PJSIP library
   */
  async shutdown(): Promise<boolean> {
    if (!this.isInitialized) {
      return true;
    }

    try {
      const result = this.native.shutdown();
      if (result) {
        this.isInitialized = false;
        this.emit('shutdown');
      }
      return result;
    } catch (error) {
      this.emit('error', error);
      return false;
    }
  }

  /**
   * Add SIP account
   */
  addAccount(config: {
    aor: string;
    registrar: string;
    username: string;
    password: string;
    proxy?: string;
  }): number {
    if (!this.isInitialized) {
      throw new Error('PJSIP not initialized');
    }

    const accountId = this.native.addAccount(
      config.aor,
      config.registrar,
      config.username,
      config.password,
      config.proxy || ''
    );

    if (accountId >= 0) {
      this.emit('accountAdded', accountId);
    }

    return accountId;
  }

  /**
   * Remove SIP account
   */
  removeAccount(accountId: number): boolean {
    if (!this.isInitialized) {
      return false;
    }

    const result = this.native.removeAccount(accountId);
    if (result) {
      this.emit('accountRemoved', accountId);
    }

    return result;
  }

  /**
   * Get account information
   */
  getAccountInfo(accountId: number): AccountInfo | null {
    if (!this.isInitialized) {
      return null;
    }

    return this.native.getAccountInfo(accountId);
  }

  /**
   * Get all accounts
   */
  getAccounts(): AccountInfo[] {
    if (!this.isInitialized) {
      return [];
    }

    return this.native.getAccounts();
  }

  /**
   * Register account
   */
  registerAccount(accountId: number): boolean {
    if (!this.isInitialized) {
      return false;
    }

    const result = this.native.registerAccount(accountId);
    if (result) {
      this.emit('registering', accountId);
    }

    return result;
  }

  /**
   * Unregister account
   */
  unregisterAccount(accountId: number): boolean {
    if (!this.isInitialized) {
      return false;
    }

    const result = this.native.unregisterAccount(accountId);
    if (result) {
      this.emit('unregistering', accountId);
    }

    return result;
  }

  /**
   * Make a call
   */
  makeCall(accountId: number, destination: string): boolean {
    if (!this.isInitialized) {
      return false;
    }

    const result = this.native.makeCall(accountId, destination);
    if (result) {
      this.emit('callInitiated', accountId, destination);
    }

    return result;
  }

  /**
   * Answer a call
   */
  answerCall(accountId: number, callId: string, fromTag: string, toTag: string, cseqNum: number): boolean {
    if (!this.isInitialized) {
      return false;
    }

    const result = this.native.answerCall(accountId, callId, fromTag, toTag, cseqNum);
    if (result) {
      this.emit('callAnswered', accountId, callId);
    }

    return result;
  }

  /**
   * Hangup a call
   */
  hangupCall(accountId: number, callId: string, fromTag: string, toTag: string, cseqNum: number): boolean {
    if (!this.isInitialized) {
      return false;
    }

    const result = this.native.hangupCall(accountId, callId, fromTag, toTag, cseqNum);
    if (result) {
      this.emit('callHangup', accountId, callId);
    }

    return result;
  }

  /**
   * Get PJSIP version
   */
  getVersion(): string {
    return this.native.getVersion();
  }

  /**
   * Get local IP address
   */
  getLocalIP(): string {
    return this.native.getLocalIP();
  }

  /**
   * Get bound port
   */
  getBoundPort(): number {
    return this.native.getBoundPort();
  }

  /**
   * Check if initialized
   */
  isReady(): boolean {
    return this.isInitialized;
  }
}

// Export the main class
export default PJSIP;

// Legacy exports for backward compatibility
export const { PJSIP: PJSIPWrapper } = { PJSIP };

