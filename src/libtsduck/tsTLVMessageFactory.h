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
//  Factory class for TLV messages
//
//----------------------------------------------------------------------------

#pragma once
#include "tsTLVProtocol.h"

namespace ts {
    namespace tlv {

        // Reference-counted auto-pointer for MessageFactory (not thread-safe)
        class MessageFactory;
        typedef SafePtr <MessageFactory, NullMutex> MessageFactoryPtr;

        // Definition of the MessageFactory
        class TSDUCKDLL MessageFactory
        {
        public:
            //-----------------------------------------------------------------
            // The following methods should be used by the application
            // to deserialize messages.
            //-----------------------------------------------------------------

            // Constructor: Analyze a TLV message in memory.
            // The message is validated according to the specified protocol.
            MessageFactory (const void* addr, size_t size, const Protocol* protocol) :
                _msg_base             (reinterpret_cast<const uint8_t*> (addr)),
                _msg_length           (size),
                _protocol             (protocol),
                _error_status         (OK),
                _error_info           (0),
                _error_info_is_offset (false),
                _protocol_version     (0),
                _command_tag          (0)
            {
                analyzeMessage ();
            }

            MessageFactory (const ByteBlock &bb, const Protocol* protocol) :
                _msg_base             (bb.data()),
                _msg_length           (bb.size()),
                _protocol             (protocol),
                _error_status         (OK),
                _error_info           (0),
                _error_info_is_offset (false),
                _protocol_version     (0),
                _command_tag          (0)
            {
                analyzeMessage ();
            }

            // The following two methods return the "error status" and
            // "error information" resulting from the analysis of the
            // message. If ErrorStatus() is not OK, there is no valid message.
            tlv::Error errorStatus() const {return _error_status;}
            uint16_t errorInformation() const {return _error_info;}

            // Public accessor for the common fields:
            TAG commandTag() const {return _command_tag;}
            VERSION protocolVersion() const {return _protocol_version;}

            // Return the fully rebuilt message.
            // Valid only when errorStatus() == OK.
            void factory (MessagePtr& msg) const
            {
                assert (_error_status == OK);
                _protocol->factory (*this, msg);
            }

            // Return the error response for the peer.
            // Valid only when errorStatus() != OK.
            void buildErrorResponse (MessagePtr& msg) const
            {
                assert (_error_status != OK);
                _protocol->buildErrorResponse (*this, msg);
            }

            //-----------------------------------------------------------------
            // The following types and methods should be used by the
            // constructors of the Message subclasses.
            //-----------------------------------------------------------------

            // Location of one parameter value inside the message block:
            struct Parameter
            {
                // Public fields:
                const void* tlv_addr;  // address of parameter TLV structure
                size_t      tlv_size;  // size of parameter TLV structure
                const void* addr;      // address of parameter value
                LENGTH      length;    // length of parameter value

                // Constructor:
                Parameter (const void* tlv_addr_ = 0,
                           size_t      tlv_size_ = 0,
                           const void* addr_     = 0,
                           LENGTH      length_   = 0) :
                    tlv_addr (tlv_addr_),
                    tlv_size (tlv_size_),
                    addr     (addr_),
                    length   (length_)
                {
                }
            };

            // Get actual number of occurences of a parameter:
            size_t count (TAG tag) const {return _params.count (tag);}

            // The following methods retrieve the value of parameters.
            // For each parameter type, two versions are available.
            //
            // - The first version returns the first occurence of a
            //   parameter and is typically used when the cardinality
            //   of a parameter is 1 or 0 to 1. In the later case,
            //   the message deserialization routine should first check
            //   the availability of the parameter using Count().
            //
            // - The second version returns all occurences of the
            //   parameter in a vector.
            //
            // An exception is thrown when the parameter is not present
            // (first version) or when the actual size of the parameter
            // does not match the expected size of the type. In both
            // cases, this should not happen in properly written message
            // classes since the validity of the parameters were checked
            // by the constructor of the MessageFactory.

