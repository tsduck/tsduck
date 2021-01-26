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
//
//  Native implementation of the Java class io.tsduck.Report and subclasses.
//
//----------------------------------------------------------------------------

#include "tsNullReport.h"
#include "tsCerrReport.h"
#include "tsAsyncReport.h"
#include "tsjniSyncReport.h"
#include "tsjniAsyncReport.h"
TSDUCK_SOURCE;

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.Report
//----------------------------------------------------------------------------

// Method: io.tsduck.Report.header (static)
// Signature: (I)Ljava/lang/String;
TSDUCKJNI jstring JNICALL Java_io_tsduck_Report_header(JNIEnv* env, jclass clazz, jint severity)
{
    return ts::jni::ToJString(env, ts::Severity::Header(int(severity)));
}

// Method: io.tsduck.Report.setMaxSeverity
// Signature: (I)V
TSDUCKJNI void JNICALL Java_io_tsduck_Report_setMaxSeverity(JNIEnv* env, jobject obj, jint severity)
{
    ts::Report* report = ts::jni::GetPointerField<ts::Report>(env, obj, "nativeObject");
    if (report != nullptr) {
        report->setMaxSeverity(int(severity));
    }
}

// Method: io.tsduck.Report.log
// Signature: (ILjava/lang/String;)V
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

// Method: io.tsduck.NullReport.initNativeObject
// Signature: ()V
TSDUCKJNI void JNICALL Java_io_tsduck_NullReport_initNativeObject(JNIEnv* env, jobject obj)
{
    // Set the same singleton address to all Java instances (won't be deleted).
    ts::jni::SetPointerField(env, obj, "nativeObject", ts::NullReport::Instance());
}

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.ErrReport
//----------------------------------------------------------------------------

// Method: io.tsduck.ErrReport.initNativeObject
// Signature: ()V
TSDUCKJNI void JNICALL Java_io_tsduck_ErrReport_initNativeObject(JNIEnv* env, jobject obj)
{
    // Set the same singleton address to all Java instances (won't be deleted).
    ts::jni::SetPointerField(env, obj, "nativeObject", ts::CerrReport::Instance());
}

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.AsyncReport
//----------------------------------------------------------------------------

// Method: io.tsduck.AsyncReport.initNativeObject
// Signature: (IBBI)V
TSDUCKJNI void JNICALL Java_io_tsduck_AsyncReport_initNativeObject(JNIEnv* env, jobject obj, jint severity, jboolean syncLog, jboolean timedLog, jint logMsgCount)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::AsyncReport* report = ts::jni::GetPointerField<ts::AsyncReport>(env, obj, "nativeObject");
    if (report == nullptr) {
        ts::AsyncReportArgs args;
        args.sync_log = bool(syncLog);
        args.timed_log = bool(timedLog);
        args.log_msg_count = size_t(std::max<jint>(1, logMsgCount));
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::AsyncReport(int(severity), args));
    }
}

// Method: io.tsduck.AsyncReport.terminate
// Signature: ()V
TSDUCKJNI void JNICALL Java_io_tsduck_AsyncReport_terminate(JNIEnv* env, jobject obj)
{
    ts::AsyncReport* report = ts::jni::GetPointerField<ts::AsyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        report->terminate();
    }
}

// Method: io.tsduck.AsyncReport.delete
// Signature: ()V
TSDUCKJNI void JNICALL Java_io_tsduck_AsyncReport_delete(JNIEnv* env, jobject obj)
{
    ts::AsyncReport* report = ts::jni::GetPointerField<ts::AsyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        delete report;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.AbstractAsyncReport
//----------------------------------------------------------------------------

// Method: io.tsduck.AbstractAsyncReport.initNativeObject
// Signature: (Ljava/lang/String;IBI)V
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractAsyncReport_initNativeObject(JNIEnv* env, jobject obj, jstring method, jint severity, jboolean syncLog, jint logMsgCount)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::jni::AsyncReport* report = ts::jni::GetPointerField<ts::jni::AsyncReport>(env, obj, "nativeObject");
    if (report == nullptr) {
        ts::AsyncReportArgs args;
        args.sync_log = bool(syncLog);
        args.log_msg_count = size_t(std::max<jint>(1, logMsgCount));
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::jni::AsyncReport(env, obj, method, int(severity), args));
    }
}

// Method: io.tsduck.AbstractAsyncReport.terminate
// Signature: ()V
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractAsyncReport_terminate(JNIEnv* env, jobject obj)
{
    ts::jni::AsyncReport* report = ts::jni::GetPointerField<ts::jni::AsyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        report->terminate();
    }
}

// Method: io.tsduck.AbstractAsyncReport.delete
// Signature: ()V
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractAsyncReport_delete(JNIEnv* env, jobject obj)
{
    ts::jni::AsyncReport* report = ts::jni::GetPointerField<ts::jni::AsyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        delete report;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.AbstractSyncReport
//----------------------------------------------------------------------------

// Method: io.tsduck.AbstractSyncReport.initNativeObject
// Signature: (Ljava/lang/String;I)V
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractSyncReport_initNativeObject(JNIEnv* env, jobject obj, jstring method, jint severity)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::jni::SyncReport* report = ts::jni::GetPointerField<ts::jni::SyncReport>(env, obj, "nativeObject");
    if (report == nullptr) {
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::jni::SyncReport(env, obj, method, int(severity)));
    }
}

// Method: io.tsduck.AbstractSyncReport.delete
// Signature: ()V
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractSyncReport_delete(JNIEnv* env, jobject obj)
{
    ts::jni::SyncReport* report = ts::jni::GetPointerField<ts::jni::SyncReport>(env, obj, "nativeObject");
    if (report != nullptr) {
        delete report;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

#endif // TS_NO_JAVA
