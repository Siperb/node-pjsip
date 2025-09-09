#ifndef NODE_PJSIP_WRAPPER_H
#define NODE_PJSIP_WRAPPER_H

#include <napi.h>

// Register a SIP account using PJSUA2
Napi::Value RegisterAccount(const Napi::CallbackInfo& info);

// Shutdown endpoint and account
Napi::Value Shutdown(const Napi::CallbackInfo& info);

// Export bindings to Node.js
Napi::Object InitPjsipWrapper(Napi::Env env, Napi::Object exports);

#endif
