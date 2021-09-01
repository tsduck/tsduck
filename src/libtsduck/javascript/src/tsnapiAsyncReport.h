#pragma once

#include <napi.h>

#include "tsAsyncReport.h"
#include "tsUString.h"

/**
 * A wrapper class for C++ AsyncReport.
 */
namespace ts {
    namespace napi {

        class AsyncReport : public Napi::ObjectWrap<AsyncReport>, public ts::AsyncReport {

          public:
            static Napi::Object Init(Napi::Env env, Napi::Object exports);
            AsyncReport(const Napi::CallbackInfo& info);
            Napi::Value GetLog(const Napi::CallbackInfo& info);
            void ClearLog(const Napi::CallbackInfo& info);

          protected:
            void writeLog(int severity, const ts::UString& msg);

          private:
            static Napi::FunctionReference constructor;
            std::vector<std::string> _log;
        };
    }
}