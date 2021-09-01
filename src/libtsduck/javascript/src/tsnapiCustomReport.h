#pragma once

#include <napi.h>

#include "tsAsyncReport.h"
#include "tsUString.h"

#include "tsnapiAsyncReport.h"

/**
 * A customized wrapper class for C++ AsyncReport, which separates the log messages into different logs.
 */
namespace ts {
    namespace napi {

        class CustomReport : public Napi::ObjectWrap<CustomReport>, public ts::AsyncReport {

          public:
            static Napi::Object Init(Napi::Env env, Napi::Object exports);
            CustomReport(const Napi::CallbackInfo& info);
            Napi::Value GetLog(const Napi::CallbackInfo& info);
            Napi::Value GetLogMarker(const Napi::CallbackInfo& info);
            void ClearLog(const Napi::CallbackInfo& info);
            void Delete(const Napi::CallbackInfo& info);

          protected:
            void writeLog(int severity, const ts::UString& msg);

          private:
            static Napi::FunctionReference constructor;
            std::vector<std::string> _errorLog;
            std::vector<std::string> _log;
            std::vector<std::string> _jsonLog;
            std::vector<std::string> _xmlLog;
            std::string _jsonLogMarker;
            std::string _xmlLogMarker;
        };
    }
}
