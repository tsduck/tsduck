//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Native implementation of the Java class io.tsduck.InputSwitcher.
//
//----------------------------------------------------------------------------

#include "tsInputSwitcher.h"
#include "tsNullReport.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)

//
// private native void initNativeObject(Report report);
//
TSDUCKJNI void JNICALL Java_io_tsduck_InputSwitcher_initNativeObject(JNIEnv* env, jobject obj, jobject jreport)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::InputSwitcher* isw = ts::jni::GetPointerField<ts::InputSwitcher>(env, obj, "nativeObject");
    if (env != nullptr && isw == nullptr) {
        ts::Report* report = nullptr;
        if (jreport != nullptr) {
            report = ts::jni::GetPointerField<ts::Report>(env, jreport, "nativeObject");
        }
        if (report == nullptr) {
            report = &NULLREP;
        }
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::InputSwitcher(*report));
    }
}

//
// public native void setInput(int pluginIndex);
//
TSDUCKJNI void JNICALL Java_io_tsduck_InputSwitcher_setInput(JNIEnv* env, jobject obj, jint pluginIndex)
{
    ts::InputSwitcher* isw = ts::jni::GetPointerField<ts::InputSwitcher>(env, obj, "nativeObject");
    if (isw != nullptr) {
        isw->setInput(size_t(pluginIndex));
    }
}

//
// public native void nextInput();
//
TSDUCKJNI void JNICALL Java_io_tsduck_InputSwitcher_nextInput(JNIEnv* env, jobject obj)
{
    ts::InputSwitcher* isw = ts::jni::GetPointerField<ts::InputSwitcher>(env, obj, "nativeObject");
    if (isw != nullptr) {
        isw->nextInput();
    }
}

//
// public native void previousInput();
//
TSDUCKJNI void JNICALL Java_io_tsduck_InputSwitcher_previousInput(JNIEnv* env, jobject obj)
{
    ts::InputSwitcher* isw = ts::jni::GetPointerField<ts::InputSwitcher>(env, obj, "nativeObject");
    if (isw != nullptr) {
        isw->previousInput();
    }
}

//
// public native int currentInput();
//
TSDUCKJNI jint JNICALL Java_io_tsduck_InputSwitcher_currentInput(JNIEnv* env, jobject obj)
{
    ts::InputSwitcher* isw = ts::jni::GetPointerField<ts::InputSwitcher>(env, obj, "nativeObject");
    return isw == nullptr ? 0 : jint(isw->currentInput());
}

//
// public native void stop();
//
TSDUCKJNI void JNICALL Java_io_tsduck_InputSwitcher_stop(JNIEnv* env, jobject obj)
{
    ts::InputSwitcher* isw = ts::jni::GetPointerField<ts::InputSwitcher>(env, obj, "nativeObject");
    if (isw != nullptr) {
        isw->stop();
    }
}

//
// public native void waitForTermination();
//
TSDUCKJNI void JNICALL Java_io_tsduck_InputSwitcher_waitForTermination(JNIEnv* env, jobject obj)
{
    ts::InputSwitcher* isw = ts::jni::GetPointerField<ts::InputSwitcher>(env, obj, "nativeObject");
    if (isw != nullptr) {
        isw->waitForTermination();
    }
}

//
// public native void delete();
//
TSDUCKJNI void JNICALL Java_io_tsduck_InputSwitcher_delete(JNIEnv* env, jobject obj)
{
    ts::InputSwitcher* isw = ts::jni::GetPointerField<ts::InputSwitcher>(env, obj, "nativeObject");
    if (isw != nullptr) {
        delete isw;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

//----------------------------------------------------------------------------
// Start method: the parameters are fetched from the Java object fields.
//----------------------------------------------------------------------------

//
// public native boolean start();
//
TSDUCKJNI jboolean JNICALL Java_io_tsduck_InputSwitcher_start(JNIEnv* env, jobject obj)
{
    ts::InputSwitcher* isw = ts::jni::GetPointerField<ts::InputSwitcher>(env, obj, "nativeObject");
    if (isw == nullptr) {
        return false;
    }

    // Build InputSwitcher arguments.
    ts::InputSwitcherArgs args;
    args.app_name = ts::jni::GetStringField(env, obj, "appName");
    args.terminate = ts::jni::GetBoolField(env, obj, "terminate");
    args.fast_switch = ts::jni::GetBoolField(env, obj, "fastSwitch");
    args.delayed_switch = ts::jni::GetBoolField(env, obj, "delayedSwitch");
    args.remote_control.reuse_port = ts::jni::GetBoolField(env, obj, "reusePort");
    args.first_input = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "firstInput")));
    const jint primaryInput = ts::jni::GetIntField(env, obj, "firstInput");
    args.primary_input = primaryInput < 0 ? ts::NPOS : size_t(primaryInput);
    args.cycle_count = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "cycleCount")));
    args.buffered_packets = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "bufferedPackets")));
    args.max_input_packets = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "maxInputPackets")));
    args.max_output_packets = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "maxOutputPackets")));
    args.sock_buffer_size = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "sockBuffer")));
    args.receive_timeout = cn::milliseconds(cn::milliseconds::rep(std::max<jint>(0, ts::jni::GetIntField(env, obj, "receiveTimeout"))));
    jint port = ts::jni::GetIntField(env, obj, "remoteServerPort");
    if (port > 0 && port < 0xFFFF) {
        args.remote_control.server_addr.setPort(uint16_t(port));
    }
    args.event_command = ts::jni::GetStringField(env, obj, "eventCommand");
    ts::UString addr(ts::jni::GetStringField(env, obj, "eventUDPAddress"));
    if (!addr.empty() && !args.event_udp.resolve(addr, isw->report())) {
        return false;
    }
    port = ts::jni::GetIntField(env, obj, "eventUDPPort");
    if (port > 0 && port < 0xFFFF) {
        args.event_udp.setPort(uint16_t(port));
    }
    addr = ts::jni::GetStringField(env, obj, "eventLocalAddress");
    if (!addr.empty() && !args.event_local_address.resolve(addr, isw->report())) {
        return false;
    }
    args.event_ttl = int(ts::jni::GetIntField(env, obj, "eventTTL"));

    // Get plugins description and start the input switcher.
    return ts::jni::GetPluginOptions(env, jobjectArray(ts::jni::GetObjectField(env, obj, "output", JCS_ARRAY(JCS_STRING))), args.output) &&
           ts::jni::GetPluginOptionsVector(env, jobjectArray(ts::jni::GetObjectField(env, obj, "inputs", JCS_ARRAY(JCS_ARRAY(JCS_STRING)))), args.inputs) &&
           isw->start(args);
}

#endif // TS_NO_JAVA
