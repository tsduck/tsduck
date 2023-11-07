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
//! The class becomes a singleton. The default contructor is private.
//! Use static Instance() method to get the instance of the singleton.
//! @see TS_DEFINE_SINGLETON()
//! @hideinitializer
//!
#define TS_DECLARE_SINGLETON(classname)                             \
        TS_NOCOPY(classname);                                       \
    public:                                                         \
        /** Get the instance of the singleton of this class. */     \
        /** @return The instance of the singleton of this class. */ \
        static classname& Instance();                               \
    private:                                                        \
        static classname* volatile _instance;                       \
        static std::once_flag _once_flag;                           \
        classname(); /* default constructor */                      \
        static void CleanupSingleton()

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
#define TS_DEFINE_SINGLETON(fullclassname)                       \
    fullclassname& fullclassname::Instance()                     \
    {                                                            \
        if (_instance == nullptr) {                              \
            std::call_once(_once_flag, []() {                    \
                _instance = new fullclassname;                   \
                std::atexit(fullclassname::CleanupSingleton);    \
          });                                                    \
        }                                                        \
        return *_instance;                                       \
    }                                                            \
    void fullclassname::CleanupSingleton()                       \
    {                                                            \
        if (_instance != nullptr) {                              \
            delete _instance;                                    \
            _instance = nullptr;                                 \
        }                                                        \
    }                                                            \
    fullclassname* volatile fullclassname::_instance = nullptr;  \
    std::once_flag fullclassname::_once_flag {}

//!
//! @hideinitializer
//! Local declaration of a static object regardless of modules initialization order.
//!
//! This macro is a variant of the singleton pattern. Instead of being part of a
//! user-defined class, this macro instantiates an object of an existing class.
//! The constructor arguments are passed to the macro.
//!
//! The macro @link TS_STATIC_INSTANCE @endlink creates a static object inside the
//! anonymous namespace of the module where it is expanded.
//!
//! Example using the @c std::string type: Two static objects @c Foo1 and @c Foo2
//! are created. Remember that @c Foo1 and @c Foo2 are not the name of objects but
//! the names of the encapsulating classes of the objects. The first static object
//! is initialized by the default constructor and the parameter @a ObjectArgs of
//! the macro is simply the empty argument list <code>()</code>. The second static object is
//! built by the constructor <code>std::string (size_t, char)</code> and the parameter
//! @a ObjectArgs is a list of two parameters <code>(4, '=')</code>.
//!
//! @code
//! TS_STATIC_INSTANCE(std::string, (), Foo1)
//! TS_STATIC_INSTANCE(std::string, (4, '='), Foo2)
//! ....
//! std::cout << "Foo1: " << Foo1::Instance() << std::endl;
//! std::cout << "Foo2: " << Foo2::Instance() << std::endl;
//! @endcode
//!
//! @param ObjectClass The name of the class of the static object.
//! This must be an existing class.
//! @param ObjectArgs The initializer list, enclosed within parentheses,
//! of the constructor of the static instance. Use () if there is no initializer.
//! @param StaticInstanceClass The name of the new class which encapsulates
//! the static instance. This class is declared by the expansion of the macro.
//!
#define TS_STATIC_INSTANCE(ObjectClass, ObjectArgs, StaticInstanceClass) \
    namespace {                                                          \
        class StaticInstanceClass                                        \
        {                                                                \
            TS_NOCOPY(StaticInstanceClass);                              \
        public:                                                          \
            /** Public static method to access the static instance. */   \
            /** @return A reference to the static instance. */           \
            static ObjectClass& Instance();                              \
        private:                                                         \
            static ObjectClass* volatile _instance;                      \
            static std::once_flag _once_flag;                            \
            static void CleanupSingleton();                              \
        };                                                               \
        ObjectClass& StaticInstanceClass::Instance()                     \
        {                                                                \
            if (_instance == nullptr) {                                  \
                std::call_once(_once_flag, []() {                        \
                    _instance = new ObjectClass ObjectArgs;              \
                    std::atexit(StaticInstanceClass::CleanupSingleton);  \
                });                                                      \
            }                                                            \
            return *_instance;                                           \
        }                                                                \
        void StaticInstanceClass::CleanupSingleton()                     \
        {                                                                \
            if (_instance != nullptr) {                                  \
                delete _instance;                                        \
                _instance = nullptr;                                     \
            }                                                            \
        }                                                                \
        ObjectClass* volatile StaticInstanceClass::_instance = nullptr;  \
        std::once_flag StaticInstanceClass::_once_flag {};               \
    }                                                                    \
    typedef int TS_UNIQUE_NAME(for_trailing_semicolon)
