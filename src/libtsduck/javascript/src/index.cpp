#include <napi.h>

#include "tsnapiTSProcessor.h"
#include "tsnapiAsyncReport.h"
#include "tsnapiCustomReport.h"
#include "tsnapiInfo.cc"

/**
 * Init function for setting the export key to JS
 */
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    exports.Set(
        Napi::String::New(env, "getVersion"),
        Napi::Function::New(env, ts::napi::getVersion)
    );
    ts::napi::AsyncReport::Init(env, exports);
    ts::napi::TSProcessor::Init(env, exports);
    ts::napi::CustomReport::Init(env, exports);
    return exports;
}

NODE_API_MODULE(tsduck, InitAll)