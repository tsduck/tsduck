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
//! @code
//! // File: MySingle.cpp
//! #include "MySingle.h"
//!
//! TS_DEFINE_SINGLETON(foo::MySingle);
//! foo::MySingle::MySingle() : ... { ... }
//! ....
//! @endcode
//!
//! The class becomes a singleton. The default constructor is private.
//! Use static Instance() method to get the instance of the singleton.
//! @see TS_DEFINE_SINGLETON()
//! @hideinitializer
//!
#define TS_DECLARE_SINGLETON(classname)          \
        TS_NOCOPY(classname);                    \
        TS_DECLARE_SINGLETON_BASE(, classname);  \
    private:                                     \
        classname() /* default constructor, to be implemented by user */

//!
//! Singleton class definition.
//!
//! The macro TS_DEFINE_SINGLETON must be used in the implementation of a singleton class.
//! @param fullclassname Fully qualified name of the singleton class.
//! @see TS_DECLARE_SINGLETON()
//! @hideinitializer
//!
#define TS_DEFINE_SINGLETON(fullclassname) \
    TS_DEFINE_SINGLETON_ATEXIT(fullclassname, ts::atexit)

//!
//! Singleton class definition, with specific atexit() function.
//!
//! @param fullclassname Fully qualified name of the singleton class.
//! @param atexitfunction The atexit() function to use to register the destructor.
//! Typical values are ts::atexit, std::atexit, OPENSSL_atexit.
//! @see TS_DEFINE_SINGLETON()
//! @hideinitializer
//!
#define TS_DEFINE_SINGLETON_ATEXIT(fullclassname, atexitfunction) \
    TS_DEFINE_SINGLETON_BASE(fullclassname, , fullclassname, , atexitfunction)

//!
//! Global object declaration.
//!
//! The macros TS_DECLARE_GLOBAL and TS_DEFINE_GLOBAL are used to declare a global object of a known type.
//! This kind of object cannot be simply statically declared with something like "extern const Foo obj"
//! if it requires a dynamic constructor. Because of the undefined initialization order in the list of
//! modules, the object could be used before being initialized. These macros ensure that the object is
//! dynamically created the first time it is used (thread-safe).
//!
//! The actual object is wrapped into a singleton and can be accessed using the operators "*" and "->".
//!
//! @param constness Either "const" or empty.
//! @param objtype Fully qualified type name of the actual object. This object is wrapped into a singleton.
//! @param objname Name of the global object. Use operators "*" and "->" to access the @a objtype object.
//!
//! Example code:
//! @code
//! // File: MyName.h
//! namespace foo {
//!     TS_DECLARE_GLOBAL(const, std::string, MyName);
//! @endcode
//!
//! @code
//! // File: MyName.cpp
//! #include "MyName.h"
//!
//! TS_DEFINE_GLOBAL(const, foo::MyName, ("the name string"));
//! void test() { std::cout << NyName->length() << ": " << *MyName << std::endl; }
//! @endcode
//! @see TS_DEFINE_GLOBAL()
//! @hideinitializer
//!
#define TS_DECLARE_GLOBAL(constness, objtype, objname)                 \
    TS_DECLARE_GLOBAL_WRAPPER(TSDUCKDLL, constness, objtype, objname); \
    TSDUCKDLL extern const objname##Wrapper objname

//!
//! Global object definition.
//!
//! @param constness Either "const" or empty.
//! @param objtype Fully qualified type name of the actual object. This object is wrapped into a singleton.
//! @param fullobjname Fully qualified name of the global object, including namespace if necessary.
//! @param initializer The initializer list, enclosed within parentheses,
//! of the constructor of the static instance. Use "()" if there is no initializer.
//! @see TS_DECLARE_GLOBAL()
//! @hideinitializer
//!
#define TS_DEFINE_GLOBAL(constness, objtype, fullobjname, initializer) \
    TS_DEFINE_GLOBAL_ATEXIT(constness, objtype, fullobjname, initializer, ts::atexit)

