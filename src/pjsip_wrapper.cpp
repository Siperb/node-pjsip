#include "pjsip_wrapper.h"

#include <napi.h>
#include <pjsua2.hpp>
#include <iostream>

using namespace pj;

static Endpoint ep;

// Standalone Init function
Napi::Value Init(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  try {
    std::cout << "[DEBUG] Checking PJSUA state..." << std::endl;
    if (ep.libGetState() == PJSUA_STATE_NULL) {
      std::cout << "[DEBUG] Calling ep.libCreate()..." << std::endl;
      ep.libCreate();
      std::cout << "[DEBUG] ep.libCreate() done." << std::endl;

      EpConfig epCfg;
      epCfg.logConfig.level = 5;
      epCfg.logConfig.consoleLevel = 5;
      std::cout << "[DEBUG] Calling ep.libInit()..." << std::endl;
      ep.libInit(epCfg);
      std::cout << "[DEBUG] ep.libInit() done." << std::endl;

      TransportConfig tcfg;
      tcfg.port = 5060;
      std::cout << "[DEBUG] Calling ep.transportCreate()..." << std::endl;
      ep.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
      std::cout << "[DEBUG] ep.transportCreate() done." << std::endl;

      std::cout << "[DEBUG] Calling ep.libStart()..." << std::endl;
      ep.libStart();
      std::cout << "[DEBUG] ep.libStart() done." << std::endl;

      return Napi::String::New(env, "PJSIP initialized");
    } else {
      std::cout << "[DEBUG] PJSIP already initialized." << std::endl;
      return Napi::String::New(env, "PJSIP already initialized");
    }
  } catch (Error& err) {
    std::cout << "[DEBUG] Exception: " << err.info() << std::endl;
    return Napi::String::New(env, std::string("PJSUA2 error: ") + err.info());
  }
}

// Subclass Account to override virtual methods
class MyAccount : public Account {
public:
  void onRegState(OnRegStateParam &prm) override {
    AccountInfo ai = getInfo();
    std::cout << "*** Registration: " << ai.regIsActive << " to " << ai.uri << std::endl;
  }
};

static std::shared_ptr<MyAccount> acc;

Napi::Value RegisterAccount(const Napi::CallbackInfo& info) {
  std::cout << "Registering with:\n";
  Napi::Env env = info.Env();

  if (!info[0].IsObject()) {
    Napi::TypeError::New(env, "First argument must be an object").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::cout << "Received configuration object" << std::endl;

  Napi::Object config = info[0].As<Napi::Object>();
  std::string aor = config.Get("aor").ToString();
  std::string registrar = config.Get("registrar").ToString();
  std::string sipUser = config.Get("username").ToString();
  std::string sipPass = config.Get("password").ToString();

  std::cout << "  AOR: " << aor << "\n";
  std::cout << "  Registrar: " << registrar << "\n";
  std::cout << "  Username: " << sipUser << "\n";
  std::cout << "  Password: " << sipPass << "\n";

  try {
    if (ep.libGetState() == PJSUA_STATE_NULL) {
      ep.libCreate();
      std::cout << "libCreate" << std::endl;

      EpConfig epCfg;
      epCfg.logConfig.level = 5;
      epCfg.logConfig.consoleLevel = 5;
      ep.libInit(epCfg);
      std::cout << "libInit" << std::endl;

      TransportConfig tcfg;
      tcfg.port = 5060;
      ep.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
      std::cout << "transportCreate" << std::endl;

      ep.libStart();
      std::cout << "PJSIP started" << std::endl;
    }

    AccountConfig acfg;
    acfg.idUri = aor;
    acfg.regConfig.registrarUri = registrar;
    acfg.sipConfig.authCreds.push_back(AuthCredInfo("digest", "*", sipUser, 0, sipPass));

    std::cout << "PJSIP authCreds" << std::endl;

    acc = std::make_shared<MyAccount>();
    std::cout << "Creating account" << std::endl;

    acc->create(acfg);
    std::cout << "Account created" << std::endl;

    return Napi::String::New(env, "Account registered");
  } catch (Error& err) {
    return Napi::String::New(env, std::string("PJSUA2 error: ") + err.info());
  }
}

Napi::Value Shutdown(const Napi::CallbackInfo& info) {
  if (acc) {
    acc->shutdown();
    acc.reset();
  }

  if (ep.libGetState() != PJSUA_STATE_NULL) {
    ep.libDestroy();
  }

  return Napi::String::New(info.Env(), "PJSIP shutdown");
}

Napi::Object InitPjsipWrapper(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "Init"), Napi::Function::New(env, Init));
  exports.Set(Napi::String::New(env, "registerAccount"), Napi::Function::New(env, RegisterAccount));
  exports.Set(Napi::String::New(env, "shutdown"), Napi::Function::New(env, Shutdown));
  return exports;
}
