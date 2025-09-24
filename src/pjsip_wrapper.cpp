#include "pjsip_wrapper.h"
#include <napi.h>
#include <iostream>
#include <sstream>

// Static instance
PJSIPWrapper* PJSIPWrapper::instance = nullptr;

// PJSIPAccount implementation
PJSIPAccount::PJSIPAccount() : acc_id(PJSUA_INVALID_ID), is_registered(false) {
    pj_bzero(&acc_info, sizeof(acc_info));
}

PJSIPAccount::~PJSIPAccount() {
}

// PJSIPWrapper implementation
PJSIPWrapper::PJSIPWrapper() : is_initialized(false), next_account_id(0), transport_id(PJSUA_INVALID_ID) {
}

PJSIPWrapper::~PJSIPWrapper() {
    shutdown();
}

PJSIPWrapper* PJSIPWrapper::getInstance() {
    if (instance == nullptr) {
        instance = new PJSIPWrapper();
    }
    return instance;
}

// PJSIP callback handlers
void PJSIPWrapper::pjsip_on_reg_state(pjsua_acc_id acc_id) {
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    pjsua_acc_info acc_info;
    pj_status_t status = pjsua_acc_get_info(acc_id, &acc_info);
    
    if (status == PJ_SUCCESS) {
        std::string aor = std::string(acc_info.acc_uri.ptr, acc_info.acc_uri.slen);
        
        if (acc_info.status == PJSIP_SC_OK) {
            std::cout << "✅ Registration successful for: " << aor << std::endl;
            if (wrapper->on_registered) {
                wrapper->on_registered(aor);
            }
        } else {
            std::cout << "❌ Registration failed for: " << aor << " (Status: " << acc_info.status << ")" << std::endl;
            if (wrapper->on_register_failed) {
                wrapper->on_register_failed(aor);
            }
        }
        
        // Update account registration status
        std::lock_guard<std::mutex> lock(wrapper->accounts_mutex);
        for (auto& account : wrapper->accounts) {
            if (account->acc_id == acc_id) {
                account->is_registered = (acc_info.status == PJSIP_SC_OK);
                account->acc_info = acc_info;
                break;
            }
        }
    }
}

void PJSIPWrapper::pjsip_on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata) {
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    pjsua_call_info call_info;
    
    pjsua_call_get_info(call_id, &call_info);
    std::string caller = std::string(call_info.remote_info.ptr, call_info.remote_info.slen);
    
    std::cout << "📞 Incoming call from: " << caller << std::endl;
    
    if (wrapper->on_incoming_call) {
        wrapper->on_incoming_call(caller);
    }
    
    // Auto-answer for demo (you can change this behavior)
    pjsua_call_answer(call_id, 200, NULL, NULL);
}

void PJSIPWrapper::pjsip_on_call_state(pjsua_call_id call_id, pjsip_event *e) {
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    pjsua_call_info call_info;
    
    pjsua_call_get_info(call_id, &call_info);
    std::string state_text = std::string(call_info.state_text.ptr, call_info.state_text.slen);
    
    std::cout << "📞 Call " << call_id << " state: " << state_text << std::endl;
    
    if (wrapper->on_call_state) {
        wrapper->on_call_state(state_text);
    }
}

void PJSIPWrapper::pjsip_on_call_media_state(pjsua_call_id call_id) {
    pjsua_call_info call_info;
    pjsua_call_get_info(call_id, &call_info);
    
    if (call_info.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        pjsua_conf_connect(call_info.conf_slot, 0);
        pjsua_conf_connect(0, call_info.conf_slot);
        std::cout << "🔊 Media connected for call " << call_id << std::endl;
    }
}

