//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the ts::ThreadLocalObjects singleton.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSingletonManager.h"
#include "tsObject.h"

namespace ts {
    //!
    //! Thread local objects.
    //! @ingroup thread
    //!
    //! System-agnostic interface to "thread local storage" (Windows) or "thread specific data" (POSIX thread).
    //! This class is a singleton which gives access to all local objects of the current thread.
    //!
    //! When a thread terminates, all its local objects which were inserted using this singleton are deleted.
    //!
    //! The actual time where these deletions occur depends on the thread and the operating system.
    //!
    //! - If the thread is an instance of ts::Thread, the deletions occur when the Thread::main() method
    //!   returns (see the private method Thread::mainWrapper()).
    //! - Otherwise, on Linux and macOS (pthread-based), the deletion occurs when the pthread is deleted.
    //!   It has been noted that it is sometimes too late, depending on what the destructors are doing.
    //!   For instance, detaching a thread from the Java Virtual Machine (JVM) when a native thread was
    //!   used in a Java Native Interface (JNI) method is too late. The JVM will wait indefinitely for
    //!   the native thread to terminate.
    //! - Otherwise, on Windows, the deletions occur only when TSDuck is used as a DLL because DllMain() is
    //!   the only known way to execute some code on thread exit. When TSDuck is used as a static library,
    //!   thread local objects are never deallocated. If you know a solution to this problem, please submit
    //!   a fix through a pull request.
    //!
    //! In other words, ThreadLocalObjects is really safe only on threads which were created by ts::Thread.
    //!
    class TSDUCKDLL ThreadLocalObjects
    {
        TS_DECLARE_SINGLETON(ThreadLocalObjects);
    public:
        //!
        //! Set the value of a thread local object.
        //! @param [in] name Name of the object.
        //! @param [in] obj A smart pointer to the object with that name in the current thread.
        //!
        void setLocalObject(const UString& name, const ObjectPtr& obj);

        //!
        //! Get the value of a thread local object.
        //! @param [in] name Name of the object.
        //! @return A smart pointer to the object with that name in the current thread or the null pointer if there is none.
        //!
        ObjectPtr getLocalObject(const UString& name);

        //!
        //! Delete all local objects in the current thread.
        //!
        void deleteLocalObjects();

    private:
        // Each thread has a local structure like this in its thread-specific storage.
        // Since there is an instance per thread, there is no need to synchronize access to each structure.
        class ThreadLocalRepository
        {
        public:
            ThreadLocalRepository();
            std::map<UString, ObjectPtr> objects;
        };

        // Get or create the ThreadLocalRepository of the current thread. Never null.
        ThreadLocalRepository* getCurrentRepo();

        // The thread local storage index/key pointing to the ThreadLocalRepository instance of the thread.
#if defined(TS_WINDOWS)
        ::DWORD _tls_index;
#else
        pthread_key_t _key;
        static void DeleteThreadLocalRepository(void*);
#endif
    };
}
