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
//  Native implementation of the Java class io.tsduck.NativeLibrary.
//
//----------------------------------------------------------------------------

#include "tsjni.h"
TSDUCK_SOURCE;

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Interface of native methods.
//----------------------------------------------------------------------------

extern "C" {
    // Load/unload notification of the JNI library.
    JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*);
    JNIEXPORT void JNICALL JNI_OnUnload(JavaVM*, void*);

    // Method: io.tsduck.NativeLibrary.initialize
    // Signature: ()V
    JNIEXPORT void JNICALL Java_io_tsduck_NativeLibrary_initialize(JNIEnv*, jclass);
}

//----------------------------------------------------------------------------
// Initialization of the JNI library.
//----------------------------------------------------------------------------

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    ts::jni::javaVM = vm;
    return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved)
{
    ts::jni::javaVM = nullptr;
}

//----------------------------------------------------------------------------
// Implementation of native methods.
//----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_io_tsduck_NativeLibrary_initialize(JNIEnv *env, jclass clazz)
{
    // Currently, there is nothing to initialize.
}

#endif // TS_NO_JAVA
