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
//  Native implementation of the Java class io.tsduck.SystemMonitor.
//
//----------------------------------------------------------------------------

#include "tsSystemMonitor.h"
#include "tsCerrReport.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)

//
// private native void initNativeObject(Report report, String config);
//
TSDUCKJNI void JNICALL Java_io_tsduck_SystemMonitor_initNativeObject(JNIEnv* env, jobject obj, jobject jreport, jstring jconfig)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::SystemMonitor* mon = ts::jni::GetPointerField<ts::SystemMonitor>(env, obj, "nativeObject");
    if (env != nullptr && mon == nullptr) {
        ts::Report* report = nullptr;
        if (jreport != nullptr) {
            report = ts::jni::GetPointerField<ts::Report>(env, jreport, "nativeObject");
        }
        if (report == nullptr) {
            report = ts::CerrReport::Instance();
        }
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::SystemMonitor(*report, ts::jni::ToUString(env, jconfig)));
    }
}

//
// public native void start();
//
TSDUCKJNI void JNICALL Java_io_tsduck_SystemMonitor_start(JNIEnv* env, jobject obj)
{
    ts::SystemMonitor* mon = ts::jni::GetPointerField<ts::SystemMonitor>(env, obj, "nativeObject");
    if (mon != nullptr) {
        mon->start();
    }
}

//
// public native void stop();
//
TSDUCKJNI void JNICALL Java_io_tsduck_SystemMonitor_stop(JNIEnv* env, jobject obj)
{
    ts::SystemMonitor* mon = ts::jni::GetPointerField<ts::SystemMonitor>(env, obj, "nativeObject");
    if (mon != nullptr) {
        mon->stop();
    }
}

//
// public native void waitForTermination();
//
TSDUCKJNI void JNICALL Java_io_tsduck_SystemMonitor_waitForTermination(JNIEnv* env, jobject obj)
{
    ts::SystemMonitor* mon = ts::jni::GetPointerField<ts::SystemMonitor>(env, obj, "nativeObject");
    if (mon != nullptr) {
        mon->waitForTermination();
    }
}

//
// public native void delete();
//
TSDUCKJNI void JNICALL Java_io_tsduck_SystemMonitor_delete(JNIEnv* env, jobject obj)
{
    ts::SystemMonitor* mon = ts::jni::GetPointerField<ts::SystemMonitor>(env, obj, "nativeObject");
    if (mon != nullptr) {
        delete mon;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

#endif // TS_NO_JAVA
