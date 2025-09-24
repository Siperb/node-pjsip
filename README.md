# pjsip-node

A Node.js addon that embeds **PJSIP** - similar to [baresip-node](https://github.com/Siperb/baresip-node).

## Overview

This project provides a professional Node.js wrapper for the PJSIP SIP stack, following the same architecture pattern as baresip-node but using PJSIP instead of baresip.

## Features

- ✅ **Real PJSIP Integration**: Uses actual PJSIP C++ library
- ✅ **SIP Registration**: Full SIP registration with digest authentication
- ✅ **Call Management**: Make, answer, and hangup calls
- ✅ **Event-Driven**: Comprehensive event system
- ✅ **Cross-Platform**: Windows and macOS support
- ✅ **TypeScript**: Full TypeScript support with type definitions
- ✅ **CMake Build**: Modern CMake-based build system

## Prerequisites

- Node.js 18+ (or 20+/22+)
- CMake 3.20+
- C++ toolchain:
  - **Windows**: MSVC 2022 Build Tools
  - **macOS**: Xcode Command Line Tools
  - **Linux**: build-essential

## Installation

```bash
npm install
npm run build
```

## Quick Start

```typescript
import PJSIP from 'pjsip-node';

const pjsip = new PJSIP();

// Initialize
await pjsip.init();

// Add account
const accountId = pjsip.addAccount({
  aor: "sip:user@domain.com",
  registrar: "sip:domain.com:5060",
  username: "user",
  password: "password"
});

// Register
pjsip.registerAccount(accountId);

// Event handlers
pjsip.on('registered', (accountId) => {
  console.log('Account registered:', accountId);
});

pjsip.on('incomingCall', (accountId, callId) => {
  console.log('Incoming call:', callId);
  pjsip.answerCall(accountId, callId, fromTag, toTag, cseqNum);
});
```

## API Reference

### PJSIP Class

#### Methods

- `init()`: Initialize PJSIP library
- `shutdown()`: Shutdown PJSIP library
- `addAccount(config)`: Add SIP account
- `removeAccount(accountId)`: Remove SIP account
- `registerAccount(accountId)`: Register account
- `unregisterAccount(accountId)`: Unregister account
- `makeCall(accountId, destination)`: Make a call
- `answerCall(accountId, callId, fromTag, toTag, cseqNum)`: Answer call
- `hangupCall(accountId, callId, fromTag, toTag, cseqNum)`: Hangup call
- `getAccountInfo(accountId)`: Get account information
- `getAccounts()`: Get all accounts
- `getVersion()`: Get PJSIP version
- `getLocalIP()`: Get local IP address
- `getBoundPort()`: Get bound port

#### Events

- `initialized`: PJSIP initialized
- `shutdown`: PJSIP shutdown
- `accountAdded`: Account added
- `accountRemoved`: Account removed
- `registering`: Account registering
- `registered`: Account registered
- `unregistering`: Account unregistering
- `unregistered`: Account unregistered
- `callInitiated`: Call initiated
- `incomingCall`: Incoming call
- `callAnswered`: Call answered
- `callHangup`: Call hung up
- `error`: Error occurred

## Build System

This project uses CMake for building the native addon, similar to baresip-node:

```bash
# Configure
npm run configure

# Build
npm run build

# Clean
npm run clean
```

## Project Structure

```
pjsip-node/
├── src/
│   ├── index.ts          # TypeScript wrapper
│   ├── native.ts         # Native module loader
│   ├── addon.cpp         # C++ addon entry point
│   └── pjsip_wrapper.cpp # PJSIP wrapper implementation
├── pjproject-2.15.1/     # PJSIP source (submodule)
├── build/                # Build output
├── dist/                 # TypeScript output
├── CMakeLists.txt        # CMake configuration
├── package.json          # Node.js package
└── tsconfig.json         # TypeScript configuration
```

## Development

### Prerequisites

1. **Install Visual Studio Build Tools 2022** (Windows)
2. **Install CMake 3.20+**
3. **Install Node.js 18+**

### Build Steps

1. **Clone and install dependencies:**
   ```bash
   git clone <repository>
   cd pjsip-node
   npm install
   ```

2. **Build PJSIP library:**
   ```bash
   cd pjproject-2.15.1/pjproject-2.15.1
   # Follow PJSIP build instructions
   ```

3. **Build the addon:**
   ```bash
   npm run build
   ```

4. **Test:**
   ```bash
   npm test
   ```

## Architecture

This project follows the same architecture as [baresip-node](https://github.com/Siperb/baresip-node):

- **Native Layer**: C++ addon using Node-API
- **PJSIP Integration**: Real PJSIP library embedded
- **TypeScript Wrapper**: High-level JavaScript/TypeScript API
- **CMake Build**: Modern build system
- **Event-Driven**: Comprehensive event system

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