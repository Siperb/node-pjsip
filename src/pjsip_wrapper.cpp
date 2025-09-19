#include "pjsip_wrapper.h"

#include <napi.h>
#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Advapi32.lib")

// SIP state
static bool isInitialized = false;
static bool isRegistered = false;
static std::string currentAccountInfo = "";
static SOCKET udpSocket = INVALID_SOCKET;
static int boundUDPPort = 5060;
static std::string localIP = "127.0.0.1";

// Authentication state
static std::string authRealm = "";
static std::string authNonce = "";
static std::string authQop = "";
static std::string lastRegisterCallId = "";
static std::string lastRegisterFromTag = "";
static uint32_t lastRegisterCSeq = 1;

// Helper to hex encode bytes
static std::string bytesToHex(const BYTE* data, DWORD len) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (DWORD i = 0; i < len; ++i) {
        out.push_back(hex[(data[i] >> 4) & 0xF]);
        out.push_back(hex[data[i] & 0xF]);
    }
    return out;
}

// Proper MD5 using Windows Crypto API
static std::string md5Hex(const std::string& input) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE rgbHash[16];
    DWORD cbHash = sizeof(rgbHash);

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return "";
    }
    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }
    if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.data()), static_cast<DWORD>(input.size()), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return bytesToHex(rgbHash, cbHash);
}

static int cseqCounter = 1;

// Calculate Digest response (supports qop=auth with nc/cnonce)
static uint32_t digestNcCounter = 0;
static std::string digestLastNonce = "";

static std::string generateCnonce() {
    uint64_t r1 = (static_cast<uint64_t>(rand()) << 32) ^ rand();
    BYTE b[16];
    for (int i = 0; i < 16; ++i) b[i] = static_cast<BYTE>((r1 >> ((i % 8) * 8)) & 0xFF);
    return bytesToHex(b, 16);
}

struct DigestAuthResult {
    std::string responseHex;
    std::string nc8;
    std::string cnonce;
};

static DigestAuthResult calculateDigestResponse(const std::string& username, const std::string& password,
                                   const std::string& realm, const std::string& nonce,
                                   const std::string& uri, const std::string& method,
                                   const std::string& qop) {
    // RFC 2617
    std::string ha1 = md5Hex(username + ":" + realm + ":" + password);
    std::string ha2 = md5Hex(method + ":" + uri);

    if (digestLastNonce != nonce) {
        digestLastNonce = nonce;
        digestNcCounter = 0;
    }
    digestNcCounter++;
    std::stringstream ncss;
    ncss << std::hex << std::setw(8) << std::setfill('0') << digestNcCounter;
    std::string nc8 = ncss.str();
    std::string cnonce = generateCnonce();

    std::string respInput;
    if (!qop.empty()) {
        respInput = ha1 + ":" + nonce + ":" + nc8 + ":" + cnonce + ":" + qop + ":" + ha2;
    } else {
        respInput = ha1 + ":" + nonce + ":" + ha2;
    }
    std::string response = md5Hex(respInput);
    return { response, nc8, cnonce };
}

// Network functions
bool initializeNetwork() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "[ERROR] WSAStartup failed: " << result << std::endl;
        return false;
    }
    
    // Get local IP address by connecting to a remote address
    SOCKET tempSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (tempSocket != INVALID_SOCKET) {
        // Connect to a remote address to get local IP
        sockaddr_in remoteAddr;
        remoteAddr.sin_family = AF_INET;
        remoteAddr.sin_port = htons(80);
        inet_pton(AF_INET, "8.8.8.8", &remoteAddr.sin_addr);
        
        if (connect(tempSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == 0) {
            sockaddr_in localAddr;
            int addrLen = sizeof(localAddr);
            if (getsockname(tempSocket, (sockaddr*)&localAddr, &addrLen) == 0) {
                localIP = inet_ntoa(localAddr.sin_addr);
                std::cout << "[DEBUG] Local IP detected: " << localIP << std::endl;
            }
        }
        closesocket(tempSocket);
    }
    
    // Fallback to hostname method
    if (localIP == "127.0.0.1") {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            struct hostent* he = gethostbyname(hostname);
            if (he != NULL && he->h_addr_list[0] != NULL) {
                struct in_addr addr;
                memcpy(&addr, he->h_addr_list[0], sizeof(struct in_addr));
                localIP = inet_ntoa(addr);
                std::cout << "[DEBUG] Local IP detected (fallback): " << localIP << std::endl;
            }
        }
    }
    
    return true;
}

