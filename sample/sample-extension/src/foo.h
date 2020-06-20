// Base definitions for the sample TSDuck "foo" extension.

#pragma once
#include "tsduck.h"

// Attribute to declare a class or function from tslibext_foo.dll on Windows.
// When building tslibext_foo.dll on Windows, define _FOODLL_IMPL in the project options.
// When building a project which references tslibext_foo.dll, define _FOODLL_USE.
// All API's  inside tslibext_foo.dll shall be prefixed by FOODLL in headers.
// This prefix exports the API when building the DLL and imports the API when used in an application.

#if defined(TS_WINDOWS) && defined(_FOODLL_IMPL)
    #define FOODLL __declspec(dllexport)
#elif defined(TS_WINDOWS) && defined(_FOODLL_USE)
    #define FOODLL __declspec(dllimport)
#else
    #define FOODLL
#endif

namespace foo {

    // Specific table identifiers values for "foo".
    enum : ts::TID {
        TID_FOOT = 0xF0,   // Table id for Foo Table (FOOT).
    };

    // Specific descriptor tags values for "foo".
    enum : ts::DID {
        DID_FOO = 0xE8,    // Descriptor id for foo_descriptor.
    };

    // Specific CA_System_Id values for "foo".
    enum : uint16_t {
        CASID_FOO_MIN = 0xF001,  // Minimum CAS Id value for FooCAS.
        CASID_FOO_MAX = 0xF008,  // Maximum CAS Id value for FooCAS.
    };

    // Digital TV standards to which the "foo" signalization belongs (no defined standard).
    constexpr ts::Standards STD = ts::Standards::NONE;
}
