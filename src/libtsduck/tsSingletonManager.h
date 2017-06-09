//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Helper for singleton definition
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsMutex.h"
#include "tsGuard.h"

namespace ts {
    //!
    //! Helper for singleton definition
    //!
    //! The class SingletonManager is a singleton itself.
    //! It helps the creation of all other singletons.
    //! Never use it directly.
    //! Used only through the macros tsDeclareSingleton() and tsDefineSingleton().
    //!
    class TSDUCKDLL SingletonManager
    {
    public:
        //!
        //! Get the instance of the singleton of this class.
        //! @return The instance of the singleton of this class.
        //!
        static SingletonManager* Instance();
        //!
        //! A global mutex used in the creation of singletons.
        //!
        Mutex mutex;
    private:
        static SingletonManager* volatile _instance;
        SingletonManager() : mutex() {}
    };
}

//!
//! Singleton class declaration.
//!
//! The macro tsDeclareSingleton must be used inside the singleton class declaration.
//! @param classname Name of the singleton class, without namespace or outer class name.
//!
//! Example code:
//! @code
//! // File: MySingle.h
//! namespace foo {
//!     class MySingle {
//!        tsDeclareSingleton(MySingle);
//!        ....
//! @endcode
//!
//! The class becomes a singleton, under control of the SingletonManager.
//! Use static Instance() method to get the instance of the singleton.
//! @see tsDefineSingleton()
//! @hideinitializer
//!
#define tsDeclareSingleton(classname)                               \
    public:                                                         \
        /** Get the instance of the singleton of this class. */     \
        /** @return The instance of the singleton of this class. */ \
        static classname* Instance();                               \
    private:                                                        \
        static classname* volatile _instance;                       \
        static void CleanupSingleton();                             \
        classname();                                                \
        classname(const classname&) = delete;                       \
        classname& operator=(const classname&) = delete

//!
//! @hideinitializer
//! Singleton class definition.
//!
//! The macro tsDefineSingleton must be used in the implementation of a singleton class.
//! @param fullclassname Fully qualified name of the singleton class.
//!
//! Example code:
//! @code
//! // File: MySingle.cpp
//! #include "MySingle.h"
//!
//! tsDefineSingleton(foo::MySingle);
//!
//! foo::MySingle::MySingle() : ... { ... }
//! ....
//! @endcode
//! @see tsDeclareSingleton()
//!
#define tsDefineSingleton(fullclassname)                             \
    fullclassname* fullclassname::Instance()                         \
    {                                                                \
        if (_instance == 0) {                                        \
            ts::Guard lock(ts::SingletonManager::Instance()->mutex); \
            if (_instance == 0) {                                    \
                _instance = new fullclassname();                     \
                ::atexit(fullclassname::CleanupSingleton);           \
            }                                                        \
        }                                                            \
        return _instance;                                            \
    }                                                                \
    void fullclassname::CleanupSingleton()                           \
    {                                                                \
        if (_instance != 0) {                                        \
            delete _instance;                                        \
            _instance = 0;                                           \
        }                                                            \
    }                                                                \
    fullclassname* volatile fullclassname::_instance = 0
