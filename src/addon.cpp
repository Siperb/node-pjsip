#include <napi.h>
#include "pjsip_wrapper.h"

// This function is called automatically by Node when the native addon is loaded.
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return InitPjsipWrapper(env, exports);
}

NODE_API_MODULE(node_pjsip, Init)
