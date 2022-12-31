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

#include "tsjniSyncReport.h"

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::jni::SyncReport::SyncReport(JNIEnv* env, jobject obj, jstring log_method, int max_severity) :
    ts::Report(max_severity),
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
