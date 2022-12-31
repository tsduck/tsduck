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
//!
//!  @file
//!  @ingroup java
//!  Base definitions for the TSDuck Java bindings (JNI C++ implementation).
//!  JNI utilitities for other JNI modules.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsPluginOptions.h"

#if !defined(TS_NO_JAVA)
#include <jni.h>

//!
//! @hideinitializer
//! Attribute to export a JNI native function to Java.
//!
#define TSDUCKJNI \
    TS_GCC_NOWARNING(missing-prototypes) \
    TS_LLVM_NOWARNING(missing-prototypes) \
    extern "C" JNIEXPORT

//
// Java Class Names (JCN) in JNI notation.
//
#define JCN_CLASS  "java/lang/Class"
#define JCN_OBJECT "java/lang/Object"
#define JCN_STRING "java/lang/String"
#define JCN_PLUGIN_EVENT_CONTEXT "io/tsduck/PluginEventContext"

//
// Java Class Signatures (JCS) in JNI notation.
//
#define JCS(name)       "L" name ";"
#define JCS_ARRAY(jcs)  "[" jcs
#define JCS_BOOLEAN     "Z"
#define JCS_BYTE        "B"
#define JCS_CHAR        "C"
#define JCS_SHORT       "S"
#define JCS_INT         "I"
#define JCS_LONG        "J"
#define JCS_FLOAT       "F"
#define JCS_DOUBLE      "D"
#define JCS_VOID        "V"
#define JCS_CLASS       JCS(JCN_CLASS)
#define JCS_OBJECT      JCS(JCN_OBJECT)
#define JCS_STRING      JCS(JCN_STRING)
#define JCS_CONSTRUCTOR "<init>"

namespace ts {
    //!
    //! Namespace for TSDuck JNI support functions
    //!
    namespace jni {
        //!
        //! A global pointer to the Java virtual machine.
        //! Null pointer if JNI is not properly initialized.
        //!
        extern JavaVM* javaVM;

        //!
        //! Get the JNIEnv pointer for the current thread.
        //! If the thread is a native one and is not yet attached to the JVM, attachment is done first.
        //! Non-native threads are automatically detached from the JVM.
        //! @return The JNIEnv pointer for the current thread.
        //!
        JNIEnv* JNIEnvForCurrentThead();

        //!
        //! Get the address of the first character in a string as a Java character.
        //! This is based on the fact that ts::UString and java.lang.String use the same
        //! representation for characters.
        //! @param [in] str A C++ unicode string.
        //! @return A constant pointer to the first character in the string.
        //!
        inline const jchar* ToJChar(const std::u16string& str) { return reinterpret_cast<const jchar*>(str.c_str()); }

        //!
        //! Convert a Java string into a ts::UString.
        //! @param [in,out] env JNI callback environment.
        //! @param [in] str A Java string.
        //! @return The converted string. Use env->ExceptionCheck() to check for
        //! error. In case of error, the returned value is the empty string.
        //!
        UString ToUString(JNIEnv* env, jstring str);

        //!
        //! Convert a ts::UString into a Java string.
        //! @param [in,out] env JNI callback environment.
        //! @param [in] str A Java string.
        //! @return The converted string. Use env->ExceptionCheck() to check for
        //! error. In case of error, the returned value is a null pointer.
        //!
        jstring ToJString(JNIEnv* env, const UString& str);

        //!
        //! Get the value of a 'boolean' field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in] obj Java object from which to get a field.
        //! @param [in] fieldName Name of the object field to get.
        //! @return The value from the object field. Use env->ExceptionCheck() to
        //! check for error. In case of error, the returned value is zero.
        //!
        bool GetBoolField(JNIEnv* env, jobject obj, const char* fieldName);

        //!
        //! Set the value of a 'boolean' field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in,out] obj Java object from which to set a field.
        //! @param [in] fieldName Name of the object field to set.
        //! @param [in] value Value to set in the field.
        //! @return True on success, false on error.
        //!
        bool SetBoolField(JNIEnv* env, jobject obj, const char* fieldName, bool value);

        //!
        //! Get the value of an 'int' field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in] obj Java object from which to get a field.
        //! @param [in] fieldName Name of the object field to get.
        //! @return The value from the object field. Use env->ExceptionCheck() to
        //! check for error. In case of error, the returned value is zero.
        //!
        jint GetIntField(JNIEnv* env, jobject obj, const char* fieldName);

        //!
        //! Set the value of an 'int' field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in,out] obj Java object from which to set a field.
        //! @param [in] fieldName Name of the object field to set.
        //! @param [in] value Value to set in the field.
        //! @return True on success, false on error.
        //!
        bool SetIntField(JNIEnv* env, jobject obj, const char* fieldName, jint value);

