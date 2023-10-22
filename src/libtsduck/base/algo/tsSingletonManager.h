//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
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
#include "tsGuardMutex.h"

namespace ts {
    //!
    //! Helper for singleton definition.
    //! @ingroup cpp
    //!
    //! The class SingletonManager is a singleton itself.
    //! It helps the creation of all other singletons.
    //! Never use it directly.
    //! Used only through the macros TS_DECLARE_SINGLETON() and TS_DEFINE_SINGLETON().
    //!
    class TSDUCKDLL SingletonManager
    {
        TS_NOCOPY(SingletonManager);
    public:
        //!
        //! Get the instance of the singleton of this class.
        //! @return The instance of the singleton of this class.
        //!
        static SingletonManager* Instance();
        //!
        //! A global mutex used in the creation of singletons.
        //!
        Mutex mutex {};
    private:
        static SingletonManager* volatile _instance;
        SingletonManager() = default;
    };
}

//!
//! Singleton class declaration.
//!
//! The macro TS_DECLARE_SINGLETON must be used inside the singleton class declaration.
//! @param classname Name of the singleton class, without namespace or outer class name.
//!
//! Example code:
//! @code
//! // File: MySingle.h
//! namespace foo {
//!     class MySingle {
//!        TS_DECLARE_SINGLETON(MySingle);
//!        ....
//! @endcode
//!
//! The class becomes a singleton, under control of the SingletonManager.
//! Use static Instance() method to get the instance of the singleton.
//! @see TS_DEFINE_SINGLETON()
//! @hideinitializer
//!
#define TS_DECLARE_SINGLETON(classname)                             \
    public:                                                         \
        /** Get the instance of the singleton of this class. */     \
        /** @return The instance of the singleton of this class. */ \
        static classname* Instance();                               \
    private:                                                        \
        static classname* volatile _instance;                       \
        static void CleanupSingleton();                             \
        classname();                                                \
        TS_NOCOPY(classname)

//!
//! @hideinitializer
//! Singleton class definition.
//!
//! The macro TS_DEFINE_SINGLETON must be used in the implementation of a singleton class.
//! @param fullclassname Fully qualified name of the singleton class.
//!
//! Example code:
//! @code
//! // File: MySingle.cpp
//! #include "MySingle.h"
//!
//! TS_DEFINE_SINGLETON(foo::MySingle);
//!
//! foo::MySingle::MySingle() : ... { ... }
//! ....
//! @endcode
//! @see TS_DECLARE_SINGLETON()
//!
#define TS_DEFINE_SINGLETON(fullclassname)                                \
    fullclassname* fullclassname::Instance()                              \
    {                                                                     \
        if (_instance == nullptr) {                                       \
            ts::GuardMutex lock(ts::SingletonManager::Instance()->mutex); \
            if (_instance == nullptr) {                                   \
                _instance = new fullclassname();                          \
                ::atexit(fullclassname::CleanupSingleton);                \
            }                                                             \
        }                                                                 \
        return _instance;                                                 \
    }                                                                     \
    void fullclassname::CleanupSingleton()                                \
    {                                                                     \
        if (_instance != nullptr) {                                       \
            delete _instance;                                             \
            _instance = nullptr;                                          \
        }                                                                 \
    }                                                                     \
    fullclassname* volatile fullclassname::_instance = nullptr
