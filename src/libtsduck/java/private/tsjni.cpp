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

#include "tsjni.h"
#include "tsCerrReport.h"
#include "tsThreadLocalObjects.h"

#if !defined(TS_NO_JAVA)

// The global pointer to the Java virtual machine.
JavaVM* ts::jni::javaVM = nullptr;

//----------------------------------------------------------------------------
// Convert between Java string and ts::UString.
//----------------------------------------------------------------------------

jstring ts::jni::ToJString(JNIEnv* env, const UString& str)
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


//----------------------------------------------------------------------------
// Get a plugin description from a Java array of string.
//----------------------------------------------------------------------------

bool ts::jni::GetPluginOptions(JNIEnv* env, jobjectArray strings, ts::PluginOptions& plugin)
{
    plugin.clear();
    if (env == nullptr || strings == nullptr || env->ExceptionCheck()) {
        return false;
    }
    const jsize count = env->GetArrayLength(strings);
    if (count > 0) {
        plugin.name = ts::jni::ToUString(env, jstring(env->GetObjectArrayElement(strings, 0)));
        plugin.args.resize(size_t(count - 1));
        for (jsize i = 1; i < count; ++i) {
            plugin.args[i-1] = ts::jni::ToUString(env, jstring(env->GetObjectArrayElement(strings, i)));
        }
    }
    return !plugin.name.empty();
}

bool ts::jni::GetPluginOptionsVector(JNIEnv* env, jobjectArray strings, PluginOptionsVector& plugins)
{
    const jsize count = strings != nullptr ? env->GetArrayLength(strings) : 0;
    plugins.resize(size_t(count));
    bool ok = true;
    for (jsize i = 0; ok && i < count; ++i) {
        ok = GetPluginOptions(env, jobjectArray(env->GetObjectArrayElement(strings, i)), plugins[i]);
    }
    return true;
}


//----------------------------------------------------------------------------
// A private class which manages the JNIEnv pointer for the current thread.
//----------------------------------------------------------------------------

namespace {

    // Class declaration, one instance per thread.
    class LocalThreadJNI : public ts::Object
    {
    public:
        // The constructor attaches to the JVM when necessary.
        LocalThreadJNI();
        LocalThreadJNI(const LocalThreadJNI&) = default;
        LocalThreadJNI& operator=(const LocalThreadJNI&) = default;

        // The destructor detaches from the JVM when necessary.
        virtual ~LocalThreadJNI() override;

        // Get the JNIEnv point for the curren thread.
        JNIEnv* env() const { return _env; }

    private:
        JNIEnv* _env;         // The JNI environment pointer for this thread.
        bool    _detach_jvm;  // The current thread shall detach from the JVM before exit.
    };

    // The constructor attaches to the JVM when necessary.
    LocalThreadJNI::LocalThreadJNI() :
        _env(nullptr),
        _detach_jvm(false)
    {
        if (ts::jni::javaVM != nullptr) {
            void* penv = nullptr;
            jint status = ts::jni::javaVM->GetEnv(&penv, JNI_VERSION_1_2);
            if (status != JNI_OK || penv == nullptr) {
                // Thread not attached, this is a native thread, attach it now.
                status = ts::jni::javaVM->AttachCurrentThread(&penv, nullptr);
                _detach_jvm = true;
            }
            if (status == JNI_OK && penv != nullptr) {
                _env = reinterpret_cast<JNIEnv*>(penv);
            }
        }
        CERR.debug(u"start of JNI thread: jvm: 0x%X, env: 0x%X, detach: %s", {ptrdiff_t(ts::jni::javaVM), ptrdiff_t(_env), _detach_jvm});
    }

    // The destructor detaches from the JVM when necessary.
    LocalThreadJNI::~LocalThreadJNI()
    {
        CERR.debug(u"end of JNI thread: jvm: 0x%X, env: 0x%X, detach: %s", {ptrdiff_t(ts::jni::javaVM), ptrdiff_t(_env), _detach_jvm});
        _env = nullptr;
        if (_detach_jvm && ts::jni::javaVM != nullptr) {
            _detach_jvm = false;
            ts::jni::javaVM->DetachCurrentThread();
        }
    }
}


//----------------------------------------------------------------------------
// Get the JNIEnv pointer for the current thread.
//----------------------------------------------------------------------------

JNIEnv* ts::jni::JNIEnvForCurrentThead()
{
    // Get the thread-specific LocalThreadJNI.
    ts::ObjectPtr obj(ThreadLocalObjects::Instance()->getLocalObject(u"LocalThreadJNI"));
    LocalThreadJNI* lobj = dynamic_cast<LocalThreadJNI*>(obj.pointer());
    if (lobj == nullptr) {
        // First time we use this thread, create the thread-specific LocalThreadJNI.
        obj = lobj = new LocalThreadJNI;
        CheckNonNull(lobj);
        CheckNonNull(lobj->env());
        ThreadLocalObjects::Instance()->setLocalObject(u"LocalThreadJNI", obj);
    }
    return lobj->env();
}

#endif // TS_NO_JAVA
