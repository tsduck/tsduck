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

            JNIEnv*   _env;         // JNI environment in the thread which called the constructor.
            jobject   _obj_ref;     // Global JNI reference to the Java object to notify.
            jmethodID _obj_method;  // Method to log messages in the Java object.
        };
    }
}
#endif // TS_NO_JAVA
