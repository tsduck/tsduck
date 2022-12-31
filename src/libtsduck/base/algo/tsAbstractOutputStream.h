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
//!  Intermediate abstract class to help implementing @c std::ostream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Intermediate abstract class to help implementing @c std::ostream.
    //! @ingroup cpp
    //!
    class TSDUCKDLL AbstractOutputStream:
        public std::basic_ostream<char>,     // Public base
        private std::basic_streambuf<char>   // Internally use a streambuf
    {
        TS_NOCOPY(AbstractOutputStream);
    public:
        //!
        //! Explicit reference to the public superclass.
        //!
        typedef std::basic_ostream<char> SuperClass;

        // These types are declared by std::basic_ostream and are inherited.
        // But the same names are also declared by the private base class basic_streambuf.
        // Because of this conflict, they are hidden. We restore here the visibility
        // of the names which are inherited by the public base class.
#if !defined(DOXYGEN)
        typedef SuperClass::char_type char_type;
        typedef SuperClass::traits_type traits_type;
        typedef SuperClass::int_type int_type;
        typedef SuperClass::pos_type pos_type;
        typedef SuperClass::off_type off_type;
#endif

        //!
        //! Default stream buffer size in bytes.
        //!
        static constexpr size_t DEFAULT_STREAM_BUFFER_SIZE = 1024;

        //!
        //! Constructor.
        //! @param [in] bufferSize Buffer size in bytes.
        //!
        explicit AbstractOutputStream(size_t bufferSize = DEFAULT_STREAM_BUFFER_SIZE);

        //!
        //! Destructor.
        //!
        virtual ~AbstractOutputStream() override;

    protected:
        //!
        //! Write buffered data to underlying output devicen whatever it is.
        //! Must be implemented by subclasses.
        //! @param [in] addr Buffered data address.
        //! @param [in] size Buffered data size in bytes.
        //! @return True on success, false on error.
        //!
        virtual bool writeStreamBuffer(const void* addr, size_t size) = 0;

    private:
        std::string _buffer;  // Internal buffer for std::streambuf

        // Inherited from std::basic_streambuf<char>.
        // This is called when buffer becomes full.
        // If buffer is not used, then this is called every time when characters are put to stream.
        virtual int_type overflow(int_type c = traits_type::eof()) override;

        // Inherited from std::basic_streambuf<char>.
        // This function is called when stream is flushed, for example when std::endl is put to stream.
        virtual int sync() override;

        // Reset buffer, make it fully available to std::streambuf.
        void resetBuffer()
        {
            setp(&_buffer[0], &_buffer[0] + _buffer.size());
        }
    };
}