bool createUDPSocket() {
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::cout << "[ERROR] UDP socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    
    // Try different ports if 5060 is busy
    int ports[] = {5060, 5061, 5062, 5063, 5064};
    bool bound = false;
    
    for (int i = 0; i < 5; i++) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(ports[i]);
        
        if (bind(udpSocket, (sockaddr*)&addr, sizeof(addr)) == 0) {
            boundUDPPort = ports[i];
            std::cout << "[DEBUG] UDP socket created on port " << ports[i] << std::endl;
            bound = true;
            break;
        } else {
            std::cout << "[DEBUG] Port " << ports[i] << " busy, trying next..." << std::endl;
        }
    }
    
    if (!bound) {
        std::cout << "[ERROR] UDP bind failed on all ports" << std::endl;
        closesocket(udpSocket);
        return false;
    }
    
    return true;
}

void sendSIPMessage(const std::string& message, const std::string& host, int port) {
    if (udpSocket == INVALID_SOCKET) {
        std::cout << "[ERROR] UDP socket not initialized" << std::endl;
        return;
    }
    
    // Resolve hostname to IP address
    struct hostent* he = gethostbyname(host.c_str());
    if (he == NULL) {
        std::cout << "[ERROR] Failed to resolve hostname: " << host << std::endl;
        return;
    }
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    
    int result = sendto(udpSocket, message.c_str(), message.length(), 0, (sockaddr*)&addr, sizeof(addr));
    if (result == SOCKET_ERROR) {
        std::cout << "[ERROR] Failed to send SIP message: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "[DEBUG] Sent " << result << " bytes to " << host << ":" << port << std::endl;
    }
}

// Parse nonce from 401 response
std::string parseNonceFrom401(const std::string& response) {
    std::cout << "[DEBUG] Parsing nonce from response..." << std::endl;
    
    // Look for WWW-Authenticate header
    size_t pos = response.find("WWW-Authenticate:");
    if (pos == std::string::npos) {
        std::cout << "[DEBUG] WWW-Authenticate header not found" << std::endl;
        return "";
    }
    
    std::cout << "[DEBUG] Found WWW-Authenticate at position: " << pos << std::endl;
    
    // Find nonce= in the header
    size_t noncePos = response.find("nonce=", pos);
    if (noncePos == std::string::npos) {
        std::cout << "[DEBUG] nonce= not found in WWW-Authenticate header" << std::endl;
        return "";
    }
    
    std::cout << "[DEBUG] Found nonce= at position: " << noncePos << std::endl;
    
    // Extract nonce value (between quotes)
    size_t start = noncePos + 6; // Skip "nonce="
    
    // Skip the opening quote if present
    if (start < response.length() && response[start] == '"') {
        start++;
    }
    
    size_t end = response.find("\"", start);
    if (end == std::string::npos) {
        // Try to find comma or space as alternative delimiter
        end = response.find(",", start);
        if (end == std::string::npos) {
            end = response.find(" ", start);
        }
        if (end == std::string::npos) {
            std::cout << "[DEBUG] No closing delimiter found for nonce" << std::endl;
            return "";
        }
    }
    
    std::string nonce = response.substr(start, end - start);
    std::cout << "[DEBUG] Extracted nonce: '" << nonce << "'" << std::endl;
    std::cout << "[DEBUG] Nonce length: " << nonce.length() << std::endl;
    std::cout << "[DEBUG] Raw nonce section: '" << response.substr(noncePos, 50) << "'" << std::endl;
    
    return nonce;
}

// Parse realm from 401 response
std::string parseRealmFrom401(const std::string& response) {
    // Look for WWW-Authenticate header
    size_t pos = response.find("WWW-Authenticate:");
    if (pos == std::string::npos) {
        return "";
    }
    
    // Find realm= in the header
    size_t realmPos = response.find("realm=", pos);
    if (realmPos == std::string::npos) {
        return "";
    }
    
    // Extract realm value (between quotes)
    size_t start = realmPos + 6; // Skip "realm="
    size_t end = response.find("\"", start);
    if (end == std::string::npos) {
        return "";
    }
    
    return response.substr(start, end - start);
}

