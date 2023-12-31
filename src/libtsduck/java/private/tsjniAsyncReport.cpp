//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsjniAsyncReport.h"

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::jni::AsyncReport::AsyncReport(JNIEnv* env, jobject obj, jstring log_method, int max_severity, const AsyncReportArgs& args) :
    ts::AsyncReport(max_severity, args),
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

ts::jni::AsyncReport::~AsyncReport()
{
    if (_env != nullptr && _obj_ref != nullptr) {
        _env->DeleteGlobalRef(_obj_ref);
        _obj_ref = nullptr;
    }
}


//----------------------------------------------------------------------------
// Message logging method.
//----------------------------------------------------------------------------

void ts::jni::AsyncReport::asyncThreadLog(int severity, const UString& message)
{
    JNIEnv* env = JNIEnvForCurrentThead();
    if (env != nullptr && _obj_ref != nullptr && _obj_method != nullptr) {
        const jstring jmessage = ToJString(env, message);
        if (jmessage != nullptr) {
            env->CallVoidMethod(_obj_ref, _obj_method, jint(severity), jmessage);
            env->DeleteLocalRef(jmessage);
        }
    }
}

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.AbstractAsyncReport
//----------------------------------------------------------------------------

//
// private native void initNativeObject(String logMethodName, int severity, boolean syncLog, int logMsgCount);
//
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractAsyncReport_initNativeObject(JNIEnv* env, jobject obj, jstring method, jint severity, jboolean syncLog, jint logMsgCount)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::jni::AsyncReport* report = ts::jni::GetPointerField<ts::jni::AsyncReport>(env, obj, "nativeObject");
    if (env != nullptr && report == nullptr) {
        ts::AsyncReportArgs args;
        args.sync_log = bool(syncLog);
        args.log_msg_count = size_t(std::max<jint>(1, logMsgCount));
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::jni::AsyncReport(env, obj, method, int(severity), args));
    }
}

//
// public native void terminate();
//
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractAsyncReport_terminate(JNIEnv* env, jobject obj)
{
    ts::jni::AsyncReport* report = ts::jni::GetPointerField<ts::jni::AsyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        report->terminate();
    }
}

//
// public native void delete();
//
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractAsyncReport_delete(JNIEnv* env, jobject obj)
{
    ts::jni::AsyncReport* report = ts::jni::GetPointerField<ts::jni::AsyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        delete report;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

#endif // TS_NO_JAVA
