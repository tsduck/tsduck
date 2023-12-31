//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------
//
//  Native implementation of the Java class io.tsduck.NativeLibrary.
//
//----------------------------------------------------------------------------

#include "tsjni.h"

#if !defined(TS_NO_JAVA)

//
// Initialization of the JNI library.
//
TSDUCKJNI jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    ts::jni::javaVM = vm;
    return JNI_VERSION_1_2;
}

//
// Termination of the JNI library.
//
TSDUCKJNI void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved)
{
    ts::jni::javaVM = nullptr;
}

//
// private static native void initialize();
//
TSDUCKJNI void JNICALL Java_io_tsduck_NativeLibrary_initialize(JNIEnv *env, jclass clazz)
{
    // Currently, there is nothing to initialize.
}

#endif // TS_NO_JAVA
