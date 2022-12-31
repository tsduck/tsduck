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
//  Native implementation of the Java class io.tsduck.Report and subclasses.
//
//----------------------------------------------------------------------------

#include "tsNullReport.h"
#include "tsCerrReport.h"
#include "tsAsyncReport.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.Report
//----------------------------------------------------------------------------

//
// public static native String header(int severity);
//
TSDUCKJNI jstring JNICALL Java_io_tsduck_Report_header(JNIEnv* env, jclass clazz, jint severity)
{
    return ts::jni::ToJString(env, ts::Severity::Header(int(severity)));
}

//
// public native void setMaxSeverity(int severity);
//
TSDUCKJNI void JNICALL Java_io_tsduck_Report_setMaxSeverity(JNIEnv* env, jobject obj, jint severity)
{
    ts::Report* report = ts::jni::GetPointerField<ts::Report>(env, obj, "nativeObject");
    if (report != nullptr) {
        report->setMaxSeverity(int(severity));
    }
}

//
// public native void log(int severity, String message);
//
TSDUCKJNI void JNICALL Java_io_tsduck_Report_log(JNIEnv* env, jobject obj, jint severity, jstring message)
{
    ts::Report* report = ts::jni::GetPointerField<ts::Report>(env, obj, "nativeObject");
    if (report != nullptr) {
        report->log(int(severity), ts::jni::ToUString(env, message));
    }
}

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.NullReport
//----------------------------------------------------------------------------

//
// private native void initNativeObject();
//
TSDUCKJNI void JNICALL Java_io_tsduck_NullReport_initNativeObject(JNIEnv* env, jobject obj)
{
    // Set the same singleton address to all Java instances (won't be deleted).
    ts::jni::SetPointerField(env, obj, "nativeObject", ts::NullReport::Instance());
}

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.ErrReport
//----------------------------------------------------------------------------

//
// private native void initNativeObject();
//
TSDUCKJNI void JNICALL Java_io_tsduck_ErrReport_initNativeObject(JNIEnv* env, jobject obj)
{
    // Set the same singleton address to all Java instances (won't be deleted).
    ts::jni::SetPointerField(env, obj, "nativeObject", ts::CerrReport::Instance());
}

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.AsyncReport
//----------------------------------------------------------------------------

//
// private native void initNativeObject(int severity, boolean syncLog, boolean timedLog, int logMsgCount);
//
TSDUCKJNI void JNICALL Java_io_tsduck_AsyncReport_initNativeObject(JNIEnv* env, jobject obj, jint severity, jboolean syncLog, jboolean timedLog, jint logMsgCount)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::AsyncReport* report = ts::jni::GetPointerField<ts::AsyncReport>(env, obj, "nativeObject");
    if (env != nullptr && report == nullptr) {
        ts::AsyncReportArgs args;
        args.sync_log = bool(syncLog);
        args.timed_log = bool(timedLog);
        args.log_msg_count = size_t(std::max<jint>(1, logMsgCount));
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::AsyncReport(int(severity), args));
    }
}

//
// public native void terminate();
//
TSDUCKJNI void JNICALL Java_io_tsduck_AsyncReport_terminate(JNIEnv* env, jobject obj)
{
    ts::AsyncReport* report = ts::jni::GetPointerField<ts::AsyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        report->terminate();
    }
}

//
// public native void delete();
//
TSDUCKJNI void JNICALL Java_io_tsduck_AsyncReport_delete(JNIEnv* env, jobject obj)
{
    ts::AsyncReport* report = ts::jni::GetPointerField<ts::AsyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        delete report;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

#endif // TS_NO_JAVA
