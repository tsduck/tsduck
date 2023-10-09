//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declaration of class DumpCharset.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCharset.h"

TS_PUSH_WARNING()
TS_GCC_NOWARNING(ctor-dtor-privacy) // private constructor here

namespace ts {
    //!
    //! Definition of the fake character set to dump string binary data.
    //!
    //! Using this fake character set (--default-charset DUMP), the binary data
    //! of a string are simply translated in an hexadecimal dump of these data.
    //!
    //! Similarly, when used to serialize tables from an XML file, the string shall
    //! be an hexadecimal string which is directly transposed as binary data.
    //!
    //! @ingroup mpeg
    //!
    class TSDUCKDLL DumpCharset: public Charset
    {
        TS_NOCOPY(DumpCharset);
    public:
        //! Only one predefined "dump character set".
        static const DumpCharset DUMP;

        // Inherited methods.
        virtual bool decode(UString& str, const uint8_t* data, size_t size) const override;
        virtual bool canEncode(const UString& str, size_t start = 0, size_t count = NPOS) const override;
        virtual size_t encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = NPOS) const override;

    private:
        // Private constructor since only one instance is available.
        explicit DumpCharset(const UChar* name = nullptr);
    };
}

TS_POP_WARNING()
