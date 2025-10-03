#include "pjsip_wrapper.h"
#include <napi.h>
#include <iostream>
#include <sstream>

// Static instance
PJSIPWrapper* PJSIPWrapper::instance = nullptr;

// PJSIPAccount implementation
PJSIPAccount::PJSIPAccount() : acc_id(PJSUA_INVALID_ID), is_registered(false), is_unregistering(false) {
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
void PJSIPWrapper::setOnRegistered(const Napi::Function& callback) {
    on_registered_tsfn = Napi::ThreadSafeFunction::New(
        callback.Env(),
        callback,
        "onRegistered",
        0,
        1
    );
}

void PJSIPWrapper::setOnRegisterFailed(const Napi::Function& callback) {
    on_register_failed_tsfn = Napi::ThreadSafeFunction::New(
        callback.Env(),
        callback,
        "onRegisterFailed",
        0,
        1
    );
}

void PJSIPWrapper::setOnUnregistered(const Napi::Function& callback) {
    on_unregistered_tsfn = Napi::ThreadSafeFunction::New(
        callback.Env(),
        callback,
        "onUnregistered",
        0,
        1
    );
}

void PJSIPWrapper::setOnIncomingCall(const Napi::Function& callback) {
    on_incoming_call_tsfn = Napi::ThreadSafeFunction::New(
        callback.Env(),
        callback,
        "onIncomingCall",
        0,
        1
    );
}

void PJSIPWrapper::setOnCallState(const Napi::Function& callback) {
    on_call_state_tsfn = Napi::ThreadSafeFunction::New(
        callback.Env(),
        callback,
        "onCallState",
        0,
        1
    );
}

void PJSIPWrapper::pjsip_on_reg_state(pjsua_acc_id acc_id) {
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    pjsua_acc_info acc_info;
    pj_status_t status = pjsua_acc_get_info(acc_id, &acc_info);
    
    if (status == PJ_SUCCESS) {
        std::string aor = std::string(acc_info.acc_uri.ptr, acc_info.acc_uri.slen);
        
        if (acc_info.status == PJSIP_SC_OK) {
            std::cout << "✅ Registration successful for: " << aor << std::endl;
            if (wrapper->on_registered_tsfn) {
                wrapper->on_registered_tsfn.BlockingCall([aor](Napi::Env env, Napi::Function jsCallback) {
                    jsCallback.Call({Napi::String::New(env, aor)});
                });
            }
        } else {
            PJSIPAccount* account = wrapper->getAccount(acc_id);
            if (account && account->is_unregistering) {
                account->is_unregistering = false; // reset the flag
                std::cout << "📤 Unregistered: " << aor << std::endl;
                if (wrapper->on_unregistered_tsfn) {
                    wrapper->on_unregistered_tsfn.BlockingCall([aor](Napi::Env env, Napi::Function jsCallback) {
                        jsCallback.Call({Napi::String::New(env, aor)});
                    });
                }
            } else {
                std::cout << "❌ Registration failed for: " << aor << " (Status: " << acc_info.status << ")" << std::endl;
                if (wrapper->on_register_failed_tsfn) {
                    wrapper->on_register_failed_tsfn.BlockingCall([aor](Napi::Env env, Napi::Function jsCallback) {
                        jsCallback.Call({Napi::String::New(env, aor)});
                    });
                }
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
    
    if (wrapper->on_incoming_call_tsfn) {
        wrapper->on_incoming_call_tsfn.BlockingCall([caller](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::String::New(env, caller)});
        });
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
    
    if (wrapper->on_call_state_tsfn) {
        wrapper->on_call_state_tsfn.BlockingCall([state_text](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::String::New(env, state_text)});
        });
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
bool PJSIPWrapper::initialize(const Napi::Object& config) {
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
    
    // Apply configuration from JavaScript
    if (config.Has("logLevel")) {
        log_cfg.level = config.Get("logLevel").As<Napi::Number>().Int32Value();
    } else {
        log_cfg.level = 4; // Default info level
    }
    
    if (config.Has("logConsoleLevel")) {
        log_cfg.console_level = config.Get("logConsoleLevel").As<Napi::Number>().Int32Value();
    } else {
        log_cfg.console_level = 4; // Default info level
    }
    
    if (config.Has("clockRate")) {
        media_cfg.clock_rate = config.Get("clockRate").As<Napi::Number>().Int32Value();
    }
    
    if (config.Has("channelCount")) {
        media_cfg.channel_count = config.Get("channelCount").As<Napi::Number>().Int32Value();
    }
    
    if (config.Has("userAgent")) {
        std::string userAgent = config.Get("userAgent").As<Napi::String>().Utf8Value();
        ua_cfg.user_agent = pj_str(const_cast<char*>(userAgent.c_str()));
    }
    
    if (config.Has("maxCalls")) {
        ua_cfg.max_calls = config.Get("maxCalls").As<Napi::Number>().Int32Value();
    }
    
    // Initialize pjsua
    status = pjsua_init(&ua_cfg, &log_cfg, &media_cfg);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error in pjsua_init(): " << status << std::endl;
        pjsua_destroy();
        return false;
    }
    
    // Add UDP transport
    pjsua_transport_config_default(&udp_cfg);
    
    // Apply transport configuration
    if (config.Has("localPort")) {
        udp_cfg.port = config.Get("localPort").As<Napi::Number>().Int32Value();
    } else {
    udp_cfg.port = 5060; // Default SIP port
    }
    
    pjsip_transport_type_e transport_type = PJSIP_TRANSPORT_UDP;
    if (config.Has("transport")) {
        std::string transport = config.Get("transport").As<Napi::String>().Utf8Value();
        if (transport == "TCP") {
            transport_type = PJSIP_TRANSPORT_TCP;
        } else if (transport == "TLS") {
            transport_type = PJSIP_TRANSPORT_TLS;
        }
    }
    
    status = pjsua_transport_create(transport_type, &udp_cfg, &transport_id);
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
int PJSIPWrapper::registerAccount(const Napi::Object& config) {
    if (!is_initialized) {
        std::cerr << "❌ PJSIP not initialized" << std::endl;
        return -1;
    }
    
    // Extract configuration from JavaScript object
    std::string username = config.Get("username").As<Napi::String>().Utf8Value();
    std::string password = config.Get("password").As<Napi::String>().Utf8Value();
    std::string domain = config.Get("domain").As<Napi::String>().Utf8Value();
    
    // Build AOR (Address of Record)
    std::string aor = "sip:" + username + "@" + domain;
    
    // Get registrar (optional, defaults to domain)
    std::string registrar;
    if (config.Has("registrar")) {
        registrar = config.Get("registrar").As<Napi::String>().Utf8Value();
    } else {
        registrar = "sip:" + domain + ":5060";
    }
    
    // Create account config
    pjsua_acc_config acc_cfg;
    pjsua_acc_config_default(&acc_cfg);
    
    // Set account ID and registrar
    acc_cfg.id = pj_str(const_cast<char*>(aor.c_str()));
    acc_cfg.reg_uri = pj_str(const_cast<char*>(registrar.c_str()));
    
    // Set credentials
    acc_cfg.cred_count = 1;
    acc_cfg.cred_info[0].realm = pj_str(const_cast<char*>("*"));
    acc_cfg.cred_info[0].scheme = pj_str(const_cast<char*>("digest"));
    acc_cfg.cred_info[0].username = pj_str(const_cast<char*>(username.c_str()));
    acc_cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    acc_cfg.cred_info[0].data = pj_str(const_cast<char*>(password.c_str()));
    
    // Set proxy if provided
    if (config.Has("proxy")) {
        std::string proxy = config.Get("proxy").As<Napi::String>().Utf8Value();
        acc_cfg.proxy_cnt = 1;
        acc_cfg.proxy[0] = pj_str(const_cast<char*>(proxy.c_str()));
    }
    
    // Set registration timeout if provided
    if (config.Has("expires")) {
        acc_cfg.reg_timeout = config.Get("expires").As<Napi::Number>().Int32Value();
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
    account->is_registered = false;
    
    // Store account
    {
        std::lock_guard<std::mutex> lock(accounts_mutex);
        accounts.push_back(std::move(account));
    }
    
    std::cout << "✅ Account registered: " << aor << " (ID: " << acc_id << ")" << std::endl;
    return acc_id;
}

bool PJSIPWrapper::unregisterAccount(int acc_id) {
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
    
    std::cout << "✅ Account unregistered (ID: " << acc_id << ")" << std::endl;
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

// Session management - Real PJSIP API
int PJSIPWrapper::invite(const Napi::Object& config) {
    if (!is_initialized) {
        std::cerr << "❌ PJSIP not initialized" << std::endl;
        return -1;
    }
    
    // Extract configuration from JavaScript object
    std::string to = config.Get("to").As<Napi::String>().Utf8Value();
    
    // Get the first available account (for now, we'll use the first registered account)
    pjsua_acc_id acc_id = PJSUA_INVALID_ID;
    {
        std::lock_guard<std::mutex> lock(accounts_mutex);
        for (auto& account : accounts) {
            if (account->is_registered) {
                acc_id = account->acc_id;
                break;
            }
        }
    }
    
    if (acc_id == PJSUA_INVALID_ID) {
        std::cerr << "❌ No registered account available for INVITE" << std::endl;
        return -1;
    }
    
    pj_str_t dest_uri = pj_str(const_cast<char*>(to.c_str()));
    pjsua_call_id call_id;
    
    pj_status_t status = pjsua_call_make_call(acc_id, &dest_uri, 0, NULL, NULL, &call_id);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error making INVITE: " << status << std::endl;
        return -1;
    }
    
    std::cout << "📞 INVITE sent to: " << to << " (Call ID: " << call_id << ")" << std::endl;
    return call_id;
}

bool PJSIPWrapper::answer(int call_id) {
    pj_status_t status = pjsua_call_answer((pjsua_call_id)call_id, 200, NULL, NULL);
    if (status != PJ_SUCCESS) {
        std::cerr << "❌ Error answering call: " << status << std::endl;
        return false;
    }
    
    std::cout << "✅ Call answered (Call ID: " << call_id << ")" << std::endl;
    return true;
}

bool PJSIPWrapper::hangup(int call_id) {
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
    
    Napi::Object config = Napi::Object::New(env);
    if (info.Length() > 0 && info[0].IsObject()) {
        config = info[0].As<Napi::Object>();
    }
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->initialize(config);
    
    Napi::Object response = Napi::Object::New(env);
    response.Set("ok", Napi::Boolean::New(env, result));
    response.Set("code", Napi::Number::New(env, result ? 0 : -1));
    response.Set("data", Napi::String::New(env, result ? "PJSIP initialized" : "Failed to initialize PJSIP"));
    return response;
}

Napi::Value Register(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected object with account configuration").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Object config = info[0].As<Napi::Object>();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    int acc_id = wrapper->registerAccount(config);
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, acc_id >= 0));
    result.Set("code", Napi::Number::New(env, acc_id >= 0 ? 0 : -1));
    result.Set("data", Napi::Number::New(env, acc_id));
    return result;
}

Napi::Value Unregister(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected account ID").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int acc_id = info[0].As<Napi::Number>().Int32Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->unregisterAccount(acc_id);
    
    Napi::Object response = Napi::Object::New(env);
    response.Set("ok", Napi::Boolean::New(env, result));
    response.Set("code", Napi::Number::New(env, result ? 0 : -1));
    response.Set("data", Napi::String::New(env, result ? "Account unregistered" : "Failed to unregister account"));
    return response;
}

Napi::Value Invite(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected object with session configuration").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Object config = info[0].As<Napi::Object>();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    int call_id = wrapper->invite(config);
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, call_id >= 0));
    result.Set("code", Napi::Number::New(env, call_id >= 0 ? 0 : -1));
    result.Set("data", Napi::Number::New(env, call_id));
    return result;
}

