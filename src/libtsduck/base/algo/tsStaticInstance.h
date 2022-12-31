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
//!  @ingroup cpp
//!  Declare the initialization-order-safe macros for static object instances.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMemory.h"

//!
//! @hideinitializer
//! Declaration of a static object regardless of modules initialization order.
//!
//! This macro expands to the @e declaration part of the static instance.
//! See @link TS_STATIC_INSTANCE @endlink for more details on the usage of static instances.
//!
//! The macro @link TS_STATIC_INSTANCE_DECLARATION @endlink is used in cunjunction with its
//! counterpart @link TS_STATIC_INSTANCE_DEFINITION @endlink. They are used when the static
//! instance must be publicly accessible or declared in a specific namespace.
//!
//! Most of the time, a static instance is used privately inside a module and
//! declared in the anonymous namespace. In this general case, the macro
//! @link TS_STATIC_INSTANCE @endlink is a simpler alternative.
//!
//! In the unusual case where you need to make a static instance publicly
//! available, use the macro @link TS_STATIC_INSTANCE_DECLARATION @endlink in the header
//! file and the macro @link TS_STATIC_INSTANCE_DEFINITION @endlink in the C++ body file
//! as illustrated below.
//!
//! In the header file (.h) of the module, we declare a static instance of
//! @c std::string in the namespace @c ts::foo.
//! @code
//! namespace ts {
//!     namespace foo {
//!         TS_STATIC_INSTANCE_DECLARATION(std::string, TSDUCKDLL, Bar);
//!     }
//! }
//! @endcode
//!
//! In the definition body file (.cpp) of the module:
//! @code
//! TS_STATIC_INSTANCE_DEFINITION(std::string, ("initial value"), ts::foo::Bar, Bar);
//! @endcode
//!
//! In application code, we use the static instance of string as follow:
//! @code
//! std::cout << "static string instance: " << ts::foo::Bar::Instance() << std::endl;
//! @endcode
//!
//! @param ObjectClass The name of the class of the static object.
//! This must be an existing class.
//! @param ClassAttributes Attributes of StaticInstanceClass, typically TSDUCKDLL or empty.
//! @param StaticInstanceClass The name of the new class which encapsulates
//! the static instance. This class is declared by the expansion of the macro.
//!
//! @see TS_STATIC_INSTANCE for more details on the usage of static instances.
//! @see TS_STATIC_INSTANCE_DEFINITION for the @e definition part.
//!
#define TS_STATIC_INSTANCE_DECLARATION(ObjectClass, ClassAttributes, StaticInstanceClass) \
    class ClassAttributes StaticInstanceClass                                \
    {                                                                        \
        TS_NOCOPY(StaticInstanceClass);                                      \
    public:                                                                  \
        /** Public static method to access the static instance. */           \
        /** @return A reference to the static instance. */                   \
        static ObjectClass& Instance();                                      \
    private:                                                                 \
        /* The actual object */                                              \
        ObjectClass _object;                                                 \
        /* The constructor of the static instance initializes */             \
        /* the actual object. */                                             \
        StaticInstanceClass();                                               \
        /* Pointer to the actual static instance. */                         \
        static StaticInstanceClass* volatile _instance;                      \
        /* Inner private class responsible for creation and */               \
        /* destruction of the static instance. */                            \
        class Controller                                                     \
        {                                                                    \
            TS_NOCOPY(Controller);                                           \
        public:                                                              \
            /* The controller constructor forces the creation of */          \
            /* the static instance if not already created by an */           \
            /* earlier method invocation. */                                 \
            Controller();                                                    \
            /* The controller destructor forces the deletion of the */       \
            /* static instance. */                                           \
            ~Controller();                                                   \
        };                                                                   \
        /* Only one instance of the controller. */                           \
        static Controller _controller;                                       \
    }

//!
//! @hideinitializer
//! Definition of a static object regardless of modules initialization order.
//!
//! This macro expands to the @e definition part of the static instance.
//!
//! @param ObjectClass The name of the class of the static object.
//! This must be an existing class.
//! @param ObjectArgs The initializer list, enclosed within parentheses,
//! of the constructor of the static instance. Use () if there is no initializer.
//! @param StaticInstancePath The full name, including namespaces if any, of
//! the new class which encapsulates the static instance. This class is
//! defined by the expansion of the macro.
//! @param StaticInstanceClass The simple name, without namespace, of the new
//! class which encapsulates the static instance. This is the class name part
//! of @a StaticInstancePath. If there is no namespace, @a StaticInstancePath
//! and @a StaticInstanceClass are identical.
//!
//! @see TS_STATIC_INSTANCE for more details on the usage of static instances.
//! @see TS_STATIC_INSTANCE_DECLARATION for the @e declaration part and usage examples.
//!
#define TS_STATIC_INSTANCE_DEFINITION(ObjectClass, ObjectArgs, StaticInstancePath, StaticInstanceClass) \
    /* The constructor of the static instance initializes */              \
    /* the actual object. */                                              \
    StaticInstancePath::StaticInstanceClass() :                           \
        _object ObjectArgs                                                \
    {                                                                     \
    }                                                                     \
    /* Public static method to access the instance. */                    \
    ObjectClass& StaticInstancePath::Instance()                           \
    {                                                                     \
        if (_instance == nullptr) {                                       \
            /* No thread synchronization here. */                         \
            StaticInstanceClass* volatile tmp = new StaticInstanceClass;  \
            ts::MemoryBarrier();                                          \
            _instance = tmp;                                              \
        }                                                                 \
        return _instance->_object;                                        \
    }                                                                     \
    /* The controller constructor forces the creation of the static */    \
    /* instance if not already created by an earlier method. */           \
    StaticInstancePath::Controller::Controller()                          \
    {                                                                     \
        StaticInstanceClass::Instance();                                  \
    }                                                                     \
    /* The controller destructor forces the deletion of the */            \
    /* static instance. */                                                \
    StaticInstancePath::Controller::~Controller()                         \
    {                                                                     \
        if (StaticInstanceClass::_instance != nullptr) {                  \
            delete StaticInstanceClass::_instance;                        \
            StaticInstanceClass::_instance = nullptr;                     \
        }                                                                 \
    }                                                                     \
    /* Pointer to the actual instance is statically initialized to */     \
    /* zero BEFORE invoking the initialization of the module. */          \
    StaticInstancePath* volatile StaticInstancePath::_instance = nullptr; \
    /* The controller is initialized ("constructed") DURING the */        \
    /* initialization of the module. */                                   \
    StaticInstancePath::Controller StaticInstancePath::_controller

