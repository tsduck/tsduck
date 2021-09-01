#include <napi.h>

#include "tsVersionInfo.h"
#include "tsVersionString.h"

/**
 * TSDuck library general information.
 */
namespace ts {
    namespace napi {

        /**
        * TSDuck version as a string.
        * @return TSDuck version as a string.
        */
        Napi::String getVersion(const Napi::CallbackInfo& info) {
            Napi::Env env = info.Env();
            return Napi::String::New(env, ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT));
        }
    }
}