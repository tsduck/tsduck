//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
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
            report = &CERR;
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
