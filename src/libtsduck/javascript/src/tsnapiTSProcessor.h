#pragma once

#include <napi.h>

#include "tsTSProcessor.h"
#include "tsPluginOptions.h"

/**
 * A wrapper class for C++ TSProcessor.
 */
namespace ts {
    namespace napi {

        class TSProcessor : public Napi::ObjectWrap<TSProcessor> {

          public:
            static Napi::Object Init(Napi::Env env, Napi::Object exports);
            TSProcessor(const Napi::CallbackInfo& info);

          private:
            static Napi::FunctionReference constructor;
            Napi::Value IsStarted(const Napi::CallbackInfo& info);
            Napi::Value Start(const Napi::CallbackInfo& info);
            void WaitForTermination(const Napi::CallbackInfo& info);
            void Abort(const Napi::CallbackInfo& info);
            void SetInput(const Napi::CallbackInfo& info);
            void SetPlugins(const Napi::CallbackInfo& info);
            void SetOutput(const Napi::CallbackInfo& info);
            void ClearFields(const Napi::CallbackInfo& info);

            ts::PluginOptions _input;
            ts::PluginOptionsVector _plugins;
            ts::PluginOptions _output;
            ts::TSProcessor *_tsProcessor;
        };
    }
}
