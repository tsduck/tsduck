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
//  Native implementation of the Java class io.tsduck.PluginEventHandlerRegistry.
//
//----------------------------------------------------------------------------

#include "tsPluginEventHandlerRegistry.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)

//
// public native void registerEventHandler(AbstractPluginEventHandler handler, int eventCode);
//
TSDUCKJNI void JNICALL Java_io_tsduck_PluginEventHandlerRegistry_registerEventHandler(JNIEnv* env, jobject obj, jobject jhandler, jint eventCode)
{
    ts::PluginEventHandlerRegistry* tsp = ts::jni::GetPointerField<ts::PluginEventHandlerRegistry>(env, obj, "nativeObject");
    ts::PluginEventHandlerInterface* handler = ts::jni::GetPointerField<ts::PluginEventHandlerInterface>(env, jhandler, "nativeObject");
    if (tsp != nullptr && handler != nullptr) {
        tsp->registerEventHandler(handler, uint32_t(eventCode));
    }
}

//
// public native void registerInputEventHandler(AbstractPluginEventHandler handler);
//
TSDUCKJNI void JNICALL Java_io_tsduck_PluginEventHandlerRegistry_registerInputEventHandler(JNIEnv* env, jobject obj, jobject jhandler)
{
    ts::PluginEventHandlerRegistry* tsp = ts::jni::GetPointerField<ts::PluginEventHandlerRegistry>(env, obj, "nativeObject");
    ts::PluginEventHandlerInterface* handler = ts::jni::GetPointerField<ts::PluginEventHandlerInterface>(env, jhandler, "nativeObject");
    if (tsp != nullptr && handler != nullptr) {
        tsp->registerEventHandler(handler, ts::PluginType::INPUT);
    }
}

//
// public native void registerOutputEventHandler(AbstractPluginEventHandler handler);
//
TSDUCKJNI void JNICALL Java_io_tsduck_PluginEventHandlerRegistry_registerOutputEventHandler(JNIEnv* env, jobject obj, jobject jhandler)
{
    ts::PluginEventHandlerRegistry* tsp = ts::jni::GetPointerField<ts::PluginEventHandlerRegistry>(env, obj, "nativeObject");
    ts::PluginEventHandlerInterface* handler = ts::jni::GetPointerField<ts::PluginEventHandlerInterface>(env, jhandler, "nativeObject");
    if (tsp != nullptr && handler != nullptr) {
        tsp->registerEventHandler(handler, ts::PluginType::OUTPUT);
    }
}

#endif // TS_NO_JAVA
