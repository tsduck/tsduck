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
//  Native implementation of the Java class io.tsduck.DuckContext.
//
//----------------------------------------------------------------------------

#include "tsDuckContext.h"
#include "tsCerrReport.h"
#include "tsjni.h"
TSDUCK_SOURCE;

#if !defined(TS_NO_JAVA)

// Method: io.tsduck.DuckContext.initNativeObject
// Signature: (Lio/tsduck/Report;)V
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_initNativeObject(JNIEnv* env, jobject obj, jobject jreport)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck == nullptr) {
        ts::Report* report = nullptr;
        if (jreport != nullptr) {
            report = ts::jni::GetPointerField<ts::Report>(env, jreport, "nativeObject");
        }
        if (report == nullptr) {
            report = ts::CerrReport::Instance();
        }
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::DuckContext(report));
    }
}

// Method: io.tsduck.DuckContext.delete
// Signature: ()V
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_delete(JNIEnv* env, jobject obj)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        delete duck;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

// Method: io.tsduck.DuckContext.setDefaultCharset
// Signature: (Ljava/lang/String;)B
TSDUCKJNI jboolean JNICALL Java_io_tsduck_DuckContext_setDefaultCharset(JNIEnv* env, jobject obj, jstring jname)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        const ts::UString name(ts::jni::ToUString(env, jname));
        const ts::Charset* charset = ts::Charset::GetCharset(name);
        if (charset != nullptr) {
            duck->setDefaultCharsetIn(charset);
            duck->setDefaultCharsetOut(charset);
            return true;
        }
        duck->report().error(u"unknown character set \"%s\"", {name});
    }
    return false;
}

// Method: io.tsduck.DuckContext.setDefaultCASId
// Signature: (S)V
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_setDefaultCASId(JNIEnv* env, jobject obj, jshort cas)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->setDefaultCASId(uint16_t(cas));
    }
}

// Method: io.tsduck.DuckContext.setDefaultPDS
// Signature: (I)V
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_setDefaultPDS(JNIEnv* env, jobject obj, jint pds)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->setDefaultPDS(ts::PDS(pds));
    }
}

// Method: io.tsduck.DuckContext.addStandards
// Signature: (I)V
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_addStandards(JNIEnv* env, jobject obj, jint mask)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->addStandards(ts::Standards(mask));
    }
}

// Method: io.tsduck.DuckContext.standards
// Signature: ()I
TSDUCKJNI jint JNICALL Java_io_tsduck_DuckContext_standards(JNIEnv* env, jobject obj)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    return duck == nullptr ? 0 : jint(duck->standards());
}

// Method: io.tsduck.DuckContext.resetStandards
// Signature: (I)V
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_resetStandards(JNIEnv* env, jobject obj, jint mask)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->resetStandards(ts::Standards(mask));
    }
}

// Method: io.tsduck.DuckContext.setTimeReferenceOffset
// Signature: (J)V
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_setTimeReferenceOffset(JNIEnv* env, jobject obj, jlong offset)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->setTimeReferenceOffset(ts::MilliSecond(offset));
    }
}

// Method: io.tsduck.DuckContext.setTimeReference
// Signature: (Ljava/lang/String;)B
TSDUCKJNI jboolean JNICALL Java_io_tsduck_DuckContext_setTimeReference(JNIEnv* env, jobject obj, jstring jname)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        const ts::UString str(ts::jni::ToUString(env, jname));
        if (duck->setTimeReference(str)) {
            return true;
        }
        duck->report().error(u"invalid time reference \"%s\"", {str});
    }
    return false;
}

#endif // TS_NO_JAVA
