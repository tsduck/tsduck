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
//
//  Abstract base class for TLV messages
//
//----------------------------------------------------------------------------

#pragma once
#include "tsTLVSerializer.h"
#include "tsHexa.h"
#include "tsFormat.h"
#include "tsSafePtr.h"

namespace ts {
    namespace tlv {

        class TSDUCKDLL Message
        {
        public:
            // Public accessors for common fields
            bool hasProtocolVersion() const {return _has_version;}
            VERSION protocolVersion() const {return _version;}
            TAG tag() const {return _tag;}

            // Serialize a message using a Serializer.
            void serialize (Serializer&) const;

            // Virtual destructor
            virtual ~Message() {}

            // Dump routine. Create a string representing the message content.
            // The implementation in the base class dumps the common fields.
            // Can be used by subclasses.
            virtual std::string dump (size_t indent = 0) const;

        protected:
            // This protected contructors are used by subclasses to initialize
            // the common fields in the message header.
            Message (TAG tag) :
                _has_version (false),
                _version     (0),
                _tag         (tag)
            {
            }

            Message (VERSION protocol_version, TAG tag) :
                _has_version (true),
                _version     (protocol_version),
                _tag         (tag)
            {
            }

            // This protected pure virtual method must be implemented
            // by subclasses to serialize their parameters.
            virtual void serializeParameters (Serializer&) const = 0;

            // Helper routines for dump methods in subclasses
            static std::string dumpVector (size_t indent, const char* name, const std::vector<std::string>&);
            static std::string dumpOptional (size_t indent, const char* name, bool has_value, const ByteBlock&, uint32_t flags = hexa::HEXA | hexa::ASCII);

            template <typename INT>
            static std::string dumpDecimal (size_t indent, const char* name, const INT& value)
            {
                return Format ("%*s%s = %" FMT_INT64 "d\n", int (indent), "", name, int64_t (value));
            }

            template <typename INT>
            static std::string dumpHexa (size_t indent, const char* name, const INT& value)
            {
                return Format ("%*s%s = 0x%0*" FMT_INT64 "X\n", int (indent), "", name, int (2 * sizeof(INT)),
                               uint64_t (uint64_t (value) & (TS_UCONST64 (0xFFFFFFFFFFFFFFFF) >> (64 - 8 * sizeof(INT)))));
            }

            template <typename INT>
            static std::string dumpInteger (size_t indent, const char* name, const INT& value)
            {
                return std::numeric_limits<INT>::is_signed ? dumpDecimal (indent, name, value) : dumpHexa (indent, name, value);
            }

            template <typename INT>
            static std::string dumpOptionalDecimal (size_t indent, const char* name, bool has_value, const INT& value)
            {
                return has_value ? dumpDecimal (indent, name, value) : "";
            }

            template <typename INT>
            static std::string dumpOptionalHexa (size_t indent, const char* name, bool has_value, const INT& value)
            {
                return has_value ? dumpHexa (indent, name, value) : "";
            }

            template <typename INT>
            static std::string dumpOptionalInteger (size_t indent, const char* name, bool has_value, const INT& value)
            {
                return has_value ? dumpInteger (indent, name, value) : "";
            }

            template <typename INT>
            static std::string dumpVector (size_t indent, const char* name, const std::vector<INT>& val)
            {
                std::string s;
                for (typename std::vector<INT>::const_iterator it = val.begin(); it != val.end(); ++it) {
                    s += dumpOptionalInteger (indent, name, true, *it);
                }
                return s;
            }

        private:
            // Unreachable constructor, must be overloaded
            Message();

            // Private members
            bool    _has_version;
            VERSION _version;
            TAG     _tag;
        };

        // Reference-counted auto-pointer for TLV messages (MT = multi-thread)
        typedef SafePtr <Message, NullMutex> MessagePtr;
        typedef SafePtr <Message, Mutex> MessagePtrMT;
    }
}
