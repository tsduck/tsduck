//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin event handler with forwarding to a Java class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginEventHandlerInterface.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)
namespace ts {
    namespace jni {
        //!
        //! Plugin event handler with forwarding to a Java class.
        //! @ingroup libtsduck java
        //!
        class TSDUCKDLL PluginEventHandler : public ts::PluginEventHandlerInterface
        {
            TS_NOBUILD_NOCOPY(PluginEventHandler);
        public:
            //!
            //! Constructor.
            //! @param [in] env JNI environment.
            //! @param [in] obj A java object which will handle plugin events.
            //! The @a obj parameter is typically a "local reference" (in JNI parlance) to the Java object.
            //! The C++ object creates a "global reference" to it and keeps it until the destructor is called.
            //! @param [in] handle_method A Java string containing the name of a method in the Java object.
            //! This method will be called for each plugin event. The Java profile of the method shall be
            //! @code
            //! boolean handlePluginEvent(PluginEventContext context, byte[] data);
            //! @endcode
            //!
            PluginEventHandler(JNIEnv* env, jobject obj, jstring handle_method);

            //!
            //! Destructor.
            //! Important: The destructor shall be called from the same Java thread as the constructor.
            //!
            virtual ~PluginEventHandler() override;

        private:
            // Inherited from ts::PluginEventHandlerInterface
            virtual void handlePluginEvent(const PluginEventContext& context) override;

            bool      _valid = false;              // If true, all JNI references are valid.
            JNIEnv*   _env = nullptr;              // JNI environment in the thread which called the constructor.
            jobject   _obj_ref = nullptr;          // Global JNI reference to the Java object to notify.
            jmethodID _obj_method = nullptr;       // Method to handle events in the Java object.
            jclass    _pec_class = nullptr;        // Global reference to Java class io.tsduck.PluginEventContext
            jmethodID _pec_constructor = nullptr;  // Constructor method to create a io.tsduck.PluginEventContext
            jfieldID  _pec_outdata = nullptr;      // Internal private field "_outputData" in io.tsduck.PluginEventContext
        };
    }
}
#endif // TS_NO_JAVA
