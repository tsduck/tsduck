//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Helper for singleton definition
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Register a function to execute when the application exits.
    //!
    //! This is a re-implementation of std::atexit() with unlimited number of registered
    //! functions (std::atexit() can only guarantee 32 entries)
    //! The functions will be called in reverse order: if A was registered before B,
    //! then the call to B is made before the call to A.
    //!
    //! @param [in] func The function to call when the program terminates.
    //! @return Always 0. This returned value makes ts::atexit() compatible with std::atexit().
    //! With std::atexit(), 0 is a success value. For OpenSSL users, note that OPENSSL_atexit()
    //! return 1 on success and 0 on error, despite an identical profile. So, if you need a
    //! flexible call, you'd better ignore the result.
    //!
    TSDUCKDLL int atexit(void (*func)());

    //!
    //! Register a function to execute when the application exits (with a parameter).
    //!
    //! This is a re-implementation of std::atexit() with:
    //! - Unlimited number of registered functions (std::atexit() can only guarantee 32 entries)
    //! - Pass a parameter to the function.
    //! The functions will be called in reverse order: if A was registered before B,
    //! then the call to B is made before the call to A.
    //!
    //! @param [in] func The function to call when the program terminates.
    //! @param [in] param The parameter to pass to @a func.
    //!
    TSDUCKDLL void atexit(void (*func)(void*), void* param = nullptr);
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
        static classname& Instance()                                \
        {                                                           \
            if (_instance == nullptr) {                             \
                InitInstance();                                     \
            }                                                       \
            return *_instance;                                      \
        }                                                           \
    private:                                                        \
        static classname* volatile _instance;                       \
        static std::once_flag _once_flag;                           \
        classname(); /* default constructor */                      \
        static void InitInstance();                                 \
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
#define TS_DEFINE_SINGLETON(fullclassname) TS_DEFINE_SINGLETON_ATEXIT(fullclassname, ts::atexit)

//!
//! @hideinitializer
//! Singleton class definition, with specific atexit() function.
//! @param fullclassname Fully qualified name of the singleton class.
//! @param atexitfunction The atexit() function to use to register the destructor.
//! Typical values are ts::atexit, std::atexit, OPENSSL_atexit.
//! @see TS_DEFINE_SINGLETON()
//!
#define TS_DEFINE_SINGLETON_ATEXIT(fullclassname, atexitfunction) \
    void fullclassname::InitInstance()                            \
    {                                                             \
        std::call_once(_once_flag, []() {                         \
            _instance = new fullclassname;                        \
            atexitfunction(fullclassname::CleanupSingleton);      \
        });                                                       \
    }                                                             \
    void fullclassname::CleanupSingleton()                        \
    {                                                             \
        if (_instance != nullptr) {                               \
            delete _instance;                                     \
            _instance = nullptr;                                  \
        }                                                         \
    }                                                             \
    fullclassname* volatile fullclassname::_instance = nullptr;   \
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
#define TS_STATIC_INSTANCE(ObjectClass, ObjectArgs, StaticInstanceClass) TS_STATIC_INSTANCE_ATEXIT(ObjectClass, ObjectArgs, StaticInstanceClass, ts::atexit)

//!
//! @hideinitializer
//! Local declaration of a static object regardless of modules initialization order, with specific atexit() function.
//! @param ObjectClass The name of the class of the static object.
//! This must be an existing class.
//! @param ObjectArgs The initializer list, enclosed within parentheses,
//! of the constructor of the static instance. Use () if there is no initializer.
//! @param StaticInstanceClass The name of the new class which encapsulates
//! the static instance. This class is declared by the expansion of the macro.
//! @param AtExitFunction The atexit() function to use to register the destructor.
//! Typical values are ts::atexit, std::atexit, OPENSSL_atexit.
//! @see TS_STATIC_INSTANCE()
//!
#define TS_STATIC_INSTANCE_ATEXIT(ObjectClass, ObjectArgs, StaticInstanceClass, AtExitFunction) \
    namespace {                                                            \
        class StaticInstanceClass                                          \
        {                                                                  \
            TS_NOCOPY(StaticInstanceClass);                                \
        public:                                                            \
            /** Public static method to access the static instance. */     \
            /** @return A reference to the static instance. */             \
            static ObjectClass& Instance();                                \
        private:                                                           \
            static ObjectClass* volatile _instance;                        \
            static std::once_flag _once_flag;                              \
            static void CleanupSingleton();                                \
        };                                                                 \
        ObjectClass& StaticInstanceClass::Instance()                       \
        {                                                                  \
            if (_instance == nullptr) {                                    \
                std::call_once(_once_flag, []() {                          \
                    _instance = new ObjectClass ObjectArgs;                \
                    AtExitFunction(StaticInstanceClass::CleanupSingleton); \
                });                                                        \
            }                                                              \
            return *_instance;                                             \
        }                                                                  \
        void StaticInstanceClass::CleanupSingleton()                       \
        {                                                                  \
            if (_instance != nullptr) {                                    \
                delete _instance;                                          \
                _instance = nullptr;                                       \
            }                                                              \
        }                                                                  \
        ObjectClass* volatile StaticInstanceClass::_instance = nullptr;    \
        std::once_flag StaticInstanceClass::_once_flag {};                 \
    }                                                                      \
    /** @cond nodoxygen */                                                 \
    using TS_UNIQUE_NAME(for_trailing_semicolon) = int                     \
    /** @endcond */
