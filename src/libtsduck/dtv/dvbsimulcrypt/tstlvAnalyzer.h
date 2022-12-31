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
//!  Analysis (deserialization) of TLV messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlv.h"

namespace ts {
    namespace tlv {
        //!
        //! Analysis (deserialization) of TLV messages.
        //! @ingroup tlv
        //!
        class TSDUCKDLL Analyzer
        {
            TS_NOBUILD_NOCOPY(Analyzer);
        public:
            //!
            //! Constructor.
            //! Associate the analyzer object with the address and size of the
            //! binary message. The corresponding memory area must remain alive
            //! as long as the object exists. Also pre-analyze the first TLV field.
            //! @param [in] addr Address of the binary message.
            //! @param [in] size Size in bytes of the binary message.
            //!
            Analyzer(const void* addr, size_t size);

            //!
            //! Check if the end of message has been reached.
            //! @return True if the end of message has been reached.
            //!
            bool endOfMessage() const
            {
                return _eom;
            }

            //!
            //! Check if the rest of the message is valid.
            //! When valid() becomes false, endOfMessage() also become false.
            //! @return True if the rest of the message is valid.
            //!
            bool valid() const
            {
                return _valid;
            }

            //!
            //! Get the address of the current TLV field.
            //! @return Address of the current TLV field.
            //!
            const void* fieldAddr() const
            {
                return _tlv_addr;
            }

            //!
            //! Get the complete size of the current TLV field.
            //! @return Complete size of the current TLV field.
            //!
            size_t fieldSize() const
            {
                return _tlv_size;
            }

            //!
            //! Get the tag of the current TLV field.
            //! @return Tag of the current TLV field.
            //!
            TAG tag() const
            {
                return _tag;
            }

            //!
            //! Get the address of the value field in the current TLV field.
            //! @return The address of the value field in the current TLV field.
            //!
            const void* valueAddr() const
            {
                return _value_addr;
            }

            //!
            //! Get the length of the value field in the current TLV field.
            //! @return The length of the value field in the current TLV field.
            //!
            LENGTH length() const
            {
                return _length;
            }

            //!
            //! Analyze the next TLV field.
            //!
            void next();

        private:
            // Private members
            const char* _base;        // start of global message
            const char* _end;         // end of global message
            bool        _eom;         // end of message
            bool        _valid;       // TLV structure is valid
            const char* _tlv_addr;    // address of current TLV field
            size_t      _tlv_size;    // size of current TLV field
            TAG         _tag;         // tag of current TLV field
            const char* _value_addr;  // address of value in current TLV field
            LENGTH      _length;      // length of current TLV field
        };
    }
}