// Core functions - Real PJSIP API
bool PJSIPWrapper::initialize() {
    if (is_initialized) {
        return true;
    }
    
    pj_status_t status;
    
    // Create pjsua first
    status = pjsua_create();
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error in pjsua_create(): " << status << std::endl;
        return false;
    }
    
    // Initialize configs
    pjsua_config_default(&ua_cfg);
    pjsua_logging_config_default(&log_cfg);
    pjsua_media_config_default(&media_cfg);
    
    // Set callbacks
    ua_cfg.cb.on_reg_state = &PJSIPWrapper::pjsip_on_reg_state;
    ua_cfg.cb.on_incoming_call = &PJSIPWrapper::pjsip_on_incoming_call;
    ua_cfg.cb.on_call_state = &PJSIPWrapper::pjsip_on_call_state;
    ua_cfg.cb.on_call_media_state = &PJSIPWrapper::pjsip_on_call_media_state;
    
    // Configure logging
    log_cfg.console_level = 4; // Info level
    log_cfg.level = 4;
    
    // Initialize pjsua
    status = pjsua_init(&ua_cfg, &log_cfg, &media_cfg);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error in pjsua_init(): " << status << std::endl;
        pjsua_destroy();
        return false;
    }
    
    // Add UDP transport
    pjsua_transport_config_default(&udp_cfg);
    udp_cfg.port = 5060; // Default SIP port
    
    status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &udp_cfg, &transport_id);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error creating transport: " << status << std::endl;
        pjsua_destroy();
        return false;
    }
    
    // Start pjsua
    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error starting pjsua: " << status << std::endl;
        pjsua_destroy();
        return false;
    }
    
    is_initialized = true;
    std::cout << "✅ PJSIP initialized successfully" << std::endl;
    return true;
}

bool PJSIPWrapper::shutdown() {
    if (!is_initialized) {
        return true;
    }
    
    std::cout << "🛑 Shutting down PJSIP..." << std::endl;
    
    // Clear accounts
    {
        std::lock_guard<std::mutex> lock(accounts_mutex);
        accounts.clear();
    }
    
    // Destroy pjsua
    pjsua_destroy();
    
    is_initialized = false;
    std::cout << "✅ PJSIP shutdown complete" << std::endl;
    return true;
}

// Account management - Real PJSIP API
int PJSIPWrapper::addAccount(const std::string& aor, const std::string& registrar, 
                           const std::string& username, const std::string& password,
                           const std::string& proxy) {
    if (!is_initialized) {
        std::cerr << "❌ PJSIP not initialized" << std::endl;
        return -1;
    }
    
    // Create account config
    pjsua_acc_config acc_cfg;
    pjsua_acc_config_default(&acc_cfg);
    
    // Set account ID and registrar
    acc_cfg.id = pj_str((char*)aor.c_str());
    acc_cfg.reg_uri = pj_str((char*)registrar.c_str());
    
    // Set credentials
    acc_cfg.cred_count = 1;
    acc_cfg.cred_info[0].realm = pj_str((char*)"*");
    acc_cfg.cred_info[0].scheme = pj_str((char*)"digest");
    acc_cfg.cred_info[0].username = pj_str((char*)username.c_str());
    acc_cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    acc_cfg.cred_info[0].data = pj_str((char*)password.c_str());
    
    // Set proxy if provided
    if (!proxy.empty()) {
        acc_cfg.proxy_cnt = 1;
        acc_cfg.proxy[0] = pj_str((char*)proxy.c_str());
    }
    
    // Add account
    pjsua_acc_id acc_id;
    pj_status_t status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &acc_id);
    
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error adding account: " << status << std::endl;
        return -1;
    }
    
    // Create account object
    auto account = std::make_unique<PJSIPAccount>();
    account->acc_id = acc_id;
    account->aor = aor;
    account->registrar = registrar;
    account->username = username;
    account->password = password;
    account->proxy = proxy;
    account->is_registered = false;
    
    int account_index = next_account_id++;
    
    // Store account
    {
        std::lock_guard<std::mutex> lock(accounts_mutex);
        accounts.push_back(std::move(account));
    }
    
    std::cout << "✅ Account added: " << aor << " (ID: " << acc_id << ")" << std::endl;
    return acc_id;
}

bool PJSIPWrapper::removeAccount(int acc_id) {
    if (!is_initialized) {
        return false;
    }
    
    pj_status_t status = pjsua_acc_del((pjsua_acc_id)acc_id);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error removing account: " << status << std::endl;
        return false;
    }
    
    // Remove from local storage
    {
        std::lock_guard<std::mutex> lock(accounts_mutex);
        accounts.erase(
            std::remove_if(accounts.begin(), accounts.end(),
                [acc_id](const std::unique_ptr<PJSIPAccount>& account) {
                    return account->acc_id == acc_id;
                }),
            accounts.end()
        );
    }
    
    std::cout << "✅ Account removed (ID: " << acc_id << ")" << std::endl;
    return true;
}

PJSIPAccount* PJSIPWrapper::getAccount(int acc_id) {
    std::lock_guard<std::mutex> lock(accounts_mutex);
    for (auto& account : accounts) {
        if (account->acc_id == acc_id) {
            return account.get();
        }
    }
    return nullptr;
}

