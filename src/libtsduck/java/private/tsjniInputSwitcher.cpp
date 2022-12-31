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
            report = ts::NullReport::Instance();
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
    args.appName = ts::jni::GetStringField(env, obj, "appName");
    args.terminate = ts::jni::GetBoolField(env, obj, "terminate");
    args.fastSwitch = ts::jni::GetBoolField(env, obj, "fastSwitch");
    args.delayedSwitch = ts::jni::GetBoolField(env, obj, "delayedSwitch");
    args.reusePort = ts::jni::GetBoolField(env, obj, "reusePort");
    args.firstInput = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "firstInput")));
    const jint primaryInput = ts::jni::GetIntField(env, obj, "firstInput");
    args.primaryInput = primaryInput < 0 ? ts::NPOS : size_t(primaryInput);
    args.cycleCount = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "cycleCount")));
    args.bufferedPackets = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "bufferedPackets")));
    args.maxInputPackets = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "maxInputPackets")));
    args.maxOutputPackets = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "maxOutputPackets")));
    args.sockBuffer = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "sockBuffer")));
    args.receiveTimeout = ts::MilliSecond(std::max<jint>(0, ts::jni::GetIntField(env, obj, "receiveTimeout")));
    jint port = ts::jni::GetIntField(env, obj, "remoteServerPort");
    if (port > 0 && port < 0xFFFF) {
        args.remoteServer.setPort(uint16_t(port));
    }
    args.eventCommand = ts::jni::GetStringField(env, obj, "eventCommand");
    ts::UString addr(ts::jni::GetStringField(env, obj, "eventUDPAddress"));
    if (!addr.empty() && !args.eventUDP.resolve(addr, isw->report())) {
        return false;
    }
    port = ts::jni::GetIntField(env, obj, "eventUDPPort");
    if (port > 0 && port < 0xFFFF) {
        args.eventUDP.setPort(uint16_t(port));
    }
    addr = ts::jni::GetStringField(env, obj, "eventLocalAddress");
    if (!addr.empty() && !args.eventLocalAddress.resolve(addr, isw->report())) {
        return false;
    }
    args.eventTTL = int(ts::jni::GetIntField(env, obj, "eventTTL"));

    // Get plugins description and start the input switcher.
    return ts::jni::GetPluginOptions(env, jobjectArray(ts::jni::GetObjectField(env, obj, "output", JCS_ARRAY(JCS_STRING))), args.output) &&
           ts::jni::GetPluginOptionsVector(env, jobjectArray(ts::jni::GetObjectField(env, obj, "inputs", JCS_ARRAY(JCS_ARRAY(JCS_STRING)))), args.inputs) &&
           isw->start(args);
}

#endif // TS_NO_JAVA
