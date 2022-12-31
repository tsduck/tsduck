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

#include "tsjniAsyncReport.h"

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::jni::AsyncReport::AsyncReport(JNIEnv* env, jobject obj, jstring log_method, int max_severity, const AsyncReportArgs& args) :
    ts::AsyncReport(max_severity, args),
    _env(env),
    _obj_ref(env == nullptr || obj == nullptr ? nullptr : env->NewGlobalRef(obj)),
    _obj_method(nullptr)
{
    if (_obj_ref != nullptr) {
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