std::vector<PJSIPAccount*> PJSIPWrapper::getAllAccounts() {
    std::vector<PJSIPAccount*> result;
    std::lock_guard<std::mutex> lock(accounts_mutex);
    
    for (auto& account : accounts) {
        result.push_back(account.get());
    }
    
    return result;
}

// Registration - Real PJSIP API
bool PJSIPWrapper::registerAccount(int acc_id) {
    if (!is_initialized) {
        return false;
    }
    
    pj_status_t status = pjsua_acc_set_registration((pjsua_acc_id)acc_id, PJ_TRUE);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error registering account: " << status << std::endl;
        return false;
    }
    
    std::cout << "🔄 Registration started for account ID: " << acc_id << std::endl;
    return true;
}

bool PJSIPWrapper::unregisterAccount(int acc_id) {
    if (!is_initialized) {
        return false;
    }
    
    pj_status_t status = pjsua_acc_set_registration((pjsua_acc_id)acc_id, PJ_FALSE);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error unregistering account: " << status << std::endl;
        return false;
    }
    
    std::cout << "📤 Unregistration started for account ID: " << acc_id << std::endl;
    return true;
}

bool PJSIPWrapper::refreshRegistration(int acc_id) {
    return registerAccount(acc_id);
}

// Call management - Real PJSIP API
bool PJSIPWrapper::makeCall(int acc_id, const std::string& uri) {
    if (!is_initialized) {
        return false;
    }
    
    pj_str_t dest_uri = pj_str((char*)uri.c_str());
    pjsua_call_id call_id;
    
    pj_status_t status = pjsua_call_make_call((pjsua_acc_id)acc_id, &dest_uri, 0, NULL, NULL, &call_id);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error making call: " << status << std::endl;
        return false;
    }
    
    std::cout << "📞 Making call to: " << uri << " (Call ID: " << call_id << ")" << std::endl;
    return true;
}

bool PJSIPWrapper::answerCall(int call_id) {
    pj_status_t status = pjsua_call_answer((pjsua_call_id)call_id, 200, NULL, NULL);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error answering call: " << status << std::endl;
        return false;
    }
    
    std::cout << "✅ Call answered (Call ID: " << call_id << ")" << std::endl;
    return true;
}

bool PJSIPWrapper::hangupCall(int call_id) {
    pj_status_t status = pjsua_call_hangup((pjsua_call_id)call_id, 0, NULL, NULL);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error hanging up call: " << status << std::endl;
        return false;
    }
    
    std::cout << "📴 Call hung up (Call ID: " << call_id << ")" << std::endl;
    return true;
}

// Event handlers
void PJSIPWrapper::setOnRegistered(std::function<void(const std::string&)> callback) {
    on_registered = callback;
}

void PJSIPWrapper::setOnRegisterFailed(std::function<void(const std::string&)> callback) {
    on_register_failed = callback;
}

void PJSIPWrapper::setOnUnregistered(std::function<void(const std::string&)> callback) {
    on_unregistered = callback;
}

void PJSIPWrapper::setOnIncomingCall(std::function<void(const std::string&)> callback) {
    on_incoming_call = callback;
}

void PJSIPWrapper::setOnCallState(std::function<void(const std::string&)> callback) {
    on_call_state = callback;
}

// Utility functions
std::string PJSIPWrapper::getVersion() {
    return "PJSIP " + std::string(pj_get_version());
}

std::string PJSIPWrapper::getLocalIP() {
    if (!is_initialized || transport_id == PJSUA_INVALID_ID) {
        return "0.0.0.0";
    }
    
    pjsua_transport_info transport_info;
    pj_status_t status = pjsua_transport_get_info(transport_id, &transport_info);
    
    if (status == PJ_SUCCESS) {
        return std::string(transport_info.local_name.host.ptr, transport_info.local_name.host.slen);
    }
    
    return "0.0.0.0";
}

int PJSIPWrapper::getBoundPort() {
    if (!is_initialized || transport_id == PJSUA_INVALID_ID) {
        return 0;
    }
    
    pjsua_transport_info transport_info;
    pj_status_t status = pjsua_transport_get_info(transport_id, &transport_info);
    
    if (status == PJ_SUCCESS) {
        return transport_info.local_name.port;
    }
    
    return 0;
}

// N-API function implementations
Napi::Value Init(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->initialize();
    
    return Napi::Boolean::New(env, result);
}

