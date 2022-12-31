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

#include "tsjniPluginEventHandler.h"
#include "tsPluginEventData.h"

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::jni::PluginEventHandler::PluginEventHandler(JNIEnv* env, jobject obj, jstring handle_method) :
    _valid(false),
    _env(env),
    _obj_ref(env == nullptr || obj == nullptr ? nullptr : env->NewGlobalRef(obj)),
    _obj_method(nullptr),
    _pec_class(nullptr),
    _pec_constructor(nullptr),
    _pec_outdata(nullptr)
{
    if (_obj_ref != nullptr) {
        const char* const handle_str = env->GetStringUTFChars(handle_method, nullptr);
        // Cache the method id of the handler method in the io.tsduck.PluginEventContext class.
        if (handle_str != nullptr) {
            // Expected profile: boolean handlePluginEvent(PluginEventContext context, byte[] data);
            _obj_method = env->GetMethodID(env->GetObjectClass(_obj_ref), handle_str, "(" JCS(JCN_PLUGIN_EVENT_CONTEXT) JCS_ARRAY(JCS_BYTE) ")" JCS_BOOLEAN);
            env->ReleaseStringUTFChars(handle_method, handle_str);
        }
        // Get a global reference to class io.tsduck.PluginEventContext.
        jclass clazz = env->FindClass(JCN_PLUGIN_EVENT_CONTEXT);
        if (clazz != nullptr) {
            // Get a global refernce from the local reference
            _pec_class = jclass(env->NewGlobalRef(clazz));
            // And free the local reference.
            env->DeleteLocalRef(clazz);
            // Get the id of the constructor:
            // PluginEventContext(int ecode, String pname, int pindex, int pcount, int brate, long ppackets, long tpackets, boolean rdonly, int maxdsize)
            _pec_constructor = env->GetMethodID(_pec_class, JCS_CONSTRUCTOR, "(" JCS_INT JCS_STRING JCS_INT JCS_INT JCS_INT JCS_LONG JCS_LONG JCS_BOOLEAN JCS_INT ")" JCS_VOID);
            // Get the id of the private field "byte[] _outputData":
            _pec_outdata = env->GetFieldID(_pec_class, "_outputData", JCS_ARRAY(JCS_BYTE));
        }
    }
    _valid = _env != nullptr && _obj_ref != nullptr && _obj_method != nullptr && _pec_class != nullptr && _pec_constructor != nullptr && _pec_outdata != nullptr;
}

ts::jni::PluginEventHandler::~PluginEventHandler()
{
    if (_env != nullptr) {
        if (_obj_ref != nullptr) {
            _env->DeleteGlobalRef(_obj_ref);
            _obj_ref = nullptr;
        }
        if (_pec_class != nullptr) {
            _env->DeleteGlobalRef(_pec_class);
            _pec_class = nullptr;
            _pec_constructor = nullptr;
            _pec_outdata = nullptr;
        }
    }
}


//----------------------------------------------------------------------------
// Event handling method.
//----------------------------------------------------------------------------

void ts::jni::PluginEventHandler::handlePluginEvent(const PluginEventContext& context)
{
    JNIEnv* env = JNIEnvForCurrentThead();
    if (env != nullptr && _valid) {
        PluginEventData* event_data = dynamic_cast<PluginEventData*>(context.pluginData());
        const bool valid_data = event_data != nullptr && event_data->data() != nullptr;
        const bool read_only_data = event_data == nullptr || event_data->readOnly();
        const jsize data_size = valid_data ? jsize(event_data->size()) : 0;
        const jsize max_data_size = read_only_data ? 0 : jsize(event_data->maxSize());
        const jstring jname = ToJString(env, context.pluginName());

        // Build an instance of io.tsduck.PluginEventContext. The constructor is:
        // PluginEventContext(int ecode, String pname, int pindex, int pcount, int brate, long ppackets, long tpackets)
        const jobject pec = env->NewObject(_pec_class, _pec_constructor,
                                           jint(context.eventCode()), jname,
                                           jint(context.pluginIndex()), jint(context.pluginCount()),
                                           jint(context.bitrate().toInt()),
                                           jlong(context.pluginPackets()),
                                           jlong(context.totalPackets()),
                                           jboolean(read_only_data),
                                           jint(max_data_size));

        // Build a Java bytes[] containing the plugin data.
        const jbyteArray jdata = env->NewByteArray(data_size);
        if (jdata != nullptr && valid_data && data_size > 0) {
            env->SetByteArrayRegion(jdata, 0, data_size, reinterpret_cast<const jbyte*>(event_data->data()));
        }

        // Call the Java event handler.
        jboolean success = true;
        if (pec != nullptr && jdata != nullptr) {
            success = env->CallBooleanMethod(_obj_ref, _obj_method, pec, jdata);
        }

        // If the event data are modifiable, check if the Java handler set some output data.
        if (success && valid_data && !read_only_data) {
            const jbyteArray joutdata = jbyteArray(env->GetObjectField(pec, _pec_outdata));
            if (joutdata != nullptr) {
                // There are some output data which were set by the Java event handler.
                const jsize outsize = env->GetArrayLength(joutdata);
                if (outsize <= max_data_size) {
                    env->GetByteArrayRegion(joutdata, 0, outsize, reinterpret_cast<jbyte*>(event_data->outputData()));
                    event_data->updateSize(size_t(outsize));
                }
                env->DeleteLocalRef(joutdata);
            }
        }

        // Free local references.
        if (jdata != nullptr) {
            env->DeleteLocalRef(jdata);
        }
        if (pec != nullptr) {
            env->DeleteLocalRef(pec);
        }
        if (jname != nullptr) {
            env->DeleteLocalRef(jname);
        }

        // Set error indicator if the Java callback returned false.
        if (!success && event_data != nullptr) {
            event_data->setError(true);
        }
    }
}


//----------------------------------------------------------------------------
// Implementation of native methods of Java class io.tsduck.AbstractPluginEventHandler
//----------------------------------------------------------------------------

//
// private native void initNativeObject(String methodName);
//
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractPluginEventHandler_initNativeObject(JNIEnv* env, jobject obj, jstring method)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::jni::PluginEventHandler* handler = ts::jni::GetPointerField<ts::jni::PluginEventHandler>(env, obj, "nativeObject");
    if (env != nullptr && handler == nullptr) {
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::jni::PluginEventHandler(env, obj, method));
    }
}

//
// public native void delete();
//
TSDUCKJNI void JNICALL Java_io_tsduck_AbstractPluginEventHandler_delete(JNIEnv* env, jobject obj)
{
    ts::jni::PluginEventHandler* handler = ts::jni::GetPointerField<ts::jni::PluginEventHandler>(env, obj, "nativeObject");
    if (handler != nullptr) {
        delete handler;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

#endif // TS_NO_JAVA
