//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Asynchronous message report with notification to a Java class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAsyncReport.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)
namespace ts {
    namespace jni {
        //!
        //! Asynchronous message report with notification to a Java class.
        //! @ingroup libtsduck java
        //!
        class TSDUCKDLL AsyncReport : public ts::AsyncReport
        {
            TS_NOBUILD_NOCOPY(AsyncReport);
        public:
            //!
            //! Constructor.
            //! @param [in] env JNI environment.
            //! @param [in] obj A java object which will be notified of the log messages.
            //! The @a obj parameter is typically a "local reference" (in JNI parlance) to the Java object.
            //! The C++ object creates a "global reference" to it and keeps it until the destructor is called.
            //! @param [in] log_method A Java string containing the name of a method in the Java object.
            //! This method will be called with each log message. The Java profile of the method shall be
            //! @code void(int,String) @endcode (severity and message parameters).
            //! @param [in] max_severity Set initial level report to that level.
            //! @param [in] args Initial parameters.
            //!
            AsyncReport(JNIEnv* env, jobject obj, jstring log_method, int max_severity, const AsyncReportArgs& args = AsyncReportArgs());

            //!
            //! Destructor.
            //! Important: The destructor shall be called from the same Java thread as the constructor.
            //!
            virtual ~AsyncReport() override;

        private:
            // Inherited from ts::AsyncReport:
            virtual void asyncThreadLog(int severity, const UString& message) override;

            JNIEnv*   _env = nullptr;         // JNI environment in the thread which called the constructor.
            jobject   _obj_ref = nullptr;     // Global JNI reference to the Java object to notify.
            jmethodID _obj_method = nullptr;  // Method to log messages in the Java object.
        };
    }
}
#endif // TS_NO_JAVA
