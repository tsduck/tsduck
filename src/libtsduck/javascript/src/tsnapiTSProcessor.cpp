#include "tsTSProcessorArgs.h"
#include "tsPluginOptions.h"

#include "tsnapiTSProcessor.h"
#include "tsnapiAsyncReport.h"
#include "tsnapiAsyncTSPWorker.cc"

/**
* A wrapper class for C++ TSProcessor.
*/
namespace ts {
    namespace napi {

        /**
         * reference to store the class definition that needs to be exported to JS
         */
        Napi::FunctionReference ts::napi::TSProcessor::constructor;

        /**
         * Constructor
         * @param info Napi Array containing: input[0]: AsyncReport (optional), default: with native error report
         */
        TSProcessor::TSProcessor(const Napi::CallbackInfo& info) : Napi::ObjectWrap<TSProcessor>(info)  {
          Napi::Env env = info.Env();

          // 1 argument: initializes the tsp with given report
          if ( info.Length() == 1 ) {
            AsyncReport* rep = Napi::ObjectWrap<AsyncReport>::Unwrap(info[0].As<Napi::Object>());
            if(env.IsExceptionPending()){ // if converting unwrapped object to Report throws error, report wrong argument type
              env.GetAndClearPendingException();
              Napi::TypeError::New(env, "Invalid argument. Expected 0 or 1 (Report).").ThrowAsJavaScriptException();
              return;
            }
            this->_tsProcessor = new ts::TSProcessor(*rep);
            return;
          }

          // default constructor: initializes the tsp with native error report
          else if ( info.Length() == 0 ) {
            ts::CerrReport* rep = ts::CerrReport::Instance();
            this->_tsProcessor = new ts::TSProcessor(*rep);
          }

          else Napi::TypeError::New(env, "Too many arguments. Expected 0 or 1 (Report).").ThrowAsJavaScriptException(); 

          return;
        }

        /**
         * Init function for setting the export key to JS
         */
        Napi::Object TSProcessor::Init(Napi::Env env, Napi::Object exports) {

          Napi::Function func = DefineClass(env, "TSProcessor", {
            InstanceMethod("isStarted", &TSProcessor::IsStarted),
            InstanceMethod("start", &TSProcessor::Start),
            InstanceMethod("waitForTermination", &TSProcessor::WaitForTermination),
            InstanceMethod("abort", &TSProcessor::Abort),
            InstanceMethod("setInput", &TSProcessor::SetInput),
            InstanceMethod("setPlugins", &TSProcessor::SetPlugins),
            InstanceMethod("setOutput", &TSProcessor::SetOutput),
            InstanceMethod("clearFields", &TSProcessor::ClearFields),
          });

          constructor = Napi::Persistent(func);
          constructor.SuppressDestruct();

          exports.Set("TSProcessor", func);
          return exports;
        }

        /**
         * calls native isStarted function
         */
        Napi::Value TSProcessor::IsStarted(const Napi::CallbackInfo& info) {
          Napi::Env env = info.Env();
          return Napi::Boolean::New(env, this->_tsProcessor->isStarted());
        }

        /**
         * calls native abort function
         */
        void TSProcessor::Abort(const Napi::CallbackInfo& info) {
          this->_tsProcessor->abort();
          return;
        }

        /**
         * calls native waitForTermination function
         */
        void TSProcessor::WaitForTermination(const Napi::CallbackInfo& info) {
          this->_tsProcessor->waitForTermination();
          return;
        }

        /**
         * sets input parameters
         * @param info Napi Array containing: input[0]: string array with input parameters
         */
        void TSProcessor::SetInput(const Napi::CallbackInfo& info) {
          Napi::Env env = info.Env();

          if ( info.Length() != 1 || !info[0].IsArray()) {
            Napi::TypeError::New(env, "Invalid argument. Expected 1 (String Array).").ThrowAsJavaScriptException();
            return;
          }

          const Napi::Array array = info[0].As<Napi::Array>();
          const int arrayLength = array.Length();

          // building ts::PluginOptions from array
          const ts::UString name = ts::UString(array.Get("0").ToString());
          ts::UStringVector vect = {};
          for (int i = 1; i < arrayLength ; i++) {
              vect.emplace_back(ts::UString(array[i].ToString()));
          }
          ts::PluginOptions* input = new ts::PluginOptions(name, vect);

          // store ts::PluginOptions in _input
          this->_input = *input;
          return;
        }

