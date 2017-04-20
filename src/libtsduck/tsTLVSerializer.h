//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Serialization of TLV messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTLV.h"
#include "tsByteBlock.h"

namespace ts {
    namespace tlv {

        // A DVB message is serialized in TLV into a ByteBlock.
        // A Serializer is always associated to a ByteBlock.
        class TSDUCKDLL Serializer
        {
        private:
            // Private members:
            ByteBlockPtr _bb;    // Associated binary block
            int _length_offset;  // Location of TLV "length" field

        public:
            // Constructor: associates an existing message block
            Serializer (const ByteBlockPtr &bb) :
                _bb (bb),
                _length_offset (-1)
            {
            }

            // Constructor: use same message block as another serializer.
            // Useful to nest serializer when building compound TLV parameters.
            Serializer (const Serializer& s) :
                _bb (s._bb),
                _length_offset (-1)
            {
            }

            // Destructor: close potential pending TLV
            ~Serializer ()
            {
                if (_length_offset >= 0) {
                    closeTLV ();
                }
            }

        private:
            // Unreachable constructors and operators.
            Serializer ();
            Serializer& operator= (const Serializer&);

        public:
            // Open and close a TLV. Cannot be nested in the same serializer.
            // Use nested factories but not nested TLV into one serializer.
            void openTLV (TAG tag);
            void closeTLV ();

            // Insert an integer value in the stream.
            void putUInt8  (uint8_t  i) {_bb->appendUInt8  (i);}
            void putUInt16 (uint16_t i) {_bb->appendUInt16 (i);}
            void putUInt32 (uint32_t i) {_bb->appendUInt32 (i);}
            void putUInt64 (uint64_t i) {_bb->appendUInt64 (i);}

            void putInt8  (int8_t  i) {_bb->appendInt8  (i);}
            void putInt16 (int16_t i) {_bb->appendInt16 (i);}
            void putInt32 (int32_t i) {_bb->appendInt32 (i);}
            void putInt64 (int64_t i) {_bb->appendInt64 (i);}

            // Insert a TLV field containing one integer.
            void putUInt8  (TAG tag, uint8_t  i) {_bb->appendUInt16 (tag); _bb->appendUInt16 (1); _bb->appendUInt8  (i);}
            void putUInt16 (TAG tag, uint16_t i) {_bb->appendUInt16 (tag); _bb->appendUInt16 (2); _bb->appendUInt16 (i);}
            void putUInt32 (TAG tag, uint32_t i) {_bb->appendUInt16 (tag); _bb->appendUInt16 (4); _bb->appendUInt32 (i);}
            void putUInt64 (TAG tag, uint64_t i) {_bb->appendUInt16 (tag); _bb->appendUInt16 (8); _bb->appendUInt64 (i);}

            void putInt8  (TAG tag, int8_t  i) {_bb->appendUInt16 (tag); _bb->appendUInt16 (1); _bb->appendInt8  (i);}
            void putInt16 (TAG tag, int16_t i) {_bb->appendUInt16 (tag); _bb->appendUInt16 (2); _bb->appendInt16 (i);}
            void putInt32 (TAG tag, int32_t i) {_bb->appendUInt16 (tag); _bb->appendUInt16 (4); _bb->appendInt32 (i);}
            void putInt64 (TAG tag, int64_t i) {_bb->appendUInt16 (tag); _bb->appendUInt16 (8); _bb->appendInt64 (i);}

            // Insert each integer in the vector as one TLV field.
            void putUInt8  (TAG, const std::vector<uint8_t>&);
            void putUInt16 (TAG, const std::vector<uint16_t>&);
            void putUInt32 (TAG, const std::vector<uint32_t>&);
            void putUInt64 (TAG, const std::vector<uint64_t>&);

            void putInt8  (TAG, const std::vector<int8_t>&);
            void putInt16 (TAG, const std::vector<int16_t>&);
            void putInt32 (TAG, const std::vector<int32_t>&);
            void putInt64 (TAG, const std::vector<int64_t>&);

            // Insert a boolean value in the stream.
            void putBool (bool val) {putUInt8 (val ? 1 : 0);}
            void putBool (TAG tag, bool val) {putUInt8 (tag, val ? 1 : 0);}
            void putBool (TAG tag, const std::vector<bool>& val);

            // Template variants
            template <typename INT>
            void put (INT i) {_bb->append<INT> (i);}

            template <typename INT>
            void put (TAG tag, INT i) {_bb->appendUInt16 (tag); _bb->appendUInt16 (sizeof(INT)); _bb->append<INT> (i);}

            template <typename INT>
            void put (TAG tag, const std::vector<INT>& val)
            {
                for (typename std::vector<INT>::const_iterator it = val.begin(); it != val.end(); ++it) {
                    put<INT> (tag, *it);
                }
            }

            // Insert strings in the stream
            void put (const std::string& val) {put (val.data (), val.size ());}
            void put (TAG tag, const std::string& val) {_bb->appendUInt16(tag); _bb->appendUInt16(uint16_t(val.size())); _bb->append(val.data(), val.size());}
            void put (TAG tag, const std::vector<std::string>& val);

            // Insert a byte block in the stream
            void put (const ByteBlock& bl) {_bb->append (bl.data (), bl.size ());}
            void put (TAG tag, const ByteBlock& bl) {_bb->appendUInt16(tag); _bb->appendUInt16(uint16_t(bl.size())); _bb->append(bl.data (), bl.size ());}

            // Insert raw data in the stream
            void put (const void *pval, size_t len) {_bb->append (pval, len);}
            void put (TAG tag, const void *pval, size_t len) {_bb->appendUInt16(tag); _bb->appendUInt16(uint16_t(len)); _bb->append(pval, len);}

            // Convert to a string (for debug purpose)
            operator std::string() const;
        };

        // Template specializations
        template<> inline void Serializer::put (bool val) {putBool (val);}
        template<> inline void Serializer::put (TAG tag, bool val) {putBool (tag, val);}
        template<> inline void Serializer::put (TAG tag, const std::vector<bool>& val) {putBool (tag, val);}
    }
}

// Output operator (for debug purpose)
TSDUCKDLL inline std::ostream& operator<< (std::ostream& strm, const ts::tlv::Serializer& ser)
{
    return strm << std::string (ser);
}