        //!
        //! Get the value of a 'long' field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in] obj Java object from which to get a field.
        //! @param [in] fieldName Name of the object field to get.
        //! @return The value from the object field. Use env->ExceptionCheck() to
        //! check for error. In case of error, the returned value is zero.
        //!
        jlong GetLongField(JNIEnv* env, jobject obj, const char* fieldName);

        //!
        //! Set the value of a 'long' field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in,out] obj Java object from which to set a field.
        //! @param [in] fieldName Name of the object field to set.
        //! @param [in] value Value to set in the field.
        //! @return True on success, false on error.
        //!
        bool SetLongField(JNIEnv* env, jobject obj, const char* fieldName, jlong value);

        //!
        //! Get the value of a pointer field in a Java object.
        //! The actual Java type of the field shall be 'long'.
        //! @tparam T A class type.
        //! @param [in,out] env JNI callback environment.
        //! @param [in] obj Java object from which to get a field.
        //! @param [in] fieldName Name of the object field to get.
        //! @return The value from the object field. Use env->ExceptionCheck() to
        //! check for error. In case of error, the returned value is a null pointer.
        //!
        template<class T>
        inline T* GetPointerField(JNIEnv* env, jobject obj, const char* fieldName)
        {
            return reinterpret_cast<T*>(GetLongField(env, obj, fieldName));
        }

        //!
        //! Set the value of a pointer field in a Java object.
        //! The actual Java type of the field shall be 'long'.
        //! @tparam T A class type.
        //! @param [in,out] env JNI callback environment.
        //! @param [in,out] obj Java object from which to set a field.
        //! @param [in] fieldName Name of the object field to set.
        //! @param [in] value Value to set in the field.
        //! @return True on success, false on error.
        //!
        template<class T>
        inline bool SetPointerField(JNIEnv* env, jobject obj, const char* fieldName, const T* value)
        {
            TS_PUSH_WARNING()
            TS_MSC_NOWARNING(4826) // Conversion from 'const T *' to 'jlong' is sign-extended. This may cause unexpected runtime behavior.
            return SetLongField(env, obj, fieldName, reinterpret_cast<jlong>(value));
            TS_POP_WARNING()
        }

        //!
        //! Get the value of a field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in] obj Java object from which to get a field.
        //! @param [in] fieldName Name of the object field to get.
        //! @param [in] signature Type of the field in JNI notation.
        //! @return The value from the object field. Use env->ExceptionCheck() to
        //! check for error. In case of error, the returned value is a null pointer.
        //!
        jobject GetObjectField(JNIEnv* env, jobject obj, const char* fieldName, const char* signature);

        //!
        //! Set the value of a field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in,out] obj Java object from which to set a field.
        //! @param [in] fieldName Name of the object field to set.
        //! @param [in] signature Type of the field in JNI notation.
        //! @param [in] value Value to set in the field.
        //! @return True on success, false on error.
        //!
        bool SetObjectField(JNIEnv* env, jobject obj, const char* fieldName, const char* signature, jobject value);

        //!
        //! Get the value of a 'String' field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in] obj Java object from which to get a field.
        //! @param [in] fieldName Name of the object field to get.
        //! @return The value from the object field. Use env->ExceptionCheck() to
        //! check for error. In case of error, the returned value is an empty string.
        //!
        ts::UString GetStringField(JNIEnv* env, jobject obj, const char* fieldName);

        //!
        //! Set the value of a 'String' field in a Java object.
        //! @param [in,out] env JNI callback environment.
        //! @param [in,out] obj Java object from which to set a field.
        //! @param [in] fieldName Name of the object field to set.
        //! @param [in] value Value to set in the field.
        //! @return True on success, false on error.
        //!
        bool SetStringField(JNIEnv* env, jobject obj, const char* fieldName, const ts::UString& value);

        //!
        //! Get a plugin description from a Java array of string.
        //! @param [in,out] env JNI callback environment.
        //! @param [in,out] strings Java array of strings.
        //! @param [out] plugin Decoded plugin description.
        //! @return True on success, false on error.
        //!
        bool GetPluginOptions(JNIEnv* env, jobjectArray strings, PluginOptions& plugin);

        //!
        //! Get a vector of plugin descriptions from a Java array of string.
        //! @param [in,out] env JNI callback environment.
        //! @param [in,out] strings Java array of arrays of strings.
        //! @param [out] plugins Decoded plugins descriptions.
        //! @return True on success, false on error.
        //!
        bool GetPluginOptionsVector(JNIEnv* env, jobjectArray strings, PluginOptionsVector& plugins);
    }
}

#endif // TS_NO_JAVA
