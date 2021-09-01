#include "tsnapiAsyncReport.h"

/**
 * A wrapper class for C++ AsyncReport.
 */
namespace ts {
    namespace napi {

      /**
       * reference to store the class definition that needs to be exported to JS
       */
      Napi::FunctionReference AsyncReport::constructor;

      /**
       * Constructor
       */
      AsyncReport::AsyncReport(const Napi::CallbackInfo& info) : Napi::ObjectWrap<AsyncReport>(info) {
        const std::vector<std::string> log = {};
        this->_log = log;
      }

      /**
       * Init function for setting the export key to JS
       */
      Napi::Object AsyncReport::Init(Napi::Env env, Napi::Object exports) {

        Napi::Function func = DefineClass(env, "AsyncReport", {
          InstanceMethod("getLog", &AsyncReport::GetLog),
          InstanceMethod("clearLog", &AsyncReport::ClearLog),
        });

        constructor = Napi::Persistent(func);
        constructor.SuppressDestruct();

        exports.Set("AsyncReport", func);
        return exports;
      }

      /**
       * overwrites the writeLog function of abstract AsyncReport class to store the messages as string values in a vector member
       */
      void AsyncReport::writeLog(int severity, const ts::UString& msg){
        this->_log.emplace_back(msg.toUTF8()); 
      }

      /**
       * clears the log
       */
      void AsyncReport::ClearLog(const Napi::CallbackInfo& info){
        this->_log.clear(); 
      }

      /**
       * gets log messages
       * @return the log messages as napi string array
       */
      Napi::Value AsyncReport::GetLog(const Napi::CallbackInfo& info){
        Napi::Env env = info.Env();

        Napi::Array array = Napi::Array::New(env, this->_log.size());

        uint32_t i = 0;
        for (auto& it : this->_log) {
          array[i++] = Napi::String::New(env, it);
        }

        return array;
      }
    }
}