//!
//! @hideinitializer
//! Local declaration of a static object regardless of modules initialization order.
//!
//! The static initialization order nightmare
//! -----------------------------------------
//!
//! In C++, each module needs to be initialized. For each module, the compiler
//! potentially generates an initialization routine. This initialization routine
//! is responsible for constructing the static objects in the module. The
//! initialization routines of all modules are executed @e before entering
//! the @c main() function of the application.
//!
//! A static object is defined inside one module. When the static object
//! has an elementary type or a pointer type and is initialized by a static
//! value (i.e. a compile-time, link-time or load-time expression), the initial
//! value of the object is ready @e before the application starts, before the
//! execution of all initialization routines of all modules. However, when the
//! static object has a class type, its initialization is the invocation of a
//! constructor. This constructor is invoked by the initialization routine of
//! the module.
//!
//! When a static object is used by the application, after entering the
//! @c main() function of the application, it is guaranteed that all
//! initialization routines have completed and, consequently, all static
//! objects are fully constructed.
//!
//! However, there are cases where a static object is used by the initialization
//! routine of another module. This is typically the case when a static object
//! references another static object in its constructor. Usually, this is not
//! as obvious as it seems. If the constructor of a static object in module A invokes
//! a static method from another module B, you do not know if this static method
//! does or does not use a static object in the module B. In this case, for the
//! application to work properly, it is necessary that the module B is initialized
//! before the module A.
//!
//! The problem is that there is no way in the C++ language to determine the
//! execution order of the initialization routines of the various modules.
//! The order is undefined and implementation dependent. In the previous example,
//! it is possible that the initialization routine of the module B is executed
//! before the initialization routine of the module A on a platform 1. On this
//! platform, the application works correctly. But a platform 2 is also allowed
//! to invoke the initialization routines in the reverse order. Consequently,
//! on this platform, the application will most certainly crash.
//!
//! To avoid this, you may simply ban all static objects in your coding rules.
//! However, if you find this too restrictive, alternate solutions must be
//! found.
//!
//! The obvious solution is the singleton pattern (see the file @link tsSingletonManager.h
//! @endlink). In this pattern, no object is constructed at initialization.
//! A pointer is statically initialized to zero, which is guaranteed to work.
//! The first time the object is requested, it is allocated, stored in this
//! static pointer and reused every other time. However, for this to work in a
//! multithreaded environment, the test-and-creation sequence must be protected
//! by a mutex in the same module. And this mutex must be ... statically initialized !
//!
//! So, there is still a requirement for a solution for the mutex, at least.
//! And this solution is intrinsicly thread-unsafe. So, the solution must
//! reduce the race condition risk to the minimum, typically by making the
//! best effort to create the critical static objects as soon as possible,
//! hopefully before the creation of the first thread.
//!
//! This is the purpose of the macros @link TS_STATIC_INSTANCE_DECLARATION @endlink
//! and @link TS_STATIC_INSTANCE_DEFINITION @endlink with the associated "shortcut"
//! macro @link TS_STATIC_INSTANCE @endlink.
//!
//! Using the TS_STATIC_INSTANCE macros
//! -----------------------------------
//!
//! You need a static object. The type for this object is the parameter @a ObjectClass
//! in the various TS_STATIC_INSTANCE macros.
//!
//! The macros create an encapsulating local class for each static object.
//! The name of this local class is the parameter @a StaticInstanceClass in the macros.
//! This class has one public static method named @c Instance() which returns
//! a reference to the static object. The static object is allocated during the
//! initialization of the application, no later than during the initialization of
//! the module where it is defined but possibly earlier if the static object is
//! referenced before the initialization of the module where it is defined.
//!
//! Thus, the initialization order problem is avoided. But, on the downside, the
//! unique construction of the object is not thread-safe. Consequently, the object
//! shall not be accessed by concurrent threads during the initialization phase of
//! the application.
//!
//! The initializers of the object are grouped into the parameter @a ObjectArgs of the macros.
//! This is the argument list, including the pair of enclosing parentheses, of the constructor
//! of the object.
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
#define TS_STATIC_INSTANCE(ObjectClass, ObjectArgs, StaticInstanceClass)    \
    namespace {                                                             \
        TS_STATIC_INSTANCE_DECLARATION(ObjectClass, , StaticInstanceClass); \
        TS_STATIC_INSTANCE_DEFINITION(ObjectClass, ObjectArgs, StaticInstanceClass, StaticInstanceClass); \
    }
