# pjsip-node

A Node.js addon that embeds **PJSIP** with complete session management - similar to [baresip-node](https://github.com/Siperb/baresip-node).

## Overview

This project provides a professional Node.js wrapper for the PJSIP SIP stack with advanced session management capabilities. It includes all necessary PJSIP libraries bundled, so users don't need to install any additional dependencies.

## Features

- ✅ **Real PJSIP Integration**: Uses actual PJSIP C++ library
- ✅ **Complete Session Management**: JavaScript-layer session tracking with unique IDs
- ✅ **SIP Registration**: Full SIP registration with digest authentication
- ✅ **Call Management**: Make, answer, and hangup calls with session objects
- ✅ **Callback-Based System**: Clean callback system (no event stacking)
- ✅ **Configuration Separation**: Separate configs for Init, Register, and Invite
- ✅ **Cross-Platform**: Windows and macOS support
- ✅ **Bundled Dependencies**: All PJSIP libraries included - no external dependencies
- ✅ **Electron Ready**: Perfect for Electron Phone UI integration
- ✅ **Client-Required API**: Implements exact API structure as specified by client

## Prerequisites

- Node.js 18+ (or 20+/22+)
- C++ toolchain:
  - **Windows**: MSVC 2022 Build Tools (for building from source)
  - **macOS**: Xcode Command Line Tools (for building from source)

## Installation

### For End Users (Pre-built Package)
```bash
npm install pjsip-node
```
**No additional dependencies required!** All PJSIP libraries are bundled.

### For Developers (Build from Source)
```bash
git clone <repository>
cd pjsip-node
npm install
npm run build
```

## Quick Start

```javascript
const api = require('pjsip-node');

// Set up callbacks (single-assignment system)
api.setCallbacks({
  onReady: (info) => {
    console.log('✅ PJSIP initialized');
    // Register account after initialization
    const reg = api.Register({
      username: "1113",
      password: "password",
      domain: "pbx.sheerbit.com",
      registrar: "sip:pbx.sheerbit.com:5060"
    });
    if (reg.ok) {
      console.log('✅ Account registered with ID:', reg.data);
    }
  },
  
  onRegisterSuccess: (e) => {
    console.log('✅ Registration successful for:', e.data);
    // Make an outgoing call
    const inviteRes = api.Invite({
      to: "sip:8080@pbx.sheerbit.com"
    });
    if (inviteRes.ok) {
      console.log('✅ INVITE sent with call ID:', inviteRes.data);
    }
  },
  
  onInvite: (e) => {
    console.log('📞 Incoming INVITE from:', e.data.caller);
    // Answer incoming calls
    const answerRes = api.Answer({ callId: e.data.callId });
    if (answerRes.ok) {
      console.log('✅ Call answered');
    }
  },
  
  onCallState: (e) => {
    console.log(`📞 Call ${e.data.callId} state: ${e.data.state}`);
    if (e.data.state === 'CONFIRMED') {
      console.log('🔊 Media connected!');
    }
  },
  
  onError: (err) => {
    console.error('❌ Error:', err);
  }
});

// Initialize PJSIP with global configuration
const initResult = api.Init({
  logLevel: 4,
  userAgent: "PJSIP Node.js Wrapper",
  maxCalls: 10,
  localPort: 5060,
  transport: "UDP"
});

if (!initResult.ok) {
  console.error('❌ Initialization failed:', initResult.message);
  process.exit(1);
}

// Shutdown when done
setTimeout(() => {
  api.Shutdown();
}, 30000);
```

## API Reference

### Core Functions

#### `Init(config)` - Initialize PJSIP
```javascript
const result = api.Init({
  logLevel: 4,           // 0-6 (0=disabled, 6=debug)
  userAgent: "My App",   // User agent string
  maxCalls: 10,         // Maximum concurrent calls
  localPort: 5060,      // Local SIP port
  transport: "UDP"      // Transport type
});
```

#### `Register(config)` - Register SIP Account
```javascript
const result = api.Register({
  username: "user",                    // SIP username
  password: "password",                // SIP password
  domain: "domain.com",               // SIP domain
  registrar: "sip:domain.com:5060",   // Registrar URI
  displayName: "My Name",             // Display name
  expires: 3600                       // Registration expiry (seconds)
});
```

#### `Invite(config)` - Make Outgoing Call
```javascript
const result = api.Invite({
  to: "sip:8080@domain.com",  // Destination URI
  withVideo: false,           // Enable video
  audioCodec: "PCMU"         // Preferred audio codec
});
```

### Call Management

- `Answer(config)` - Answer incoming call
- `Bye(config)` - Hangup call
- `Hold(config)` - Hold call
- `Unhold(config)` - Unhold call
- `Mute(config)` - Mute audio
- `Unmute(config)` - Unmute audio
- `Dtmf(config)` - Send DTMF tones

### Utility Functions

- `GetVersion()` - Get PJSIP version
- `GetLocalIP()` - Get local IP address
- `GetBoundPort()` - Get bound SIP port
- `GetRegistrationStatus()` - Get account registration status
- `ListCalls()` - List active calls
- `Shutdown()` - Shutdown PJSIP

### Callback System

The API uses a single-assignment callback system (no event stacking):

```javascript
api.setCallbacks({
  // Library events
  onReady: (info) => { /* PJSIP initialized */ },
  onError: (err) => { /* Error occurred */ },
  onShutdown: () => { /* PJSIP shutdown */ },
  
  // Registration events
  onRegisterStart: (e) => { /* Registration started */ },
  onRegisterSuccess: (e) => { /* Registration successful */ },
  onRegisterFailed: (e) => { /* Registration failed */ },
  onUnregister: (e) => { /* Unregistered */ },
  
  // Call events
  onInvite: (e) => { /* Incoming INVITE */ },
  onCallState: (e) => { /* Call state changed */ },
  onBye: (e) => { /* Call ended */ },
  onHold: (e) => { /* Call held */ },
  onUnhold: (e) => { /* Call unheld */ },
  onDtmf: (e) => { /* DTMF received */ },
  
  // Media events
  onMediaState: (e) => { /* Media state changed */ },
  onStats: (e) => { /* Media statistics */ }
});
```

## Project Evolution & Chat Summary

This project has been developed through extensive collaboration to meet specific client requirements:

### Key Requirements Implemented

1. **JavaScript Wrapper Architecture**: Replaced TypeScript with a pure JavaScript wrapper (`lib/index.js`) that loads the native addon and provides a clean API interface.

2. **Callback-Based System**: Implemented single-assignment callbacks instead of EventEmitter to prevent event stacking and ensure only one handler per event type.

3. **API Configuration Separation**: 
   - `Init(options)` - Global PJSIP configuration (log levels, bindings, etc.)
   - `Register(options)` - Account-specific configuration (username, password, etc.)
   - `Invite(options)` - Session-specific configuration (To, From, WithVideo, etc.)

4. **Session Management**: JavaScript-layer session tracking with unique IDs for easy integration with Electron Phone UI.

5. **Bundled Dependencies**: All PJSIP binary libraries (`.lib` files) are included for distribution, ensuring users don't need to install any external dependencies.

6. **Result Normalization**: All API functions return a consistent `{ ok: boolean, code: number, message: string, data?: any }` format.

### Technical Implementation

- **Native Addon**: C++ addon using N-API for Node.js integration
- **PJSIP Integration**: Real PJSIP 2.15.1 library embedded
- **Cross-Platform**: Windows (x64) and macOS support
- **Build System**: Node-gyp for native addon compilation
- **Error Handling**: Comprehensive error handling with normalized results

### Project Cleanup

The project has been optimized by removing unnecessary files:
- Removed old TypeScript files (`src/index.ts`, `src/native.ts`)
- Removed TypeScript build artifacts (`dist/` directory)
- Removed TypeScript configuration (`tsconfig.json`)
- Updated `package.json` to remove TypeScript dependencies
- Kept only essential working files

## Bundled Dependencies

This package includes all necessary PJSIP libraries and dependencies:

### PJSIP Core Libraries
- `pjsua2-lib-x86_64-x64-vc14-Release.lib` - PJSIP User Agent Library v2
- `pjsua-lib-x86_64-x64-vc14-Release.lib` - PJSIP User Agent Library
- `pjsip-ua-x86_64-x64-vc14-Release.lib` - PJSIP User Agent Layer
- `pjsip-simple-x86_64-x64-vc14-Release.lib` - PJSIP Simple Framework
- `pjsip-core-x86_64-x64-vc14-Release.lib` - PJSIP Core

### Media Libraries
- `pjmedia-x86_64-x64-vc14-Release.lib` - PJMedia Framework
- `pjmedia-codec-x86_64-x64-vc14-Release.lib` - Media Codecs
- `pjmedia-audiodev-x86_64-x64-vc14-Release.lib` - Audio Devices
- `pjmedia-videodev-x86_64-x64-vc14-Release.lib` - Video Devices

### Network Libraries
- `pjnath-x86_64-x64-vc14-Release.lib` - NAT Traversal
- `pjlib-util-x86_64-x64-vc14-Release.lib` - PJLIB Utilities
- `pjlib-x86_64-x64-vc14-Release.lib` - PJLIB Core

### Third-Party Codecs
- `libspeex-x86_64-x64-vc14-Release.lib` - Speex Codec
- `libilbccodec-x86_64-x64-vc14-Release.lib` - iLBC Codec
- `libg7221codec-x86_64-x64-vc14-Release.lib` - G.722.1 Codec
- `libgsmcodec-x86_64-x64-vc14-Release.lib` - GSM Codec
- `libsrtp-x86_64-x64-vc14-Release.lib` - SRTP Security
- `libresample-x86_64-x64-vc14-Release.lib` - Audio Resampling
- `libwebrtc-x86_64-x64-vc14-Release.lib` - WebRTC Components
- `libyuv-x86_64-x64-vc14-Release.lib` - YUV Processing
- `libbaseclasses-x86_64-x64-vc14-Release.lib` - Base Classes
- `libmilenage-x86_64-x64-vc14-Release.lib` - Milenage Algorithm

**No external dependencies required!** Users can install and use this package without installing any additional libraries.

## Build System

This project uses node-gyp for building the native addon:

```bash
# Configure
npm run configure

# Build
npm run build

# Clean
npm run clean

# Test
npm test
```

## Project Structure

```
pjsip-node/
├── .git/                    # Git repository
├── .github/                  # GitHub workflows
├── build/                    # Native build artifacts
├── lib/                      # JavaScript wrapper (main entry point)
│   └── index.js             # Client-required JS wrapper
├── node_modules/            # Dependencies
├── pjproject-2.15.1/        # PJSIP source
├── src/                     # C++ source files
│   ├── addon.cpp           # N-API addon entry point
│   ├── pjsip_wrapper.cpp   # Main PJSIP wrapper
│   └── pjsip_wrapper.h     # Header file
├── *.lib                    # PJSIP binary libraries (bundled)
├── binding.gyp             # Node-gyp configuration
├── package.json            # Package configuration
├── test.js      # Working test file
└── README.md               # This file
```

## Development

### Prerequisites

1. **Install Visual Studio Build Tools 2022** (Windows)
2. **Install Node.js 18+**
3. **Install Python 3.x** (for node-gyp)

### Build Steps

1. **Clone and install dependencies:**
   ```bash
   git clone <repository>
   cd pjsip-node
   npm install
   ```

2. **Build the addon:**
   ```bash
   npm run build
   ```

3. **Test:**
   ```bash
   npm test
   ```

## Architecture

This project follows a clean architecture:

- **Native Layer**: C++ addon using N-API
- **PJSIP Integration**: Real PJSIP library embedded
- **JavaScript Wrapper**: High-level JavaScript API
- **Node-gyp Build**: Modern build system
- **Callback-Driven**: Single-assignment callback system

## Testing

The project includes a comprehensive test file (`test.js`) that demonstrates:
- PJSIP initialization
- Account registration with authentication
- Outgoing call initiation
- Call state management
- Proper shutdown and cleanup

Run the test:
```bash
node test.js
```

## License

MIT License

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## References

- [baresip-node](https://github.com/Siperb/baresip-node) - Reference implementation
- [PJSIP Documentation](https://www.pjsip.org/)
- [Node-API Documentation](https://nodejs.org/api/n-api.html)