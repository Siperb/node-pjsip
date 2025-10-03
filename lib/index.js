'use strict';

// ---------------------------
// Binding Loader
// ---------------------------
function loadBinding() {
  const tried = [];
  const override = process.env.PJSIP_NODE_BINDING;
  if (override) {
    try { 
      return require(require('path').resolve(override)); 
    } catch (e) { 
      tried.push(override); 
    }
  }
  
  const path = require('path');
  const triplet = `${process.platform}-${process.arch}`;
  const candidates = [
    path.join(__dirname, '..', 'build', 'Release', `node_pjsip-${triplet}.node`),
    path.join(__dirname, '..', 'build', 'Release', 'node_pjsip.node'),
    path.join(__dirname, '..', 'build', 'Release', 'pjsip-node-mac-arm64.node'), // specific fallback if you ship it
  ];
  
  for (const p of candidates) {
    try { 
      return require(p); 
    } catch (e) { 
      tried.push(p); 
    }
  }
  
  const err = new Error(`Failed to load native binding. Tried: \n- ${tried.join('\n- ')}`);
  err.code = 'PJSIP_BINDING_LOAD_FAILED';
  throw err;
}

const binding = loadBinding();

// ---------------------------
// State & Utilities
// ---------------------------
const state = {
  inited: false,
  callbacks: Object.freeze({}), // replaced by setCallbacks
};

function setCallbacks(cbs) {
  const next = Object.assign({
    onReady: null, onError: null, onTransportUp: null, onTransportDown: null,
    onRegisterStart: null, onRegisterSuccess: null, onRegisterFailed: null, onUnregister: null,
    onInvite: null, onMessage: null, onOptions: null,
    onProgress: null, onEarlyMedia: null, onRinging: null, onConnected: null,
    onReinvite: null, onUpdate: null, onMediaState: null, onHold: null, onUnhold: null,
    onDtmf: null, onInfo: null, onRefer: null, onTransferProgress: null, onTransferComplete: null,
    onBye: null, onFailure: null, onStats: null,
  }, cbs || {});
  state.callbacks = Object.freeze(next);
}

function ensureInited(fnName) {
  if (!state.inited && fnName !== 'Init' && fnName !== 'GetVersion') {
    const e = new Error(`Call Init() before ${fnName}()`);
    e.code = 'PJSIP_NOT_INITIALIZED';
    throw e;
  }
}

function toJson(obj) {
  try { 
    return obj ? JSON.stringify(obj) : '{}'; 
  } catch (e) {
    const err = new Error('Payload must be JSON serializable');
    err.code = 'PJSIP_JSON_ENCODE_ERROR';
    throw err;
  }
}

function normalize(result) {
  if (!result || typeof result.ok !== 'boolean') {
    return { ok: false, code: -1, message: 'Malformed native result' };
  }
  if (!result.ok) {
    return { ok: false, code: result.code || -1, message: result.message || 'Native error' };
  }
  let data = result.data;
  if (typeof data === 'string') {
    try { 
      data = JSON.parse(data); 
    } catch (_) {
      // Keep as string if not valid JSON
    }
  }
  return { ok: true, code: 0, data };
}

// ---------------------------
// Native Event Bridge (single hook)
// ---------------------------
if (typeof binding.SetEventCallback === 'function') {
  binding.SetEventCallback((name, json) => {
    let payload = undefined;
    if (json && typeof json === 'string') {
      try { 
        payload = JSON.parse(json); 
      } catch (e) {
        state.callbacks.onError && state.callbacks.onError({ 
          code: -2, 
          message: 'Event JSON parse error', 
          data: { name, json } 
        });
        return;
      }
    }
    
    const cbMap = {
      OnReady: 'onReady', 
      OnError: 'onError',
      OnTransportUp: 'onTransportUp', 
      OnTransportDown: 'onTransportDown',
      OnRegisterStart: 'onRegisterStart', 
      OnRegisterSuccess: 'onRegisterSuccess', 
      OnRegisterFailed: 'onRegisterFailed', 
      OnUnregister: 'onUnregister',
      OnInvite: 'onInvite', 
      OnMessage: 'onMessage', 
      OnOptions: 'onOptions',
      OnProgress: 'onProgress', 
      OnEarlyMedia: 'onEarlyMedia', 
      OnRinging: 'onRinging', 
      OnConnected: 'onConnected',
      OnReinvite: 'onReinvite', 
      OnUpdate: 'onUpdate', 
      OnMediaState: 'onMediaState', 
      OnHold: 'onHold', 
      OnUnhold: 'onUnhold',
      OnDtmf: 'onDtmf', 
      OnInfo: 'onInfo', 
      OnRefer: 'onRefer', 
      OnTransferProgress: 'onTransferProgress', 
      OnTransferComplete: 'onTransferComplete',
      OnBye: 'onBye', 
      OnFailure: 'onFailure', 
      OnStats: 'onStats',
    };
    
    const key = cbMap[name];
    const fn = key && state.callbacks[key];
    if (typeof fn === 'function') {
      try { 
        fn(payload); 
      } catch (e) { 
        // swallow callback errors to prevent crashes
      }
    }
  });
}