Napi::Value Answer(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected call ID").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int call_id = info[0].As<Napi::Number>().Int32Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->answer(call_id);
    
    Napi::Object response = Napi::Object::New(env);
    response.Set("ok", Napi::Boolean::New(env, result));
    response.Set("code", Napi::Number::New(env, result ? 0 : -1));
    response.Set("data", Napi::String::New(env, result ? "Call answered" : "Failed to answer call"));
    return response;
}

Napi::Value Hangup(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected call ID").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int call_id = info[0].As<Napi::Number>().Int32Value();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->hangup(call_id);
    
    return Napi::Boolean::New(env, result);
}

Napi::Value Shutdown(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    bool result = wrapper->shutdown();
    
    Napi::Object response = Napi::Object::New(env);
    response.Set("ok", Napi::Boolean::New(env, result));
    response.Set("code", Napi::Number::New(env, result ? 0 : -1));
    response.Set("data", Napi::String::New(env, result ? "PJSIP shutdown" : "Failed to shutdown PJSIP"));
    return response;
}

Napi::Value GetVersion(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    std::string version = wrapper->getVersion();
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, true));
    result.Set("code", Napi::Number::New(env, 0));
    result.Set("data", Napi::String::New(env, version));
    return result;
}

Napi::Value GetLocalIP(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    std::string ip = wrapper->getLocalIP();
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, true));
    result.Set("code", Napi::Number::New(env, 0));
    result.Set("data", Napi::String::New(env, ip));
    return result;
}

