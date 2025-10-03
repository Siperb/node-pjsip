#ifndef NODE_PJSIP_WRAPPER_H
#define NODE_PJSIP_WRAPPER_H

#include <napi.h>

// Real PJSIP includes
#include <pjsua-lib/pjsua.h>
#include <pjlib-util.h>
#include <pjlib.h>

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <mutex>

// Forward declarations
class PJSIPWrapper;

// PJSIP Account class - uses real PJSIP types
class PJSIPAccount {
public:
    pjsua_acc_id acc_id;
    std::string aor;
    std::string registrar;
    std::string username;
    std::string password;
    std::string proxy;
    bool is_registered;
    bool is_unregistering;
    pjsua_acc_info acc_info;
    
    PJSIPAccount();
    ~PJSIPAccount();
};

// PJSIP Wrapper class - uses real PJSIP API
class PJSIPWrapper {
private:
    static PJSIPWrapper* instance;
    bool is_initialized;
    std::vector<std::unique_ptr<PJSIPAccount>> accounts;
    pjsua_acc_id next_account_id;
    std::mutex accounts_mutex;
    
    // PJSIP configuration
    pjsua_config ua_cfg;
    pjsua_logging_config log_cfg;
    pjsua_media_config media_cfg;
    pjsua_transport_config udp_cfg;
    pjsua_transport_id transport_id;
    
    // Event callbacks
    std::function<void(const std::string&)> on_registered;
    std::function<void(const std::string&)> on_register_failed;
    std::function<void(const std::string&)> on_unregistered;
    std::function<void(const std::string&)> on_incoming_call;
    std::function<void(const std::string&)> on_call_state;

    // N-API Thread-safe functions
    Napi::ThreadSafeFunction on_registered_tsfn;
    Napi::ThreadSafeFunction on_register_failed_tsfn;
    Napi::ThreadSafeFunction on_unregistered_tsfn;
    Napi::ThreadSafeFunction on_incoming_call_tsfn;
    Napi::ThreadSafeFunction on_call_state_tsfn;
    
public:
    PJSIPWrapper();
    ~PJSIPWrapper();
    
    static PJSIPWrapper* getInstance();
    
    // Core functions - Real PJSIP API
    bool initialize(const Napi::Object& config);
    bool shutdown();
    
    // Account management - Real PJSIP API
    int registerAccount(const Napi::Object& config);
    bool unregisterAccount(int acc_id);
    PJSIPAccount* getAccount(int acc_id);
    std::vector<PJSIPAccount*> getAllAccounts();
    
    // Session management - Real PJSIP API
    int invite(const Napi::Object& config);
    bool answer(int call_id);
    bool hangup(int call_id);
    
    // Event handlers - Real PJSIP API
    void setOnRegistered(std::function<void(const std::string&)> callback);
    void setOnRegisterFailed(std::function<void(const std::string&)> callback);
    void setOnUnregistered(std::function<void(const std::string&)> callback);
    void setOnIncomingCall(std::function<void(const std::string&)> callback);
    void setOnCallState(std::function<void(const std::string&)> callback);

    // N-API Thread-safe functions
    void setOnRegistered(const Napi::Function& callback);
    void setOnRegisterFailed(const Napi::Function& callback);
    void setOnUnregistered(const Napi::Function& callback);
    void setOnIncomingCall(const Napi::Function& callback);
    void setOnCallState(const Napi::Function& callback);
    
    // Utility functions
    std::string getVersion();
    std::string getLocalIP();
    int getBoundPort();
    
    // PJSIP callback handlers (static)
    static void pjsip_on_reg_state(pjsua_acc_id acc_id);
    static void pjsip_on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);
    static void pjsip_on_call_state(pjsua_call_id call_id, pjsip_event *e);
    static void pjsip_on_call_media_state(pjsua_call_id call_id);
};

// N-API function declarations
// Core functions
Napi::Value Init(const Napi::CallbackInfo& info);
Napi::Value Shutdown(const Napi::CallbackInfo& info);
Napi::Value SetLogLevel(const Napi::CallbackInfo& info);
Napi::Value GetVersion(const Napi::CallbackInfo& info);

// Account / Registration functions
Napi::Value Register(const Napi::CallbackInfo& info);
Napi::Value Unregister(const Napi::CallbackInfo& info);
Napi::Value GetRegistrationStatus(const Napi::CallbackInfo& info);

// Call functions
Napi::Value Invite(const Napi::CallbackInfo& info);
Napi::Value Answer(const Napi::CallbackInfo& info);
Napi::Value Reject(const Napi::CallbackInfo& info);
Napi::Value Cancel(const Napi::CallbackInfo& info);
Napi::Value Bye(const Napi::CallbackInfo& info);
Napi::Value Hold(const Napi::CallbackInfo& info);
Napi::Value Unhold(const Napi::CallbackInfo& info);
Napi::Value Reinvite(const Napi::CallbackInfo& info);
Napi::Value Update(const Napi::CallbackInfo& info);
Napi::Value Mute(const Napi::CallbackInfo& info);
Napi::Value Unmute(const Napi::CallbackInfo& info);
Napi::Value Dtmf(const Napi::CallbackInfo& info);
Napi::Value Info(const Napi::CallbackInfo& info);
Napi::Value GetCallInfo(const Napi::CallbackInfo& info);
Napi::Value GetMediaStats(const Napi::CallbackInfo& info);

// Messaging & Presence functions
Napi::Value SendMessage(const Napi::CallbackInfo& info);
Napi::Value Subscribe(const Napi::CallbackInfo& info);
Napi::Value Unsubscribe(const Napi::CallbackInfo& info);
Napi::Value Publish(const Napi::CallbackInfo& info);

// Conference / Utility functions
Napi::Value Conference(const Napi::CallbackInfo& info);
Napi::Value ListTransports(const Napi::CallbackInfo& info);
Napi::Value ListCalls(const Napi::CallbackInfo& info);
Napi::Value SetCodecPriority(const Napi::CallbackInfo& info);

// Transport & Network functions
Napi::Value CreateTransport(const Napi::CallbackInfo& info);
Napi::Value SetOutboundProxy(const Napi::CallbackInfo& info);
Napi::Value SetDnsServers(const Napi::CallbackInfo& info);
Napi::Value SetStunServers(const Napi::CallbackInfo& info);
Napi::Value SetTurnServers(const Napi::CallbackInfo& info);

// Additional utility functions
Napi::Value GetLocalIP(const Napi::CallbackInfo& info);
Napi::Value GetBoundPort(const Napi::CallbackInfo& info);

// Event callback system
Napi::Value SetEventCallback(const Napi::CallbackInfo& info);

// Export bindings to Node.js
Napi::Object InitPjsipWrapper(Napi::Env env, Napi::Object exports);

#endif