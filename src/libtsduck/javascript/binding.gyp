{
    "targets": [{
        "target_name": "tsduck",
        "cflags!": [ "-fno-exceptions" ],
        "cflags_cc!": [ "-fno-exceptions" ],
        "cflags_cc": [ "$(shell tsconfig --cflags)" ],
        "sources": [
            "src/index.cpp",
            "src/tsnapiAsyncReport.cpp",
            "src/tsnapiCustomReport.cpp",
            "src/tsnapiTSProcessor.cpp"
        ],
        'include_dirs': [
            "<!@(node -p \"require('node-addon-api').include\")"
        ],
        'dependencies': [
            "<!(node -p \"require('node-addon-api').gyp\")"
        ],
        'libraries': [
            "$(shell tsconfig --libs)"
        ],
        'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ]
    }]
}