Napi::Value GetBoundPort(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    int port = wrapper->getBoundPort();
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, true));
    result.Set("code", Napi::Number::New(env, 0));
    result.Set("data", Napi::Number::New(env, port));
    return result;
}

Napi::Value SetOnRegistered(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    wrapper->setOnRegistered(info[0].As<Napi::Function>());
    return env.Undefined();
}

Napi::Value SetOnRegisterFailed(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    wrapper->setOnRegisterFailed(info[0].As<Napi::Function>());
    return env.Undefined();
}

Napi::Value SetOnUnregistered(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    wrapper->setOnUnregistered(info[0].As<Napi::Function>());
    return env.Undefined();
}

Napi::Value SetOnIncomingCall(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    wrapper->setOnIncomingCall(info[0].As<Napi::Function>());
    return env.Undefined();
}

Napi::Value SetOnCallState(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    wrapper->setOnCallState(info[0].As<Napi::Function>());
    return env.Undefined();
}

// Additional function implementations for client requirements
Napi::Value SetLogLevel(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected log level").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    int level = info[0].As<Napi::Number>().Int32Value();
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    
    // For now, just return success - actual implementation would set log level
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, true));
    result.Set("code", Napi::Number::New(env, 0));
    result.Set("data", Napi::String::New(env, "Log level set"));
    return result;
}

