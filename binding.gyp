{
  "targets": [
    {
      "target_name": "node_pjsip",
      "sources": [
        "src/addon.cpp",
        "src/pjsip_wrapper.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "pjproject-2.15.1/pjlib/include",
        "pjproject-2.15.1/pjlib-util/include",
        "pjproject-2.15.1/pjmedia/include",
        "pjproject-2.15.1/pjnath/include",
        "pjproject-2.15.1/pjsip/include",
        "pjproject-2.15.1/pjsip-apps/src/pjsua"
      ],
      "conditions": [
        ["OS=='win'", {
          "libraries": [
            "ws2_32.lib",
            "advapi32.lib",
            "crypt32.lib",
            "iphlpapi.lib",
            "winmm.lib",
            "msvcrt.lib",
            "kernel32.lib",
            "user32.lib",
            "<!(node -e \"const fs=require('fs'); const path=require('path'); const libs = ['pjlib', 'pjlib-util', 'pjmedia', 'pjnath', 'pjsip', 'pjsua']; const arch = process.arch === 'x64' ? 'x86_64-x64' : 'i386-Win32'; const foundLibs = []; libs.forEach(lib => { const libPath = 'pjproject-2.15.1/lib/' + lib + '-' + arch + '-vc14-Release.lib'; if(fs.existsSync(libPath)) { foundLibs.push(path.resolve(libPath)); } else { console.log('lib not found: ' + libPath); } }); if(foundLibs.length === 0) { console.log('No PJSIP libraries found'); process.exit(1); } foundLibs.forEach(lib => console.log(lib));\")"
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": ["/std:c++17"]
            },
            "VCLinkerTool": {
              "AdditionalOptions": ["/NODEFAULTLIB:MSVCRT"]
            }
          }
        }],
        ["OS=='mac'", {
          "libraries": [
            "<!(node -e \"const fs=require('fs'); const path=require('path'); const arch = process.arch === 'x64' ? 'x86_64-x64' : 'i386-Win32'; const libPath = 'pjproject-2.15.1/lib/libpjproject-' + arch + '-vc14-Release.lib'; if(fs.existsSync(libPath)) { console.log(path.resolve(libPath)); } else { console.log('lib not found: ' + libPath); process.exit(1); }\")"
          ],
          "xcode_settings": {
            "MACOSX_DEPLOYMENT_TARGET": "15.0",
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++17"
          }
        }]
      ],
      "defines": [
        "NAPI_CPP_EXCEPTIONS",
        "PJ_IS_LITTLE_ENDIAN=1",
        "PJ_IS_BIG_ENDIAN=0",
        "PJ_WIN64=1",
        "PJ_M_X86_64=1",
        "WIN64=1",
        "WIN32=1",
        "_WIN32_WINNT=0x0601"
      ]
    }
  ]
}
