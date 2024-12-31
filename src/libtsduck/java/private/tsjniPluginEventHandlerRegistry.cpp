//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