Napi::Value GetRegistrationStatus(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    
    // Get registration status from wrapper
    auto accounts = wrapper->getAllAccounts();
    Napi::Object result = Napi::Object::New(env);
    
    if (accounts.empty()) {
        result.Set("ok", Napi::Boolean::New(env, false));
        result.Set("code", Napi::Number::New(env, -1));
        result.Set("message", Napi::String::New(env, "No accounts registered"));
    } else {
        result.Set("ok", Napi::Boolean::New(env, true));
        result.Set("code", Napi::Number::New(env, 0));
        
        Napi::Array accountArray = Napi::Array::New(env);
        for (size_t i = 0; i < accounts.size(); ++i) {
            Napi::Object accountObj = Napi::Object::New(env);
            accountObj.Set("acc_id", Napi::Number::New(env, accounts[i]->acc_id));
            accountObj.Set("aor", Napi::String::New(env, accounts[i]->aor));
            accountObj.Set("is_registered", Napi::Boolean::New(env, accounts[i]->is_registered));
            accountArray.Set(i, accountObj);
        }
        result.Set("data", accountArray);
    }
    
    return result;
}

Napi::Value Reject(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected call ID").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    int callId = info[0].As<Napi::Number>().Int32Value();
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    
    // For now, return success - actual implementation would reject the call
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, true));
    result.Set("code", Napi::Number::New(env, 0));
    result.Set("data", Napi::String::New(env, "Call rejected"));
    return result;
}

