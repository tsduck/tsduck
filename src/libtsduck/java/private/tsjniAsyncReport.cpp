//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
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
TSDUCK_SOURCE;

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::jni::AsyncReport::AsyncReport(JNIEnv* env, jobject obj, jstring log_method, int max_severity, const AsyncReportArgs& args) :
    ts::AsyncReport(max_severity, args),
    _env_constructor(env),
    _env_thread(nullptr),
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
    if (_env_constructor != nullptr && _obj_ref != nullptr) {
        _env_constructor->DeleteGlobalRef(_obj_ref);
        _obj_ref = nullptr;
    }
}


//----------------------------------------------------------------------------
// Initialization/completion of the asynchronous logging thread.
//----------------------------------------------------------------------------

void ts::jni::AsyncReport::asyncThreadStarted()
{
    // Attach the logging thread to the Java VM.
    if (ts::jni::javaVM != nullptr) {
        void* penv = nullptr;
        const jint status = ts::jni::javaVM->AttachCurrentThread(&penv, nullptr);
        if (status == JNI_OK && penv != nullptr) {
            _env_thread = reinterpret_cast<JNIEnv*>(penv);
        }
    }
}

void ts::jni::AsyncReport::asyncThreadCompleted()
{
    // Detach the logging thread from the Java VM.
    if (ts::jni::javaVM != nullptr) {
        _env_thread = nullptr;
        ts::jni::javaVM->DetachCurrentThread();
    }
}


//----------------------------------------------------------------------------
// Message logging method.
//----------------------------------------------------------------------------

void ts::jni::AsyncReport::asyncThreadLog(int severity, const UString& message)
{
    if (_env_thread != nullptr && _obj_ref != nullptr && _obj_method != nullptr) {
        const jstring jmessage = ToJString(_env_thread, message);
        if (jmessage != nullptr) {
            _env_thread->CallVoidMethod(_obj_ref, _obj_method, jint(severity), jmessage);
            _env_thread->DeleteLocalRef(jmessage);
        }
    }
}

#endif // TS_NO_JAVA