// ---------------------------
// Thin wrappers (JS -> Native)
// ---------------------------
function callNative(name, payload) {
  ensureInited(name);
  const fn = binding[name];
  if (typeof fn !== 'function') {
    return { ok: false, code: -3, message: `Native method ${name} not found` };
  }
  try { 
    // Pass the payload directly to native functions, not as JSON string
    return normalize(fn(payload)); 
  } catch (e) {
    return { ok: false, code: -4, message: e && e.message || 'Native call failed' };
  }
}

// ---------------------------
// Core Functions
// ---------------------------
function Init(config) {
  const res = callNative('Init', config);
  if (res.ok) state.inited = true;
  return res;
}

function Shutdown() {
  const res = callNative('Shutdown');
  state.inited = false;
  setCallbacks(null);
  return res;
}

function SetLogLevel(level) { 
  return callNative('SetLogLevel', { level }); 
}

function GetVersion() { 
  return callNative('GetVersion'); 
}

// ---------------------------
// Account / Registration Functions
// ---------------------------
function Register(cfg) { 
  return callNative('Register', cfg); 
}

function Unregister() { 
  return callNative('Unregister'); 
}

function GetRegistrationStatus() { 
  return callNative('GetRegistrationStatus'); 
}

// ---------------------------
// Call Functions (UAC + UAS)
// ---------------------------
function Invite(cfg) { 
  return callNative('Invite', cfg); 
}

function Answer(cfg) { 
  return callNative('Answer', cfg); 
}

function Reject(cfg) { 
  return callNative('Reject', cfg); 
}

function Cancel(cfg) { 
  return callNative('Cancel', cfg); 
}

function Bye(cfg) { 
  return callNative('Bye', cfg); 
}

function Hold(cfg) { 
  return callNative('Hold', cfg); 
}

function Unhold(cfg) { 
  return callNative('Unhold', cfg); 
}

function Reinvite(cfg) { 
  return callNative('Reinvite', cfg); 
}

function Update(cfg) { 
  return callNative('Update', cfg); 
}

function Mute(cfg) { 
  return callNative('Mute', cfg); 
}

function Unmute(cfg) { 
  return callNative('Unmute', cfg); 
}

function Dtmf(cfg) { 
  return callNative('Dtmf', cfg); 
}

function Info(cfg) { 
  return callNative('Info', cfg); 
}

function GetCallInfo(cfg) { 
  return callNative('GetCallInfo', cfg); 
}

function GetMediaStats(cfg) { 
  return callNative('GetMediaStats', cfg); 
}

// ---------------------------
// Messaging & Presence Functions
// ---------------------------
function SendMessage(cfg) { 
  return callNative('SendMessage', cfg); 
}

function Subscribe(cfg) { 
  return callNative('Subscribe', cfg); 
}

function Unsubscribe(cfg) { 
  return callNative('Unsubscribe', cfg); 
}

function Publish(cfg) { 
  return callNative('Publish', cfg); 
}

// ---------------------------
// Conference / Utility Functions
// ---------------------------
function Conference(cfg) { 
  return callNative('Conference', cfg); 
}

function ListTransports() { 
  return callNative('ListTransports'); 
}

function ListCalls() { 
  return callNative('ListCalls'); 
}

function SetCodecPriority(cfg) { 
  return callNative('SetCodecPriority', cfg); 
}

// ---------------------------
// Transport & Network Functions (placeholder implementations)
// ---------------------------
function CreateTransport(cfg) { 
  return callNative('CreateTransport', cfg); 
}

function SetOutboundProxy(cfg) { 
  return callNative('SetOutboundProxy', cfg); 
}

function SetDnsServers(cfg) { 
  return callNative('SetDnsServers', cfg); 
}

function SetStunServers(cfg) { 
  return callNative('SetStunServers', cfg); 
}

function SetTurnServers(cfg) { 
  return callNative('SetTurnServers', cfg); 
}

// ---------------------------
// Additional utility functions for compatibility
// ---------------------------
function GetLocalIP() { 
  return callNative('GetLocalIP'); 
}

function GetBoundPort() { 
  return callNative('GetBoundPort'); 
}

// ---------------------------
// Module Exports
// ---------------------------
module.exports = {
  // lifecycle & config
  Init, 
  Shutdown, 
  SetLogLevel, 
  GetVersion,
  
  // transport & network
  CreateTransport, 
  SetOutboundProxy, 
  SetDnsServers, 
  SetStunServers, 
  SetTurnServers,
  
  // account
  Register, 
  Unregister, 
  GetRegistrationStatus,
  
  // calls
  Invite, 
  Answer, 
  Reject, 
  Cancel, 
  Bye, 
  Hold, 
  Unhold, 
  Reinvite, 
  Update, 
  Mute, 
  Unmute, 
  Dtmf, 
  Info, 
  GetCallInfo, 
  GetMediaStats,
  
  // messaging & presence
  SendMessage, 
  Subscribe, 
  Unsubscribe, 
  Publish,
  
  // conference & utils
  Conference, 
  ListTransports, 
  ListCalls, 
  SetCodecPriority,
  
  // additional utilities
  GetLocalIP,
  GetBoundPort,
  
  // callbacks
  setCallbacks,
  
  // diagnostics
  getBindingInfo: () => ({
    platform: process.platform,
    arch: process.arch,
    triplet: `${process.platform}-${process.arch}`,
    inited: state.inited,
    callbacksCount: Object.keys(state.callbacks).filter(k => state.callbacks[k]).length
  })
};