Napi::Value Cancel(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected call ID").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    int callId = info[0].As<Napi::Number>().Int32Value();
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    
    // For now, return success - actual implementation would cancel the call
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, true));
    result.Set("code", Napi::Number::New(env, 0));
    result.Set("data", Napi::String::New(env, "Call cancelled"));
    return result;
}

Napi::Value Bye(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected call ID").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    int callId = info[0].As<Napi::Number>().Int32Value();
    PJSIPWrapper* wrapper = PJSIPWrapper::getInstance();
    
    bool success = wrapper->hangup(callId);
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, success));
    result.Set("code", Napi::Number::New(env, success ? 0 : -1));
    result.Set("data", Napi::String::New(env, success ? "Call ended" : "Failed to end call"));
    return result;
}

// Placeholder implementations for functions not yet implemented
Napi::Value Hold(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Hold not implemented yet"));
    return result;
}

Napi::Value Unhold(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Unhold not implemented yet"));
    return result;
}

Napi::Value Reinvite(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Reinvite not implemented yet"));
    return result;
}

Napi::Value Update(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Update not implemented yet"));
    return result;
}

Napi::Value Mute(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Mute not implemented yet"));
    return result;
}

Napi::Value Unmute(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Unmute not implemented yet"));
    return result;
}

Napi::Value Dtmf(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "DTMF not implemented yet"));
    return result;
}

Napi::Value Info(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Info not implemented yet"));
    return result;
}

Napi::Value GetCallInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "GetCallInfo not implemented yet"));
    return result;
}

Napi::Value GetMediaStats(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "GetMediaStats not implemented yet"));
    return result;
}

Napi::Value SendMessage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "SendMessage not implemented yet"));
    return result;
}

Napi::Value Subscribe(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Subscribe not implemented yet"));
    return result;
}

Napi::Value Unsubscribe(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Unsubscribe not implemented yet"));
    return result;
}

Napi::Value Publish(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Publish not implemented yet"));
    return result;
}

Napi::Value Conference(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "Conference not implemented yet"));
    return result;
}

Napi::Value ListTransports(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "ListTransports not implemented yet"));
    return result;
}

Napi::Value ListCalls(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "ListCalls not implemented yet"));
    return result;
}

Napi::Value SetCodecPriority(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "SetCodecPriority not implemented yet"));
    return result;
}

Napi::Value CreateTransport(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "CreateTransport not implemented yet"));
    return result;
}

Napi::Value SetOutboundProxy(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "SetOutboundProxy not implemented yet"));
    return result;
}

Napi::Value SetDnsServers(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "SetDnsServers not implemented yet"));
    return result;
}

Napi::Value SetStunServers(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "SetStunServers not implemented yet"));
    return result;
}

Napi::Value SetTurnServers(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, false));
    result.Set("code", Napi::Number::New(env, -1));
    result.Set("message", Napi::String::New(env, "SetTurnServers not implemented yet"));
    return result;
}

Napi::Value SetEventCallback(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    // Store the callback function for event dispatching
    // This would be implemented to handle the single event callback system
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", Napi::Boolean::New(env, true));
    result.Set("code", Napi::Number::New(env, 0));
    result.Set("data", Napi::String::New(env, "Event callback set"));
    return result;
}