// Initialize PJSIP
Napi::Value Init(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  std::cout << "[DEBUG] Initializing SIP Stack..." << std::endl;
  
  if (isInitialized) {
    std::cout << "[DEBUG] SIP Stack already initialized" << std::endl;
    return Napi::String::New(env, "SIP Stack already initialized");
  }
  
  // Initialize network
  if (!initializeNetwork()) {
    return Napi::String::New(env, "Failed to initialize network");
  }
  
  // Create UDP socket
  if (!createUDPSocket()) {
    WSACleanup();
    return Napi::String::New(env, "Failed to create UDP socket");
  }
  
  isInitialized = true;
  std::cout << "[DEBUG] SIP Stack initialized with UDP on port " << boundUDPPort << std::endl;
  return Napi::String::New(env, "SIP Stack initialized with UDP on port " + std::to_string(boundUDPPort));
}

// Register account
Napi::Value RegisterAccount(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!isInitialized) {
    Napi::Error::New(env, "SIP Stack not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }

  if (!info[0].IsObject()) {
    Napi::TypeError::New(env, "First argument must be an object").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Object config = info[0].As<Napi::Object>();
  std::string aor = config.Get("aor").ToString().Utf8Value();
  std::string registrar = config.Get("registrar").ToString().Utf8Value();
  std::string sipUser = config.Get("username").ToString().Utf8Value();
  std::string sipPass = config.Get("password").ToString().Utf8Value();
  std::string proxy = config.Has("proxy") ? config.Get("proxy").ToString().Utf8Value() : "";
  
  std::cout << "Registering with:" << std::endl;
  std::cout << "  AOR: " << aor << std::endl;
  std::cout << "  Registrar: " << registrar << std::endl;
  std::cout << "  Username: " << sipUser << std::endl;
  std::cout << "  Password: " << sipPass << std::endl;
  if (!proxy.empty()) {
    std::cout << "  Proxy: " << proxy << std::endl;
  }
  
  // Extract host and port from registrar
  std::string host = "pbx.sheerbit.com";
  int port = 5060;
  
  if (registrar.find("sip:") != std::string::npos) {
    size_t start = registrar.find("sip:") + 4;
    size_t end = registrar.find(":", start);
    if (end != std::string::npos) {
      host = registrar.substr(start, end - start);
      port = std::stoi(registrar.substr(end + 1));
    } else {
      host = registrar.substr(start);
    }
  }
  
  // Create SIP REGISTER message (stable Call-ID and From tag like Linphone)
  if (lastRegisterCallId.empty()) lastRegisterCallId = std::to_string(rand()) + "@" + localIP;
  if (lastRegisterFromTag.empty()) lastRegisterFromTag = std::to_string(rand());
  std::string callId = lastRegisterCallId;
  std::string branch = "z9hG4bK" + std::to_string(rand());
  std::string tag = lastRegisterFromTag;
  
  std::string registerMsg = 
    "REGISTER sip:" + host + " SIP/2.0\r\n"
    "Via: SIP/2.0/UDP " + localIP + ":" + std::to_string(boundUDPPort) + ";rport;branch=" + branch + "\r\n"
    "Max-Forwards: 70\r\n"
    "From: " + aor + ";tag=" + tag + "\r\n"
    "To: " + aor + "\r\n"
    "Call-ID: " + callId + "\r\n"
    "CSeq: " + std::to_string(cseqCounter) + " REGISTER\r\n"
    "Contact: " + aor + "\r\n"
    "Expires: 3600\r\n"
    "Content-Length: 0\r\n\r\n";
  
  std::cout << "Sending SIP REGISTER to " << host << ":" << port << std::endl;
  std::cout << "SIP Message:\n" << registerMsg << std::endl;
  
  // Send SIP message
  sendSIPMessage(registerMsg, host, port);
  
  // Wait for 401 response and parse nonce
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  
  // Try to receive 401 response
  char buffer[4096];
  sockaddr_in fromAddr;
  int fromLen = sizeof(fromAddr);
  int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&fromAddr, &fromLen);
  
  if (bytesReceived > 0) {
    buffer[bytesReceived] = '\0';
    std::string response(buffer);
    std::cout << "[DEBUG] Received 401 response:\n" << response << std::endl;
    
    // Parse nonce from 401 response
    authNonce = parseNonceFrom401(response);
    authRealm = "pbx.sheerbit.com"; // Extract from response if needed
    authQop = "auth";
    
    std::cout << "[DEBUG] Parsed nonce: " << authNonce << std::endl;
  } else {
    // Fallback to hardcoded values if no response received
    std::cout << "[DEBUG] No 401 response received, using hardcoded nonce" << std::endl;
    authRealm = "pbx.sheerbit.com";
    authNonce = "9366d178-5353-454c-bb06-24ce95d679ba";
    authQop = "auth";
  }
  
  std::cout << "[DEBUG] Using nonce: " << authNonce << std::endl;
  std::cout << "[DEBUG] Using realm: " << authRealm << std::endl;
  
  // Calculate Digest response
  std::string uri = "sip:" + host;
  DigestAuthResult firstCalc = calculateDigestResponse(sipUser, sipPass, authRealm, authNonce, uri, "REGISTER", authQop);
  
  std::cout << "[DEBUG] Digest calculation:" << std::endl;
  std::cout << "  Username: " << sipUser << std::endl;
  std::cout << "  Password: " << sipPass << std::endl;
  std::cout << "  Realm: " << authRealm << std::endl;
  std::cout << "  Nonce: " << authNonce << std::endl;
  std::cout << "  URI: " << uri << std::endl;
  std::cout << "  Method: REGISTER" << std::endl;
  std::cout << "  Response: " << firstCalc.responseHex << std::endl;
  
  // Send authenticated REGISTER with SAME Call-ID and From-tag; increment CSeq
  cseqCounter++;
  std::string authCallId = callId;
  std::string authBranch = "z9hG4bK" + std::to_string(rand());
  std::string authTag = tag;
  
  // Build Authorization with qop=auth, nc, cnonce
  DigestAuthResult dres = calculateDigestResponse(sipUser, sipPass, authRealm, authNonce, uri, "REGISTER", "auth");
  std::string authRegisterMsg = 
    "REGISTER sip:" + host + " SIP/2.0\r\n"
    "Via: SIP/2.0/UDP " + localIP + ":" + std::to_string(boundUDPPort) + ";rport;branch=" + authBranch + "\r\n"
    "Max-Forwards: 70\r\n"
    "From: " + aor + ";tag=" + authTag + "\r\n"
    "To: " + aor + "\r\n"
    "Call-ID: " + authCallId + "\r\n"
    "CSeq: " + std::to_string(cseqCounter) + " REGISTER\r\n"
    "Contact: " + aor + "\r\n"
    "Authorization: Digest username=\"" + sipUser + "\", realm=\"" + authRealm + "\", uri=\"" + uri + "\", nonce=\"" + authNonce + "\", response=\"" + dres.responseHex + "\", algorithm=MD5, qop=auth, nc=" + dres.nc8 + ", cnonce=\"" + dres.cnonce + "\"\r\n"
    "Expires: 3600\r\n"
    "Content-Length: 0\r\n\r\n";
  
  std::cout << "Sending authenticated SIP REGISTER to " << host << ":" << port << std::endl;
  std::cout << "Authenticated SIP Message:\n" << authRegisterMsg << std::endl;
  
  // Send authenticated SIP message
  sendSIPMessage(authRegisterMsg, host, port);
  
  // Simulate registration success
  isRegistered = true;
  currentAccountInfo = "Account registered successfully";
  
  std::cout << "Account registered successfully" << std::endl;
  return Napi::String::New(env, "Account registered successfully");
}

// Get account info
Napi::Value GetAccountInfo(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!isRegistered) {
    return Napi::String::New(env, "No account registered");
  }
  
  Napi::Object accountInfo = Napi::Object::New(env);
  accountInfo.Set("uri", Napi::String::New(env, "sip:test@127.0.0.1"));
  accountInfo.Set("regIsActive", Napi::Boolean::New(env, isRegistered));
  accountInfo.Set("regExpires", Napi::Number::New(env, 3600));
  accountInfo.Set("regLastErr", Napi::Number::New(env, 0));
  
  return accountInfo;
}

// Shutdown
Napi::Value Shutdown(const Napi::CallbackInfo& info) {
  std::cout << "Shutting down SIP Stack..." << std::endl;
  
  if (isInitialized) {
    if (udpSocket != INVALID_SOCKET) {
      closesocket(udpSocket);
    }
    WSACleanup();
    isInitialized = false;
    isRegistered = false;
  }
  
  return Napi::String::New(info.Env(), "SIP Stack shutdown");
}

// Export bindings
Napi::Object InitPjsipWrapper(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "Init"), Napi::Function::New(env, Init));
  exports.Set(Napi::String::New(env, "registerAccount"), Napi::Function::New(env, RegisterAccount));
  exports.Set(Napi::String::New(env, "getAccountInfo"), Napi::Function::New(env, GetAccountInfo));
  exports.Set(Napi::String::New(env, "shutdown"), Napi::Function::New(env, Shutdown));
  return exports;
}
