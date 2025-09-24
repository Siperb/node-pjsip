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
        "pjsip-install/include",
        "pjsip-install/include/pjlib",
        "pjsip-install/include/pjlib-util",
        "pjsip-install/include/pjmedia",
        "pjsip-install/include/pjnath",
        "pjsip-install/include/pjsip"
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
            "pjsip-install/lib/libpjproject-x86_64-x64-vc14-Release.lib"
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
            "pjsip-install/lib/libpjproject-x86_64-x64-vc14-Release.lib"
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