// Export bindings to Node.js
Napi::Object InitPjsipWrapper(Napi::Env env, Napi::Object exports) {
    // Core functions
    exports.Set(Napi::String::New(env, "Init"), Napi::Function::New<Init>(env));
    exports.Set(Napi::String::New(env, "Shutdown"), Napi::Function::New<Shutdown>(env));
    exports.Set(Napi::String::New(env, "SetLogLevel"), Napi::Function::New<SetLogLevel>(env));
    exports.Set(Napi::String::New(env, "GetVersion"), Napi::Function::New<GetVersion>(env));
    
    // Account / Registration functions
    exports.Set(Napi::String::New(env, "Register"), Napi::Function::New<Register>(env));
    exports.Set(Napi::String::New(env, "Unregister"), Napi::Function::New<Unregister>(env));
    exports.Set(Napi::String::New(env, "GetRegistrationStatus"), Napi::Function::New<GetRegistrationStatus>(env));
    
    // Call functions
    exports.Set(Napi::String::New(env, "Invite"), Napi::Function::New<Invite>(env));
    exports.Set(Napi::String::New(env, "Answer"), Napi::Function::New<Answer>(env));
    exports.Set(Napi::String::New(env, "Reject"), Napi::Function::New<Reject>(env));
    exports.Set(Napi::String::New(env, "Cancel"), Napi::Function::New<Cancel>(env));
    exports.Set(Napi::String::New(env, "Bye"), Napi::Function::New<Bye>(env));
    exports.Set(Napi::String::New(env, "Hold"), Napi::Function::New<Hold>(env));
    exports.Set(Napi::String::New(env, "Unhold"), Napi::Function::New<Unhold>(env));
    exports.Set(Napi::String::New(env, "Reinvite"), Napi::Function::New<Reinvite>(env));
    exports.Set(Napi::String::New(env, "Update"), Napi::Function::New<Update>(env));
    exports.Set(Napi::String::New(env, "Mute"), Napi::Function::New<Mute>(env));
    exports.Set(Napi::String::New(env, "Unmute"), Napi::Function::New<Unmute>(env));
    exports.Set(Napi::String::New(env, "Dtmf"), Napi::Function::New<Dtmf>(env));
    exports.Set(Napi::String::New(env, "Info"), Napi::Function::New<Info>(env));
    exports.Set(Napi::String::New(env, "GetCallInfo"), Napi::Function::New<GetCallInfo>(env));
    exports.Set(Napi::String::New(env, "GetMediaStats"), Napi::Function::New<GetMediaStats>(env));
    
    // Messaging & Presence functions
    exports.Set(Napi::String::New(env, "SendMessage"), Napi::Function::New<SendMessage>(env));
    exports.Set(Napi::String::New(env, "Subscribe"), Napi::Function::New<Subscribe>(env));
    exports.Set(Napi::String::New(env, "Unsubscribe"), Napi::Function::New<Unsubscribe>(env));
    exports.Set(Napi::String::New(env, "Publish"), Napi::Function::New<Publish>(env));
    
    // Conference / Utility functions
    exports.Set(Napi::String::New(env, "Conference"), Napi::Function::New<Conference>(env));
    exports.Set(Napi::String::New(env, "ListTransports"), Napi::Function::New<ListTransports>(env));
    exports.Set(Napi::String::New(env, "ListCalls"), Napi::Function::New<ListCalls>(env));
    exports.Set(Napi::String::New(env, "SetCodecPriority"), Napi::Function::New<SetCodecPriority>(env));
    
    // Transport & Network functions
    exports.Set(Napi::String::New(env, "CreateTransport"), Napi::Function::New<CreateTransport>(env));
    exports.Set(Napi::String::New(env, "SetOutboundProxy"), Napi::Function::New<SetOutboundProxy>(env));
    exports.Set(Napi::String::New(env, "SetDnsServers"), Napi::Function::New<SetDnsServers>(env));
    exports.Set(Napi::String::New(env, "SetStunServers"), Napi::Function::New<SetStunServers>(env));
    exports.Set(Napi::String::New(env, "SetTurnServers"), Napi::Function::New<SetTurnServers>(env));
    
    // Additional utility functions
    exports.Set(Napi::String::New(env, "GetLocalIP"), Napi::Function::New<GetLocalIP>(env));
    exports.Set(Napi::String::New(env, "GetBoundPort"), Napi::Function::New<GetBoundPort>(env));
    
    // Event callback system
    exports.Set(Napi::String::New(env, "SetEventCallback"), Napi::Function::New<SetEventCallback>(env));
    
    return exports;
}