Napi::Value AddAccount(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected object with account configuration").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Object config = info[0].As<Napi::Object>();
    
    std::string aor = config.Get("aor").As<Napi::String>().Utf8Value();
    std::string registrar = config.Get("registrar").As<Napi::String>().Utf8Value();
    std::string username = config.Get("username").As<Napi::String>().Utf8Value();
    std::string password = config.Get("password").As<Napi::String>().Utf8Value();
    std::string proxy = config.Has("proxy") ? config.Get("proxy").As<Napi::String>().Utf8Value() : "";
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    int acc_id = wrapper->addAccount(aor, registrar, username, password, proxy);
    
    return Napi::Number::New(env, acc_id);
}

Napi::Value RegisterAccount(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected account ID").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int acc_id = info[0].As<Napi::Number>().Int32Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->registerAccount(acc_id);
    
    return Napi::Boolean::New(env, result);
}

Napi::Value UnregisterAccount(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected account ID").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int acc_id = info[0].As<Napi::Number>().Int32Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->unregisterAccount(acc_id);
    
    return Napi::Boolean::New(env, result);
}

Napi::Value GetAccountInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected account ID").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int acc_id = info[0].As<Napi::Number>().Int32Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    PJSIPAccount* account = wrapper->getAccount(acc_id);
    
    if (!account) {
        return env.Null();
    }
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("acc_id", Napi::Number::New(env, account->acc_id));
    result.Set("aor", Napi::String::New(env, account->aor));
    result.Set("registrar", Napi::String::New(env, account->registrar));
    result.Set("username", Napi::String::New(env, account->username));
    result.Set("proxy", Napi::String::New(env, account->proxy));
    result.Set("is_registered", Napi::Boolean::New(env, account->is_registered));
    
    return result;
}

Napi::Value RemoveAccount(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected account ID").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int acc_id = info[0].As<Napi::Number>().Int32Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->removeAccount(acc_id);
    
    return Napi::Boolean::New(env, result);
}

Napi::Value Shutdown(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->shutdown();
    
    return Napi::Boolean::New(env, result);
}

Napi::Value MakeCall(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Expected account ID and URI").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int acc_id = info[0].As<Napi::Number>().Int32Value();
    std::string uri = info[1].As<Napi::String>().Utf8Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->makeCall(acc_id, uri);
    
    return Napi::Boolean::New(env, result);
}

Napi::Value AnswerCall(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected call ID").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int call_id = info[0].As<Napi::Number>().Int32Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->answerCall(call_id);
    
    return Napi::Boolean::New(env, result);
}

Napi::Value HangupCall(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected call ID").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int call_id = info[0].As<Napi::Number>().Int32Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->hangupCall(call_id);
    
    return Napi::Boolean::New(env, result);
}

Napi::Value GetVersion(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    std::string version = wrapper->getVersion();
    
    return Napi::String::New(env, version);
}

Napi::Value GetLocalIP(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    std::string ip = wrapper->getLocalIP();
    
    return Napi::String::New(env, ip);
}

Napi::Value GetBoundPort(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    int port = wrapper->getBoundPort();
    
    return Napi::Number::New(env, port);
}

// Export bindings to Node.js
Napi::Object InitPjsipWrapper(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "Init"), Napi::Function::New<Init>(env));
    exports.Set(Napi::String::New(env, "addAccount"), Napi::Function::New<AddAccount>(env));
    exports.Set(Napi::String::New(env, "registerAccount"), Napi::Function::New<RegisterAccount>(env));
    exports.Set(Napi::String::New(env, "unregisterAccount"), Napi::Function::New<UnregisterAccount>(env));
    exports.Set(Napi::String::New(env, "getAccountInfo"), Napi::Function::New<GetAccountInfo>(env));
    exports.Set(Napi::String::New(env, "removeAccount"), Napi::Function::New<RemoveAccount>(env));
    exports.Set(Napi::String::New(env, "shutdown"), Napi::Function::New<Shutdown>(env));
    exports.Set(Napi::String::New(env, "makeCall"), Napi::Function::New<MakeCall>(env));
    exports.Set(Napi::String::New(env, "answerCall"), Napi::Function::New<AnswerCall>(env));
    exports.Set(Napi::String::New(env, "hangupCall"), Napi::Function::New<HangupCall>(env));
    exports.Set(Napi::String::New(env, "getVersion"), Napi::Function::New<GetVersion>(env));
    exports.Set(Napi::String::New(env, "getLocalIP"), Napi::Function::New<GetLocalIP>(env));
    exports.Set(Napi::String::New(env, "getBoundPort"), Napi::Function::New<GetBoundPort>(env));
    
    return exports;
}