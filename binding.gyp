{
  "targets": [
    {
      "target_name": "node_pjsip",
      "sources": [
        "src/addon.cpp",
        "src/pjsip_wrapper.cpp"
      ],
      "include_dirs": [
        "node_modules/node-addon-api",
        "deps/pjsip/build/include"
      ],
      "conditions": [
        ["OS=='win'", {
          "libraries": [
            "ws2_32.lib"
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1
            }
          }
        }],
        ["OS=='mac'", {
          "libraries": [
            "<(module_root_dir)/deps/pjsip/build/lib/libpj-arm-apple-darwin24.5.0.a",
            "<(module_root_dir)/deps/pjsip/build/lib/libpjlib-util-arm-apple-darwin24.5.0.a",
            "<(module_root_dir)/deps/pjsip/build/lib/libpjnath-arm-apple-darwin24.5.0.a",
            "<(module_root_dir)/deps/pjsip/build/lib/libpjsip-arm-apple-darwin24.5.0.a",
            "<(module_root_dir)/deps/pjsip/build/lib/libpjsip-ua-arm-apple-darwin24.5.0.a",
            "<(module_root_dir)/deps/pjsip/build/lib/libpjsua-arm-apple-darwin24.5.0.a",
            "<(module_root_dir)/deps/pjsip/build/lib/libpjsua2-arm-apple-darwin24.5.0.a"
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
        "PJ_IS_BIG_ENDIAN=0"
      ]
    }
  ]
}
