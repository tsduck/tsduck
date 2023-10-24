//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Synchronous message report with notification to a Java class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)
namespace ts {
    namespace jni {
        //!
        //! Synchronous message report with notification to a Java class.
        //! @ingroup java
        //!
        class TSDUCKDLL SyncReport : public ts::Report
        {
            TS_NOBUILD_NOCOPY(SyncReport);
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
            //!
            SyncReport(JNIEnv* env, jobject obj, jstring log_method, int max_severity);

            //!
            //! Destructor.
            //! Important: The destructor shall be called from the same Java thread as the constructor.
            //!
            virtual ~SyncReport() override;

        private:
            // Inherited from ts::Report:
            virtual void writeLog(int severity, const UString& message) override;

            JNIEnv*   _env = nullptr;         // JNI environment in the thread which called the constructor.
            jobject   _obj_ref = nullptr;     // Global JNI reference to the Java object to notify.
            jmethodID _obj_method = nullptr;  // Method to log messages in the Java object.
        };
    }
}
#endif // TS_NO_JAVA
