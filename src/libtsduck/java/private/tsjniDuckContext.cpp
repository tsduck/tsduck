//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------
//
//  Native implementation of the Java class io.tsduck.DuckContext.
//
//----------------------------------------------------------------------------

#include "tsDuckContext.h"
#include "tsCerrReport.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)

//
// private native void initNativeObject(Report report);
//
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_initNativeObject(JNIEnv* env, jobject obj, jobject jreport)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (env != nullptr && duck == nullptr) {
        ts::Report* report = nullptr;
        if (jreport != nullptr) {
            report = ts::jni::GetPointerField<ts::Report>(env, jreport, "nativeObject");
        }
        if (report == nullptr) {
            report = &CERR;
        }
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::DuckContext(report));
    }
}

//
// public native void delete();
//
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_delete(JNIEnv* env, jobject obj)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        delete duck;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

//
// public native boolean setDefaultCharset(String charset);
//
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

//
// public native void setDefaultCASId(short cas);
//
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_setDefaultCASId(JNIEnv* env, jobject obj, jshort cas)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->setDefaultCASId(uint16_t(cas));
    }
}

//
// public native void setDefaultPDS(int pds);
//
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_setDefaultPDS(JNIEnv* env, jobject obj, jint pds)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->setDefaultPDS(ts::PDS(pds));
    }
}

//
// public native void addStandards(int mask);
//
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_addStandards(JNIEnv* env, jobject obj, jint mask)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->addStandards(ts::Standards(mask));
    }
}

//
// public native int standards();
//
TSDUCKJNI jint JNICALL Java_io_tsduck_DuckContext_standards(JNIEnv* env, jobject obj)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    return duck == nullptr ? 0 : jint(duck->standards());
}

//
// public native void resetStandards(int mask);
//
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_resetStandards(JNIEnv* env, jobject obj, jint mask)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->resetStandards(ts::Standards(mask));
    }
}

//
// public native void setTimeReferenceOffset(long offset);
//
TSDUCKJNI void JNICALL Java_io_tsduck_DuckContext_setTimeReferenceOffset(JNIEnv* env, jobject obj, jlong offset)
{
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeObject");
    if (duck != nullptr) {
        duck->setTimeReferenceOffset(ts::MilliSecond(offset));
    }
}

//
// public native boolean setTimeReference(String name);
//
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