//!
//! Global object definition, with specific atexit() function.
//!
//! @param constness Either "const" or empty.
//! @param objtype Fully qualified type name of the actual object. This object is wrapped into a singleton.
//! @param fullobjname Fully qualified name of the global object, including namespace if necessary.
//! @param initializer The initializer list, enclosed within parentheses,
//! of the constructor of the static instance. Use "()" if there is no initializer.
//! @param atexitfunction The atexit() function to use to register the destructor.
//! Typical values are ts::atexit, std::atexit, OPENSSL_atexit.
//! @see TS_DECLARE_GLOBAL()
//! @hideinitializer
//!
#define TS_DEFINE_GLOBAL_ATEXIT(constness, objtype, fullobjname, initializer, atexitfunction) \
    TS_DEFINE_SINGLETON_BASE(fullobjname##Wrapper, constness, objtype, initializer, atexitfunction); \
    const fullobjname##Wrapper fullobjname

//!
//! Local declaration of a static object regardless of modules initialization order.
//!
//! This macro is a variant of the TS_DECLARE_GLOBAL / TS_DEFINE_GLOBAL pattern.//!
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
//! std::cout << "Foo1: " << *Foo1 << std::endl;
//! std::cout << "Foo2: " << *Foo2 << std::endl;
//! @endcode
//!
//! @param constness Either @a TS_VARIABLE or @a TS_CONST (if the object is constant and fully defined by its initializer).
//! @param objtype Fully qualified type name of the actual object. This object is wrapped into a singleton.
//! @param objname Name of the static object. Use operators "*" and "->" to access the @a objtype object.
//! @param initializer The initializer list, enclosed within parentheses,
//! of the constructor of the static instance. Use "()" if there is no initializer.
//! @see TS_DECLARE_GLOBAL()
//! @hideinitializer
//!
#define TS_STATIC_INSTANCE(constness, objtype, objname, initializer) \
    TS_STATIC_INSTANCE_ATEXIT(constness, objtype, objname, initializer, ts::atexit)

//!
//! Local declaration of a static object regardless of modules initialization order, with specific atexit() function.
//!
//! @param constness Either @a TS_VARIABLE or @a TS_CONST (if the object is constant and fully defined by its initializer).
//! @param objtype Fully qualified type name of the actual object. This object is wrapped into a singleton.
//! @param objname Name of the static object. Use operators "*" and "->" to access the @a objtype object.
//! @param initializer The initializer list, enclosed within parentheses,
//! of the constructor of the static instance. Use "()" if there is no initializer.
//! @param atexitfunction The atexit() function to use to register the destructor.
//! Typical values are ts::atexit, std::atexit, OPENSSL_atexit.
//! @see TS_STATIC_INSTANCE()
//! @hideinitializer
//!
#define TS_STATIC_INSTANCE_ATEXIT(constness, objtype, objname, initializer, atexitfunction) \
    namespace {                                                    \
        TS_DECLARE_GLOBAL_WRAPPER(, constness, objtype, objname);  \
    }                                                              \
    TS_DEFINE_GLOBAL_ATEXIT(constness, objtype, objname, initializer, atexitfunction)


//----------------------------------------------------------------------------
// Support macros (not publicly documented).
//----------------------------------------------------------------------------

//! Support macro for TS_DECLARE_SINGLETON, do not use directly.
#define TS_DECLARE_SINGLETON_BASE(constness, objtype)               \
    public:                                                         \
        /** Get the instance of the singleton of this class. */     \
        /** @return The instance of the singleton of this class. */ \
        static constness objtype& Instance()                        \
        {                                                           \
            if (_instance == nullptr) {                             \
                InitInstance();                                     \
            }                                                       \
            return *_instance;                                      \
        }                                                           \
    private:                                                        \
        static constness objtype* volatile _instance;               \
        static std::once_flag _once_flag;                           \
        static void InitInstance();                                 \
        static void CleanupSingleton()

//! Support macro for TS_DEFINE_SINGLETON, do not use directly.
#define TS_DEFINE_SINGLETON_BASE(fullclassname, constness, objtype, initializer, atexitfunction) \
    void fullclassname::InitInstance()                              \
    {                                                               \
        std::call_once(_once_flag, []() {                           \
            _instance = new objtype initializer;                    \
            atexitfunction(fullclassname::CleanupSingleton);        \
        });                                                         \
    }                                                               \
    void fullclassname::CleanupSingleton()                          \
    {                                                               \
        if (_instance != nullptr) {                                 \
            delete _instance;                                       \
            _instance = nullptr;                                    \
        }                                                           \
    }                                                               \
    constness objtype* volatile fullclassname::_instance = nullptr; \
    std::once_flag fullclassname::_once_flag {}

#if defined(DOXYGEN)

//! Support macro for TS_DECLARE_GLOBAL, do not use directly.
#define TS_DECLARE_GLOBAL_WRAPPER(constness, objtype, objname)

#else

#define TS_DECLARE_GLOBAL_WRAPPER(exportness, constness, objtype, objname) \
    /** @cond nodoxygen */                                                 \
    class exportness objname##Wrapper                                      \
    {                                                                      \
        TS_NOCOPY(objname##Wrapper);                                       \
        TS_DECLARE_SINGLETON_BASE(constness, objtype);                     \
    public:                                                                \
        objname##Wrapper() = default;                                      \
        TS_PUSH_WARNING()                                                  \
        TS_LLVM_NOWARNING(unused-member-function)                          \
        constness objtype* operator->() const { return &Instance(); }      \
        constness objtype& operator*() const { return Instance(); }        \
        TS_POP_WARNING()                                                   \
    } /** @cond nodoxygen */

#endif
