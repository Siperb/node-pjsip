#!/usr/bin/env node

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

console.log('🔨 Building node-pjsip native addon...\n');

try {
  // Check if node-gyp is available
  try {
    execSync('node-gyp --version', { stdio: 'pipe' });
  } catch (error) {
    console.log('📦 Installing node-gyp...');
    execSync('npm install -g node-gyp', { stdio: 'inherit' });
  }

  // Clean previous builds
  console.log('🧹 Cleaning previous builds...');
  if (fs.existsSync('build')) {
    fs.rmSync('build', { recursive: true, force: true });
  }

  // Configure
  console.log('⚙️  Configuring build...');
  execSync('node-gyp configure', { stdio: 'inherit' });

  // Build
  console.log('🔨 Building native addon...');
  execSync('node-gyp build', { stdio: 'inherit' });

  console.log('\n✅ Build completed successfully!');
  console.log('\n📋 Next steps:');
  console.log('1. Run tests: node test.js');
  console.log('2. Check lib/ directory for JavaScript wrappers');
  console.log('3. Use in your React Electron app');

} catch (error) {
  console.error('\n❌ Build failed:', error.message);
  console.log('\n🔧 Troubleshooting:');
  console.log('1. Make sure you have Python 3.x installed');
  console.log('2. Install build tools for your platform:');
  console.log('   - Windows: Visual Studio Build Tools');
  console.log('   - macOS: Xcode Command Line Tools');
  console.log('   - Linux: build-essential package');
  console.log('3. Check that PJSIP dependencies are available');
  process.exit(1);
}