        /**
         * sets plugin parameters
         * @param info Napi Array containing: input[0]: array of string arrays with plugin parameters
         */ 
        void TSProcessor::SetPlugins(const Napi::CallbackInfo& info) {
          Napi::Env env = info.Env();

          if ( info.Length() != 1 || !info[0].IsArray()) {
            Napi::TypeError::New(env, "Invalid argument. Expected 1 (Array of String Arrays).").ThrowAsJavaScriptException();
            return;
          }

          const Napi::Array arrayOfArrays = info[0].As<Napi::Array>();
          const int pluginLength = arrayOfArrays.Length();

          // building ts::PluginOptionsVector from array
          ts::PluginOptionsVector plugins = {};
          for (int i = 0; i < pluginLength ; i++) {

            if ( !arrayOfArrays[i].IsArray()) {
              Napi::TypeError::New(env, "Invalid argument. Expected 1 (Array of String Arrays).").ThrowAsJavaScriptException();
              return;
            }
          
            const Napi::Array array = arrayOfArrays[i].As<Napi::Array>();
            const int arrayLength = array.Length();

            // building ts::PluginOptions from every array
            const ts::UString name = ts::UString(array.Get("0").ToString());
            ts::UStringVector vect = {};
            for (int j = 1; j < arrayLength ; j++) {
                vect.emplace_back(ts::UString(array[j].ToString()));
            }
            ts::PluginOptions* pluginOption = new ts::PluginOptions(name, vect);

            // add ts::PluginOptions to ts::PluginOptionsVector
            plugins.emplace_back(*pluginOption);
          }

          // store ts::PluginOptionsVector in _plugins
          this->_plugins = plugins;
          return;
        }

        /**
         * sets output parameters
         * @param info Napi Array containing: input[0]: string array with output parameters
         */
        void TSProcessor::SetOutput(const Napi::CallbackInfo& info) {
          Napi::Env env = info.Env();

          if ( info.Length() != 1 || !info[0].IsArray()) {
            Napi::TypeError::New(env, "Invalid argument. Expected 1 (String Array).").ThrowAsJavaScriptException();
            return;
          }

          const Napi::Array array = info[0].As<Napi::Array>();
          const int arrayLength = array.Length();

          // building ts::PluginOptions from array
          const ts::UString name = ts::UString(array.Get("0").ToString());
          ts::UStringVector vect = {};
          for (int i = 1; i < arrayLength ; i++) {
              vect.emplace_back(ts::UString(array[i].ToString()));
          }
          ts::PluginOptions* output = new ts::PluginOptions(name, vect);

          // store ts::PluginOptions in _output
          this->_output = *output;

          return;
        }

        /**
         * clears input, plugins and output parameters
         */
        void TSProcessor::ClearFields(const Napi::CallbackInfo& info) {
          ts::PluginOptions* emptyIn = new ts::PluginOptions();
          ts::PluginOptionsVector emptyPlugins = {};
          ts::PluginOptions* emptyOut = new ts::PluginOptions();
          
          this->_input = *emptyIn;
          this->_plugins = emptyPlugins;
          this->_output = *emptyOut;

          return;
        }

        /**
         * calls native start function with ts::TSProcessorArgs by customized async worker
         * @return Promise
         */
        Napi::Value TSProcessor::Start(const Napi::CallbackInfo& info) {
          Napi::Env env = info.Env();

          // error if input or output are not set
          if ( this->_input.name.empty() || this->_output.name.empty()) {
            Napi::Error err = Napi::Error::New(env, "Input and Output are not set.");
            Napi::Promise::Deferred errPromise = Napi::Promise::Deferred::New(env);
            errPromise.Reject(err.Value());
            return errPromise.Promise();
          }

          // builds TSProcessorArgs from members
          ts::TSProcessorArgs* args = new ts::TSProcessorArgs();
          args->input = this->_input;
          args->plugins = this->_plugins;
          args->output = this->_output;
          
          // calls native start function with ts::TSProcessorArgs
          NapiTspWorker *worker = new NapiTspWorker(env, *(this->_tsProcessor), *args);
          Napi::Promise promise = worker->GetPromise();
          worker->Queue();

          return promise;
        }
    }
}
