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

#include "tsjni.h"
TSDUCK_SOURCE;

#if !defined(TS_NO_JAVA)

// The global pointer to the Java virtual machine.
JavaVM* ts::jni::javaVM = nullptr;

//----------------------------------------------------------------------------
// Convert between Java string and ts::UString.
//----------------------------------------------------------------------------

jstring ts::jni::ToJString(JNIEnv* env, const ts::UString& str)
{
    if (env == nullptr || env->ExceptionCheck()) {
        return nullptr;
    }
    else {
        return env->NewString(ToJChar(str), jsize(str.size()));
    }
}

ts::UString ts::jni::ToUString(JNIEnv* env, jstring str)
{
    if (env == nullptr || str == nullptr || env->ExceptionCheck()) {
        return ts::UString();
    }
    const jsize size = env->GetStringLength(str);
    const jchar* base = env->GetStringChars(str, nullptr);
    if (base == nullptr) {
        return ts::UString();
    }
    const ts::UString result(reinterpret_cast<const ts::UChar*>(base), size);
    env->ReleaseStringChars(str, base);
    return result;
}

//----------------------------------------------------------------------------
// Get/set the value of 'boolean' fields in a Java object.
//----------------------------------------------------------------------------

bool ts::jni::GetBoolField(JNIEnv* env, jobject obj, const char* fieldName)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || env->ExceptionCheck()) {
        return 0;
    }
    const jfieldID fid = env->GetFieldID(env->GetObjectClass(obj), fieldName, JCS_BOOLEAN);
    if (fid == nullptr) {
        return 0;
    }
    return bool(env->GetBooleanField(obj, fid));
}

bool ts::jni::SetBoolField(JNIEnv* env, jobject obj, const char* fieldName, bool value)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || env->ExceptionCheck()) {
        return false;
    }
    const jfieldID fid = env->GetFieldID(env->GetObjectClass(obj), fieldName, JCS_BOOLEAN);
    if (fid == nullptr) {
        return false;
    }
    env->SetBooleanField(obj, fid, jboolean(value));
    return !env->ExceptionCheck();
}

//----------------------------------------------------------------------------
// Get/set the value of 'int' fields in a Java object.
//----------------------------------------------------------------------------

jint ts::jni::GetIntField(JNIEnv* env, jobject obj, const char* fieldName)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || env->ExceptionCheck()) {
        return 0;
    }
    const jfieldID fid = env->GetFieldID(env->GetObjectClass(obj), fieldName, JCS_INT);
    if (fid == nullptr) {
        return 0;
    }
    return env->GetIntField(obj, fid);
}

bool ts::jni::SetIntField(JNIEnv* env, jobject obj, const char* fieldName, jint value)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || env->ExceptionCheck()) {
        return false;
    }
    const jfieldID fid = env->GetFieldID(env->GetObjectClass(obj), fieldName, JCS_INT);
    if (fid == nullptr) {
        return false;
    }
    env->SetIntField(obj, fid, value);
    return !env->ExceptionCheck();
}

//----------------------------------------------------------------------------
// Get/set the value of 'long' fields in a Java object.
//----------------------------------------------------------------------------

jlong ts::jni::GetLongField(JNIEnv* env, jobject obj, const char* fieldName)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || env->ExceptionCheck()) {
        return 0;
    }
    const jfieldID fid = env->GetFieldID(env->GetObjectClass(obj), fieldName, JCS_LONG);
    if (fid == nullptr) {
        return 0;
    }
    return env->GetLongField(obj, fid);
}

bool ts::jni::SetLongField(JNIEnv* env, jobject obj, const char* fieldName, jlong value)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || env->ExceptionCheck()) {
        return false;
    }
    const jfieldID fid = env->GetFieldID(env->GetObjectClass(obj), fieldName, JCS_LONG);
    if (fid == nullptr) {
        return false;
    }
    env->SetLongField(obj, fid, value);
    return !env->ExceptionCheck();
}

//----------------------------------------------------------------------------
// Get/set the value of object fields in a Java object.
//----------------------------------------------------------------------------

jobject ts::jni::GetObjectField(JNIEnv* env, jobject obj, const char* fieldName, const char* signature)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || signature == nullptr || env->ExceptionCheck()) {
        return nullptr;
    }
    const jfieldID fid = env->GetFieldID(env->GetObjectClass(obj), fieldName, signature);
    if (fid == nullptr) {
        return nullptr;
    }
    return env->GetObjectField(obj, fid);
}

bool ts::jni::SetObjectField(JNIEnv* env, jobject obj, const char* fieldName, const char* signature, jobject value)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || signature == nullptr || env->ExceptionCheck()) {
        return false;
    }
    const jfieldID fid = env->GetFieldID(env->GetObjectClass(obj), fieldName, signature);
    if (fid == nullptr) {
        return false;
    }
    env->SetObjectField(obj, fid, value);
    return !env->ExceptionCheck();
}

//----------------------------------------------------------------------------
// Get/set the value of 'String' fields in a Java object.
//----------------------------------------------------------------------------

ts::UString ts::jni::GetStringField(JNIEnv* env, jobject obj, const char* fieldName)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || env->ExceptionCheck()) {
        return UString();
    }
    const jstring jstr = jstring(GetObjectField(env, obj, fieldName, JCS_STRING));
    return jstr == nullptr ? UString() : ToUString(env, jstr);
}

bool ts::jni::SetStringField(JNIEnv* env, jobject obj, const char* fieldName, const ts::UString& value)
{
    if (env == nullptr || obj == nullptr || fieldName == nullptr || env->ExceptionCheck()) {
        return false;
    }
    const jstring jval = ToJString(env, value);
    return jval != nullptr && SetObjectField(env, obj, fieldName, JCS_STRING, jval);
}

#endif // TS_NO_JAVA