            // Get the location of a parameter (address and size into the original buffer, use with care).
            void get (TAG tag, Parameter& param) const throw (DeserializationInternalError);
            void get (TAG tag, std::vector<Parameter>& param) const;

            // Get an integer parameter:
            template <typename INT>
            INT get (TAG tag) const throw (DeserializationInternalError);

            template <typename INT>
            void get (TAG tag, std::vector<INT>& param) const throw (DeserializationInternalError);

            // Get a boolean parameter (see also specialization after end of class declaration)
            void get (TAG tag, std::vector<bool>& param) const throw (DeserializationInternalError);

            // Get a string parameter:
            void get (TAG tag, std::string& param) const throw (DeserializationInternalError)
            {
                Parameter p;
                get (tag, p);
                param.assign (static_cast<const char*> (p.addr), p.length);
            }
            void get (TAG tag, std::vector<std::string>& param) const;

            // Get an opaque byte block parameter:
            void get (TAG tag, ByteBlock& param) const throw (DeserializationInternalError)
            {
                Parameter p;
                get (tag, p);
                param.copy (static_cast<const uint8_t*> (p.addr), p.length);
            }

            // Get a compound TLV parameter.
            // First version: use a pointer to a generic message.
            void getCompound (TAG tag, MessagePtr& param) const throw (DeserializationInternalError);
            void getCompound (TAG tag, std::vector<MessagePtr>& param) const throw (DeserializationInternalError);

            // Get a compound TLV parameter.
            // Second version: template using an MSG class derived from Message.
            template <class MSG>
            void getCompound (TAG tag, MSG& param) const throw (DeserializationInternalError);

            template <class MSG>
            void getCompound (TAG tag, std::vector<MSG>& param) const throw (DeserializationInternalError);

        private:
            // Unreachable constructors and operators
            MessageFactory ();
            MessageFactory (const MessageFactory&);
            MessageFactory& operator= (const MessageFactory&);

            // Internal description of a parameter.
            // Include the description of compound TLV parameter.
            // When compound.isNull(), this is not a compound TLV parameter.
            struct ExtParameter : public Parameter
            {
                // Public fields:
                MessageFactoryPtr compound; // for compound TLV parameter

                // Constructor:
                ExtParameter (const void*     tlv_addr_     = 0,
                              size_t          tlv_size_     = 0,
                              const void*     addr_         = 0,
                              LENGTH          length_       = 0,
                              MessageFactory* compound_     = 0) :
                    Parameter (tlv_addr_, tlv_size_, addr_, length_),
                    compound (compound_)
                {
                }
            };

            // MessageFactory private members:
            const uint8_t*    _msg_base;             // Addresse of raw TLV message
            size_t          _msg_length;           // Size of raw TLV message
            const Protocol* _protocol;             // Associated protocol definition
            tlv::Error      _error_status;         // Error status or OK
            uint16_t          _error_info;           // Associated error information
            bool            _error_info_is_offset; // Error info is an offset in message
            VERSION         _protocol_version;
            TAG             _command_tag;

            // Location of actual parameters. Point into the message block.
            typedef std::multimap <TAG, ExtParameter> ParameterMultimap;
            ParameterMultimap _params;

            // Analyze the TLV message, called by constructors.
            void analyzeMessage();

            // Expected size of a type: default is sizeof().
            // Specializations can be provided.
            template <typename T> size_t dataSize() const {return sizeof(T);}

            // Internal method: Check the size of a parameter.
            // Should never throw an exception, except bug in the
            // constructor of the Message subclasses.
            template <typename T>
            void checkParamSize (TAG, const ParameterMultimap::const_iterator&) const throw (DeserializationInternalError);
        };

        // Specializations for get()
        template<> inline bool MessageFactory::get<bool> (TAG tag) const throw (DeserializationInternalError) {return get<uint8_t> (tag) != 0;}

        // Expected size: specializations
        template<> inline size_t MessageFactory::dataSize<bool>() const {return 1;}
    }
}

#include "tsTLVMessageFactoryTemplate.h"
