#ifndef NODE_PJSIP_WRAPPER_H
#define NODE_PJSIP_WRAPPER_H

#include <napi.h>
#include <memory>
#include <map>
#include <string>
#include <thread>
#include <chrono>

// Initialize PJSIP
Napi::Value Init(const Napi::CallbackInfo& info);

// Register a SIP account using PJSUA2
Napi::Value RegisterAccount(const Napi::CallbackInfo& info);

// Get account info
Napi::Value GetAccountInfo(const Napi::CallbackInfo& info);

// Shutdown endpoint and account
Napi::Value Shutdown(const Napi::CallbackInfo& info);

// Send SIP message
void sendSIPMessage(const std::string& message, const std::string& host, int port);

// Export bindings to Node.js
Napi::Object InitPjsipWrapper(Napi::Env env, Napi::Object exports);

#endif
