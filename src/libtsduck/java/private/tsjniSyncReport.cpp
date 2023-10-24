//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsjniSyncReport.h"

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::jni::SyncReport::SyncReport(JNIEnv* env, jobject obj, jstring log_method, int max_severity) :
    ts::Report(max_severity),
    _env(env)
{
    if (env != nullptr && obj != nullptr) {
        _obj_ref = env->NewGlobalRef(obj);
        const char* const log_str = env->GetStringUTFChars(log_method, nullptr);
        if (log_str != nullptr) {
            _obj_method = env->GetMethodID(env->GetObjectClass(_obj_ref), log_str, "(" JCS_INT JCS_STRING ")" JCS_VOID);
            env->ReleaseStringUTFChars(log_method, log_str);
        }
    }
}

ts::jni::SyncReport::~SyncReport()
{
    if (_env != nullptr && _obj_ref != nullptr) {
        _env->DeleteGlobalRef(_obj_ref);
        _obj_ref = nullptr;
    }
}


//----------------------------------------------------------------------------
// Message logging method.
//----------------------------------------------------------------------------

void ts::jni::SyncReport::writeLog(int severity, const UString& message)
{
    if (_env != nullptr && _obj_ref != nullptr && _obj_method != nullptr) {
        const jstring jmessage = ToJString(_env, message);
        if (jmessage != nullptr) {
            _env->CallVoidMethod(_obj_ref, _obj_method, jint(severity), jmessage);
            _env->DeleteLocalRef(jmessage);
        }
    }
}


//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.AbstractSyncReport
//----------------------------------------------------------------------------

//
// private native void initNativeObject(String logMethodName, int severity);
//
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractSyncReport_initNativeObject(JNIEnv* env, jobject obj, jstring method, jint severity)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::jni::SyncReport* report = ts::jni::GetPointerField<ts::jni::SyncReport>(env, obj, "nativeObject");
    if (env != nullptr && report == nullptr) {
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::jni::SyncReport(env, obj, method, int(severity)));
    }
}

//
// public native void delete();
//
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractSyncReport_delete(JNIEnv* env, jobject obj)
{
    ts::jni::SyncReport* report = ts::jni::GetPointerField<ts::jni::SyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        delete report;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

#endif // TS_NO_JAVA
