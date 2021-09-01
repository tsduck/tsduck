#include <napi.h>
#include <unistd.h>

#include "tsTSProcessorArgs.h"
#include "tsTSProcessor.h"

/**
 * Async Worker to execute native TSP start method.
 */
namespace ts {
    namespace napi {

		class NapiTspWorker : public Napi::AsyncWorker {

			public:
				NapiTspWorker(Napi::Env &env, ts::TSProcessor& tsp, ts::TSProcessorArgs& args) 
					: Napi::AsyncWorker(env), _tsp(tsp), _args(args), _deferred(Napi::Promise::Deferred::New(env)) {};
					
				~NapiTspWorker() {};

				void Execute() {
					_tsp.start(_args);
					_tsp.waitForTermination();
					_tsp.abort();
				}

				void OnOK() {
					_deferred.Resolve(Napi::Boolean::New(Env(), true));
				}

				void OnError(Napi::Error const &error) {
					std::cerr << error << std::endl;
					_deferred.Reject(error.Value());
				}

				Napi::Promise GetPromise() {
					return _deferred.Promise();
				}

			private:
				ts::TSProcessor& _tsp;
				ts::TSProcessorArgs& _args;
				Napi::Promise::Deferred _deferred;
		};
	}
}