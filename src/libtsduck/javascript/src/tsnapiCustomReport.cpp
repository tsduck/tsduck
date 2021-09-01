#include "tsnapiCustomReport.h"

/**
 * A customized wrapper class for C++ AsyncReport, which separates the log messages into different logs.
 */
namespace ts {
    namespace napi {

        /**
        * reference to store the class definition that needs to be exported to JS
        */
        Napi::FunctionReference CustomReport::constructor;

        /**
        * Constructor
        * @param info Napi Array containing: input[0]: json log marker, input[1]: xml log marker
        */
        CustomReport::CustomReport(const Napi::CallbackInfo& info) : Napi::ObjectWrap<CustomReport>(info) {
          Napi::Env env = info.Env();

          if ( info.Length() != 2 || !info[0].IsString() || !info[1].IsString() ) {
            Napi::TypeError::New(env, "Invalid arguments. Expected 2 Strings: JSON-Marker, XML-Marker.").ThrowAsJavaScriptException();
            return;
          }
          else {
            const std::vector<std::string> log = {};
            const std::vector<std::string> errorLog = {};
            const std::vector<std::string> jsonLog = {};
            const std::vector<std::string> xmlLog = {};
            this->_log = log;
            this->_errorLog = errorLog;
            this->_jsonLog = jsonLog;
            this->_xmlLog = xmlLog;
            this->_jsonLogMarker = info[0].As<Napi::String>();
            this->_xmlLogMarker = info[1].As<Napi::String>();
          }
        }

        /**
        * Init function for setting the export key to JS
        */
        Napi::Object CustomReport::Init(Napi::Env env, Napi::Object exports) {

          Napi::Function func = DefineClass(env, "CustomReport", {
            InstanceMethod("getLog", &CustomReport::GetLog),
            InstanceMethod("getLogMarker", &CustomReport::GetLogMarker),
            InstanceMethod("clearLog", &CustomReport::ClearLog),
          });

          constructor = Napi::Persistent(func);
          constructor.SuppressDestruct();

          exports.Set("CustomReport", func);
          return exports;
        }

        /**
        * overwrites the writeLog function of abstract Report class to store the messages as string values in a vector members
        * customized: separates the log messages to store them into the right log vector
        */
        void CustomReport::writeLog(int severity, const ts::UString& msg){
          std::string str = msg.toUTF8();
          std::size_t jsonPos = str.find(this->_jsonLogMarker);
          std::size_t xmlPos = str.find(this->_xmlLogMarker);
          if(jsonPos != std::string::npos){
            this->_jsonLog.emplace_back(str.substr(jsonPos + this->_jsonLogMarker.length()));
          } else if (xmlPos != std::string::npos){
            this->_xmlLog.emplace_back(str.substr(xmlPos + this->_xmlLogMarker.length()));
          } else {
            if(severity>-3){
              this->_log.emplace_back(str);
            } else {
              std::cout << str << std::endl;
              this->_errorLog.emplace_back(str);
            }
          }
        }

        /**
        * clears the logs
        */
        void CustomReport::ClearLog(const Napi::CallbackInfo& info){
          this->_log.clear(); 
          this->_errorLog.clear(); 
          this->_jsonLog.clear(); 
          this->_xmlLog.clear(); 
        }

        /**
        * gets the log marker
        * @param info Napi Array containing: input[0]: type of log ('json' or 'xml')
        * @return the specified log marker as Napi String
        */
        Napi::Value CustomReport::GetLogMarker(const Napi::CallbackInfo& info){
            Napi::Env env = info.Env();
            if(info.Length()!=1 || !info[0].IsString()) {
              Napi::TypeError::New(env, "Invalid arguments. Expected 1 String ('json' or 'xml').").ThrowAsJavaScriptException();
              return {};
            }
            std::string input = info[0].ToString().Utf8Value();
            if(input=="json"){
                return Napi::String::New(env, this->_jsonLogMarker);
            } else if(input=="xml"){
                return Napi::String::New(env, this->_xmlLogMarker);
            } else {
              Napi::TypeError::New(env, "Invalid arguments. Expected 1 String ('json' or 'xml').").ThrowAsJavaScriptException();
              return {};
            }
        }

        /**
        * gets the log messages
        * @param info Napi Array containing: info[0] (optional): type of log ('error', 'json' or 'xml'), default: normal log
        * @return the log messages of specified log marker as Napi String Array
        */
        Napi::Value CustomReport::GetLog(const Napi::CallbackInfo& info){
          Napi::Env env = info.Env();

          if (info.Length() > 1) {
            Napi::TypeError::New(env, "Invalid arguments. Expected 0 arguments for normal log or 1 String ('error', 'json' or 'xml').").ThrowAsJavaScriptException();
            return {};
          }

          if(info.Length() == 0){
            Napi::Array array = Napi::Array::New(env, this->_log.size());
            uint32_t i = 0;
            for (auto& it : this->_log) {
                array[i++] = Napi::String::New(env, it);
            }
            return array; 
          } else {
            if(!info[0].IsString()){
                Napi::TypeError::New(env, "Invalid argument. Expected 0 arguments for normal log or 1 String ('error', 'json' or 'xml').").ThrowAsJavaScriptException();
                return {};
            }
            std::string input = info[0].ToString().Utf8Value();
            if(input == "json"){
              Napi::Array array = Napi::Array::New(env, this->_jsonLog.size());
              uint32_t i = 0;
              for (auto& it : this->_jsonLog) {
                  array[i++] = Napi::String::New(env, it);
              }
              return array;
            } else if (input == "xml") {
              Napi::Array array = Napi::Array::New(env, this->_xmlLog.size());
              uint32_t i = 0;
              for (auto& it : this->_xmlLog) {
                  array[i++] = Napi::String::New(env, it);
              }
              return array;
            } else if (input == "error") {
              Napi::Array array = Napi::Array::New(env, this->_errorLog.size());
              uint32_t i = 0;
              for (auto& it : this->_errorLog) {
                  array[i++] = Napi::String::New(env, it);
              }
              return array;
            } else {
              Napi::TypeError::New(env, "Invalid argument. Expected 0 arguments for normal log or 1 String ('json' or 'xml').").ThrowAsJavaScriptException();
              return {};
            }
          }
        }
    }
}
