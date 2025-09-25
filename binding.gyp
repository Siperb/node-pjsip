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
            "<(module_root_dir)/pjproject-2.15.1/lib/pjsua2-lib-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjsua-lib-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjsip-ua-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjsip-simple-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjsip-core-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjmedia-codec-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjmedia-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjmedia-audiodev-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjmedia-videodev-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjnath-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjlib-util-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/pjlib-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libspeex-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libilbccodec-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libg7221codec-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libgsmcodec-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libsrtp-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libresample-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libwebrtc-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libyuv-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libbaseclasses-x86_64-x64-vc14-Release.lib",
            "<(module_root_dir)/pjproject-2.15.1/lib/libmilenage-x86_64-x64-vc14-Release.lib"
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": ["/std:c++17"]
            },
            "VCLinkerTool": {
              "AdditionalOptions": ["/NODEFAULTLIB:MSVCRT"],
              "AdditionalDependencies": ["Delayimp.lib"],
              "DelayLoadDLLs": ["node.exe"]
            }
          }
        }],
        ["OS=='mac'", {
          "libraries": [
            "<!(node -e \"const fs=require('fs'); const path=require('path'); const libDir = 'pjproject-2.15.1/lib'; const foundLibs = []; if(fs.existsSync(libDir)) { const libFiles = fs.readdirSync(libDir).filter(f => f.endsWith('.a') || f.endsWith('.dylib')); libFiles.forEach(lib => { foundLibs.push(path.resolve(libDir, lib)); }); } else { console.log('lib directory not found: ' + libDir); } if(foundLibs.length === 0) { console.log('No PJSIP libraries found in ' + libDir); process.exit(1); } foundLibs.forEach(lib => console.log(lib));\")"
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
