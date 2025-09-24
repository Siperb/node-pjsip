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
    
public:
    PJSIPWrapper();
    ~PJSIPWrapper();
    
    static PJSIPWrapper* getInstance();
    
    // Core functions - Real PJSIP API
    bool initialize();
    bool shutdown();
    
    // Account management - Real PJSIP API
    int addAccount(const std::string& aor, const std::string& registrar, 
                   const std::string& username, const std::string& password,
                   const std::string& proxy = "");
    bool removeAccount(int acc_id);
    PJSIPAccount* getAccount(int acc_id);
    std::vector<PJSIPAccount*> getAllAccounts();
    
    // Registration - Real PJSIP API
    bool registerAccount(int acc_id);
    bool unregisterAccount(int acc_id);
    bool refreshRegistration(int acc_id);
    
    // Call management - Real PJSIP API
    bool makeCall(int acc_id, const std::string& uri);
    bool answerCall(int call_id);
    bool hangupCall(int call_id);
    
    // Event handlers - Real PJSIP API
    void setOnRegistered(std::function<void(const std::string&)> callback);
    void setOnRegisterFailed(std::function<void(const std::string&)> callback);
    void setOnUnregistered(std::function<void(const std::string&)> callback);
    void setOnIncomingCall(std::function<void(const std::string&)> callback);
    void setOnCallState(std::function<void(const std::string&)> callback);
    
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
Napi::Value Init(const Napi::CallbackInfo& info);
Napi::Value RegisterAccount(const Napi::CallbackInfo& info);
Napi::Value UnregisterAccount(const Napi::CallbackInfo& info);
Napi::Value GetAccountInfo(const Napi::CallbackInfo& info);
Napi::Value Shutdown(const Napi::CallbackInfo& info);
Napi::Value AddAccount(const Napi::CallbackInfo& info);
Napi::Value RemoveAccount(const Napi::CallbackInfo& info);
Napi::Value MakeCall(const Napi::CallbackInfo& info);
Napi::Value AnswerCall(const Napi::CallbackInfo& info);
Napi::Value HangupCall(const Napi::CallbackInfo& info);
Napi::Value GetVersion(const Napi::CallbackInfo& info);
Napi::Value GetLocalIP(const Napi::CallbackInfo& info);
Napi::Value GetBoundPort(const Napi::CallbackInfo& info);

// Export bindings to Node.js
Napi::Object InitPjsipWrapper(Napi::Env env, Napi::Object exports);

#endif