//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    // warning C5291: 'ts::AbstractOutputStream': deriving from the base class 'std::basic_ostream<char,std::char_traits<char>>'
    // can cause potential runtime issues due to an ABI bug. Recommend adding a 4-byte data member to the base class for the
    // padding at the end of it to work around this bug.
    TS_PUSH_WARNING()
    TS_MSC_NOWARNING(5291)

    //!
    //! Intermediate abstract class to help implementing @c std::ostream.
    //! @ingroup libtscore cpp
    //!
    class TSCOREDLL AbstractOutputStream:
        public std::basic_ostream<char>,     // Public base
        private std::basic_streambuf<char>   // Internally use a streambuf
    {
        TS_POP_WARNING()
        TS_NOCOPY(AbstractOutputStream);
    public:
        //!
        //! Explicit reference to the public superclass.
        //!
        using SuperClass = std::basic_ostream<char>;

        // These types are declared by std::basic_ostream and are inherited.
        // But the same names are also declared by the private base class basic_streambuf.
        // Because of this conflict, they are hidden. We restore here the visibility
        // of the names which are inherited by the public base class.
#if !defined(DOXYGEN)
        using char_type = SuperClass::char_type;
        using traits_type = SuperClass::traits_type;
        using int_type = SuperClass::int_type;
        using pos_type = SuperClass::pos_type;
        using off_type = SuperClass::off_type;
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
        std::string _buffer {};  // Internal buffer for std::streambuf

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
