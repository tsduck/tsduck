//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Native implementation of the Java class io.tsduck.TSProcessor.
//
//----------------------------------------------------------------------------

#include "tsTSProcessor.h"
#include "tsNullReport.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)

//
// private native void initNativeObject(Report report);
//
TSDUCKJNI void JNICALL Java_io_tsduck_TSProcessor_initNativeObject(JNIEnv* env, jobject obj, jobject jreport)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (env != nullptr && tsp == nullptr) {
        ts::Report* report = nullptr;
        if (jreport != nullptr) {
            report = ts::jni::GetPointerField<ts::Report>(env, jreport, "nativeObject");
        }
        if (report == nullptr) {
            report = ts::NullReport::Instance();
        }
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::TSProcessor(*report));
    }
}

//
// public native void abort();
//
TSDUCKJNI void JNICALL Java_io_tsduck_TSProcessor_abort(JNIEnv* env, jobject obj)
{
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (tsp != nullptr) {
        tsp->abort();
    }
}

//
// public native void waitForTermination();
//
TSDUCKJNI void JNICALL Java_io_tsduck_TSProcessor_waitForTermination(JNIEnv* env, jobject obj)
{
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (tsp != nullptr) {
        tsp->waitForTermination();
    }
}

//
// public native void delete();
//
TSDUCKJNI void JNICALL Java_io_tsduck_TSProcessor_delete(JNIEnv* env, jobject obj)
{
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (tsp != nullptr) {
        delete tsp;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

//----------------------------------------------------------------------------
// Start method: the parameters are fetched from the Java object fields.
//----------------------------------------------------------------------------

//
// public native boolean start();
//
TSDUCKJNI jboolean JNICALL Java_io_tsduck_TSProcessor_start(JNIEnv* env, jobject obj)
{
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (tsp == nullptr) {
        return false;
    }

    // Build TSProcessor arguments.
    ts::TSProcessorArgs args;
    args.ignore_jt = ts::jni::GetBoolField(env, obj, "ignoreJointTermination");
    args.log_plugin_index = ts::jni::GetBoolField(env, obj, "logPluginIndex");
    args.ts_buffer_size = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "bufferSize")));
    if (args.ts_buffer_size == 0) {
        args.ts_buffer_size = ts::TSProcessorArgs::DEFAULT_BUFFER_SIZE;
    }
    args.max_flush_pkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "maxFlushedPackets")));
    args.max_input_pkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "maxInputPackets")));
    args.max_output_pkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "maxOutputPackets")));
    if (args.max_output_pkt == 0) {
        args.max_output_pkt = ts::NPOS;  // zero means unlimited
    }
    args.init_input_pkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "initialInputPackets")));
    args.instuff_nullpkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "addInputStuffingNull")));
    args.instuff_inpkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "addInputStuffingInput")));
    args.instuff_start = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "addStartStuffing")));
    args.instuff_stop = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "addStopStuffing")));
    args.fixed_bitrate = ts::BitRate(std::max<jint>(0, ts::jni::GetIntField(env, obj, "bitrate")));
    args.bitrate_adj = ts::MilliSecond(std::max<jint>(0, ts::jni::GetIntField(env, obj, "bitrateAdjustInterval")));
    args.receive_timeout = ts::MilliSecond(std::max<jint>(0, ts::jni::GetIntField(env, obj, "receiveTimeout")));
    args.app_name = ts::jni::GetStringField(env, obj, "appName");

    // Get plugins description.
    // Note: The packet processor plugin can be null (no plugin) but the presence of the input and output plugin is required.
    bool ok = ts::jni::GetPluginOptions(env, jobjectArray(ts::jni::GetObjectField(env, obj, "input", JCS_ARRAY(JCS_STRING))), args.input) &&
              ts::jni::GetPluginOptions(env, jobjectArray(ts::jni::GetObjectField(env, obj, "output", JCS_ARRAY(JCS_STRING))), args.output) &&
              ts::jni::GetPluginOptionsVector(env, jobjectArray(ts::jni::GetObjectField(env, obj, "plugins", JCS_ARRAY(JCS_ARRAY(JCS_STRING)))), args.plugins);

    // Debug message.
    if (tsp->report().debug()) {
        ts::UString cmd(args.app_name);
        cmd.append(u" ");
        cmd.append(args.input.toString(ts::PluginType::INPUT));
        for (const auto& it : args.plugins) {
            cmd.append(u" ");
            cmd.append(it.toString(ts::PluginType::PROCESSOR));
        }
        cmd.append(u" ");
        cmd.append(args.output.toString(ts::PluginType::OUTPUT));
        tsp->report().debug(u"starting: %s", {cmd});
    }

    // Finally start the TSProcessor.
    return ok = ok && tsp->start(args);
}

#endif // TS_NO_JAVA
