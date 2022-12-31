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
//  Native implementation of the Java class io.tsduck.SectionFile.
//
//----------------------------------------------------------------------------

#include "tsSectionFile.h"
#include "tsDuckContext.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)

//
// private native void initNativeObject(DuckContext duck);
//
TSDUCKJNI void JNICALL Java_io_tsduck_SectionFile_initNativeObject(JNIEnv* env, jobject obj, jobject jduck)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    if (sf == nullptr) {
        ts::DuckContext* duck = nullptr;
        if (jduck != nullptr) {
            duck = ts::jni::GetPointerField<ts::DuckContext>(env, jduck, "nativeObject");
        }
        if (duck == nullptr) {
            // We must have a DuckContext with SectionFile. Create one with all default values.
            duck = new ts::DuckContext;
            // And save it in the Java SectionFile object to delete it later.
            ts::jni::SetPointerField(env, obj, "nativeDuckContext", duck);
        }
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::SectionFile(*duck));
    }
}

//
// public native void delete();
//
TSDUCKJNI void JNICALL Java_io_tsduck_SectionFile_delete(JNIEnv* env, jobject obj)
{
    // Delete the SectionFile object.
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    if (sf != nullptr) {
        delete sf;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
    // If one DuckContext was allocated in the constructor, delete it as well.
    ts::DuckContext* duck = ts::jni::GetPointerField<ts::DuckContext>(env, obj, "nativeDuckContext");
    if (duck != nullptr) {
        delete duck;
        ts::jni::SetLongField(env, obj, "nativeDuckContext", 0);
    }
}

//
// public native void clear();
//
TSDUCKJNI void JNICALL Java_io_tsduck_SectionFile_clear(JNIEnv* env, jobject obj)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    if (sf != nullptr) {
        sf->clear();
    }
}

//
// public native int binarySize();
//
TSDUCKJNI jint JNICALL Java_io_tsduck_SectionFile_binarySize(JNIEnv* env, jobject obj)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return sf == nullptr ? 0 : jint(sf->binarySize());
}

//
// public native int sectionsCount();
//
TSDUCKJNI jint JNICALL Java_io_tsduck_SectionFile_sectionsCount(JNIEnv* env, jobject obj)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return sf == nullptr ? 0 : jint(sf->sectionsCount());
}

//
// public native int tablesCount();
//
TSDUCKJNI jint JNICALL Java_io_tsduck_SectionFile_tablesCount(JNIEnv* env, jobject obj)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return sf == nullptr ? 0 : jint(sf->tablesCount());
}

//
// public native void setCRCValidation(int mode);
//
TSDUCKJNI void Java_io_tsduck_SectionFile_setCRCValidation(JNIEnv* env, jobject obj, jint mode)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    if (sf != nullptr) {
        sf->setCRCValidation(ts::CRC32::Validation(mode));
    }
}

//
// public native boolean fromBinary(byte[] data);
//
TSDUCKJNI jboolean JNICALL Java_io_tsduck_SectionFile_fromBinary(JNIEnv* env, jobject obj, jbyteArray jdata)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    if (sf == nullptr) {
        return false;
    }
    else {
        void* data = env->GetPrimitiveArrayCritical(jdata, nullptr);
        const bool result = sf->loadBuffer(data, size_t(env->GetArrayLength(jdata)));
        env->ReleasePrimitiveArrayCritical(jdata, data, JNI_ABORT);
        return result;
    }
}

//
// public native byte[] toBinary();
//
TSDUCKJNI jbyteArray JNICALL Java_io_tsduck_SectionFile_toBinary(JNIEnv* env, jobject obj)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    if (sf == nullptr) {
        return nullptr;
    }
    else {
        const size_t size = sf->binarySize();
        const jbyteArray result = env->NewByteArray(jsize(size));
        void* data = env->GetPrimitiveArrayCritical(result, nullptr);
        sf->saveBuffer(data, size);
        env->ReleasePrimitiveArrayCritical(result, data, 0);
        return result;
    }
}

//
// public native boolean loadBinary(String file);
//
TSDUCKJNI jboolean JNICALL Java_io_tsduck_SectionFile_loadBinary(JNIEnv* env, jobject obj, jstring jname)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return sf != nullptr && sf->loadBinary(ts::jni::ToUString(env, jname));
}

//
// public native boolean saveBinary(String file);
//
TSDUCKJNI jboolean JNICALL Java_io_tsduck_SectionFile_saveBinary(JNIEnv* env, jobject obj, jstring jname)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return sf != nullptr && sf->saveBinary(ts::jni::ToUString(env, jname));
}

//
// public native boolean loadXML(String file);
//
TSDUCKJNI jboolean JNICALL Java_io_tsduck_SectionFile_loadXML(JNIEnv* env, jobject obj, jstring jname)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return sf != nullptr && sf->loadXML(ts::jni::ToUString(env, jname));
}

//
// public native boolean saveXML(String file);
//
TSDUCKJNI jboolean JNICALL Java_io_tsduck_SectionFile_saveXML(JNIEnv* env, jobject obj, jstring jname)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return sf != nullptr && sf->saveXML(ts::jni::ToUString(env, jname));
}

//
// public native boolean saveJSON(String file);
//
TSDUCKJNI jboolean JNICALL Java_io_tsduck_SectionFile_saveJSON(JNIEnv* env, jobject obj, jstring jname)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return sf != nullptr && sf->saveJSON(ts::jni::ToUString(env, jname));
}

//
// public native String toXML();
//
TSDUCKJNI jstring JNICALL Java_io_tsduck_SectionFile_toXML(JNIEnv* env, jobject obj)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return ts::jni::ToJString(env, sf != nullptr ? sf->toXML() : ts::UString());
}

//
// public native String toJSON();
//
TSDUCKJNI jstring JNICALL Java_io_tsduck_SectionFile_toJSON(JNIEnv* env, jobject obj)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    return ts::jni::ToJString(env, sf != nullptr ? sf->toJSON() : ts::UString());
}

//
// public native void reorganizeEITs(int year, int month, int day);
//
TSDUCKJNI void JNICALL Java_io_tsduck_SectionFile_reorganizeEITs(JNIEnv* env, jobject obj, jint year, jint month, jint day)
{
    ts::SectionFile* sf = ts::jni::GetPointerField<ts::SectionFile>(env, obj, "nativeObject");
    if (sf != nullptr) {
        ts::Time reftime;
        if (year > 0 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
            reftime = ts::Time::Fields(int(year), int(month), int(day));
        }
        sf->reorganizeEITs(reftime);
    }
}

#endif // TS_NO_JAVA
