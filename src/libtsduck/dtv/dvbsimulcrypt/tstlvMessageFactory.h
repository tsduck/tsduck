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
//!  Factory class for TLV messages
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvProtocol.h"

namespace ts {
    namespace tlv {

        class MessageFactory;

        //!
        //! Safe pointer for MessageFactory (not thread-safe).
        //!
        typedef SafePtr <MessageFactory, NullMutex> MessageFactoryPtr;

        //!
        //! Factory class for TLV messages
        //! @ingroup tlv
        //!
        //! The following methods should be used by the application
        //! to deserialize messages:
        //! - Constructors
        //! - errorStatus()
        //! - errorInformation()
        //! - commandTag()
        //! - protocolVersion()
        //! - factory()
        //! - buildErrorResponse()
        //!
        //! The following types and methods should be used by the
        //! constructors of the ts::tlv::Message subclasses.
        //! - Parameter
        //! - count()
        //! - get()
        //! - getCompound()
        //!
        //! The get() and getCompound() methods retrieve the value of parameters.
        //! For each parameter type, two versions are available.
        //! - The first version returns the first occurence of a
        //!   parameter and is typically used when the cardinality
        //!   of a parameter is 1 or 0 to 1. In the later case,
        //!   the message deserialization routine should first check
        //!   the availability of the parameter using count().
        //! - The second version returns all occurences of the
        //!   parameter in a vector.
        //!
        //! An exception is thrown when the parameter is not present
        //! (first version) or when the actual size of the parameter
        //! does not match the expected size of the type. In both
        //! cases, this should not happen in properly written message
        //! classes since the validity of the parameters were checked
        //! by the constructor of the MessageFactory.
        //!
        class TSDUCKDLL MessageFactory
        {
            TS_NOBUILD_NOCOPY(MessageFactory);
        public:
            //!
            //! Constructor: Analyze a TLV message in memory.
            //! @param [in] addr Address of a binary TLV message.
            //! @param [in] size Size in bytes of the message.
            //! @param [in] protocol The message is validated according to this protocol.
            //!
            MessageFactory(const void* addr, size_t size, const Protocol* protocol);

            //!
            //! Constructor: Analyze a TLV message in memory.
            //! @param [in] bb Binary TLV message.
            //! @param [in] protocol The message is validated according to this protocol.
            //!
            MessageFactory(const ByteBlock &bb, const Protocol* protocol);

            //!
            //! Get the "error status" resulting from the analysis of the message.
            //! @return The error status. If not OK, there is no valid message.
            //!
            tlv::Error errorStatus() const
            {
                return _error_status;
            }

            //!
            //! Get the "error information" resulting from the analysis of the message.
            //! @return The error information.
            //!
            uint16_t errorInformation() const
            {
                return _error_info;
            }

            //!
            //! Get the message tag.
            //! @return The message tag.
            //!
            TAG commandTag() const {return _command_tag;}

            //!
            //! Get the protocol version number.
            //! @return The protocol version number.
            //!
            VERSION protocolVersion() const
            {
                return _protocol_version;
            }

            //!
            //! Return the fully rebuilt message.
            //! Valid only when errorStatus() == OK.
            //! @param [out] msg Safe pointer to the rebuilt message.
            //! Set a null pointer on error.
            //!
            void factory(MessagePtr& msg) const;

            //!
            //! Return the fully rebuilt message.
            //! Valid only when errorStatus() == OK.
            //! @return Safe pointer to the rebuilt message or null pointer on error.
            //!
            MessagePtr factory() const;

            //!
            //! Return the error response for the peer.
            //! Valid only when errorStatus() != OK.
            //! @param [out] msg Safe pointer to the error response message.
            //! Set a null pointer without error.
            //!
            void buildErrorResponse(MessagePtr& msg) const;

            //!
            //! Return the error response for the peer.
            //! Valid only when errorStatus() != OK.
            //! @return Safe pointer to the error response message or null pointer without error.
            //!
            MessagePtr errorResponse() const;

            //!
            //! Location of one parameter value inside the message block.
            //! Address and size point into the original message buffer, use with care!
            //!
            struct Parameter
            {
                // Public fields:
                const void* tlv_addr;  //!< Address of parameter TLV structure.
                size_t      tlv_size;  //!< Size of parameter TLV structure.
                const void* addr;      //!< Address of parameter value.
                LENGTH      length;    //!< Length of parameter value.

                //!
                //! Constructor.
                //! @param [in] tlv_addr_ Address of parameter TLV structure.
                //! @param [in] tlv_size_ Size of parameter TLV structure.
                //! @param [in] addr_ Address of parameter value.
                //! @param [in] length_ Length of parameter value.
                //!
                Parameter(const void* tlv_addr_ = nullptr,
                          size_t      tlv_size_ = 0,
                          const void* addr_     = nullptr,
                          LENGTH      length_   = 0) :
                    tlv_addr(tlv_addr_),
                    tlv_size(tlv_size_),
                    addr(addr_),
                    length(length_)
                {
                }
            };

            //!
            //! Get actual number of occurences of a parameter.
            //! @param [in] tag Parameter tag to search.
            //! @return The actual number of occurences of a parameter.
            //!
            size_t count(TAG tag) const
            {
                return _params.count(tag);
            }

            //!
            //! Get the location of a parameter.
            //! Address and size point into the original message buffer, use with care!
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param Description of the parameter value.
            //!
            void get(TAG tag, Parameter& param) const;

            //!
            //! Get the location of all occurences of a parameter.
            //! Address and size point into the original message buffer, use with care!
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param Vector of descriptions of the parameter values.
            //!
            void get(TAG tag, std::vector<Parameter>& param) const;

            //!
            //! Get an integer parameter.
            //! @tparam INT Integer type for the value.
            //! The size of INT must match the parameter size.
            //! @param [in] tag Parameter tag to search.
            //! @return The parameter value.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            INT get(TAG tag) const;

            //!
            //! Get an integer parameter.
            //! @tparam INT Integer type for the value.
            //! The size of INT must match the parameter size.
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param All parameter values.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            void get(TAG tag, std::vector<INT>& param) const;

            //!
            //! Get a boolean parameter.
            //! This method returns a vector of booleans.
            //! For one single boolean value, use the template version.
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param All parameter values.
            //!
            void get(TAG tag, std::vector<bool>& param) const;

            //!
            //! Get a string parameter.
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param Parameter value.
            //!
            void get(TAG tag, std::string& param) const
            {
                Parameter p;
                get(tag, p);
                param.assign(static_cast<const char*>(p.addr), p.length);
            }

            //!
            //! Get a string parameter.
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param All parameter values.
            //!
            void get(TAG tag, std::vector<std::string>& param) const;

            //!
            //! Get an opaque byte block parameter.
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param Parameter value.
            //!
            void get(TAG tag, ByteBlock& param) const
            {
                Parameter p;
                get(tag, p);
                param.copy(static_cast<const uint8_t*>(p.addr), p.length);
            }

            //!
            //! Get a compound TLV parameter.
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param Safe pointer to the parameter value.
            //!
            void getCompound(TAG tag, MessagePtr& param) const;

            //!
            //! Get a compound TLV parameter.
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param Vector of safe pointers to all parameter values.
            //!
            void getCompound(TAG tag, std::vector<MessagePtr>& param) const;

            //!
            //! Get a compound TLV parameter (template version).
            //! @tparam MSG Expected class of parameter value, a subclass of ts::tlv::Message.
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param Parameter value.
            //!
            template <class MSG>
            void getCompound(TAG tag, MSG& param) const;

            //!
            //! Get a compound TLV parameter (template version).
            //! @tparam MSG Expected class of parameter value, a subclass of ts::tlv::Message.
            //! @param [in] tag Parameter tag to search.
            //! @param [out] param Vector of all parameter values.
            //!
            template <class MSG>
            void getCompound(TAG tag, std::vector<MSG>& param) const;

        private:
            // Internal description of a parameter.
            // Include the description of compound TLV parameter.
            // When compound.isNull(), this is not a compound TLV parameter.
            struct ExtParameter : public Parameter
            {
                // Public fields:
                MessageFactoryPtr compound; // for compound TLV parameter

                // Constructor:
                ExtParameter(const void*     tlv_addr_ = nullptr,
                             size_t          tlv_size_ = 0,
                             const void*     addr_ = nullptr,
                             LENGTH          length_ = 0,
                             MessageFactory* compound_ = nullptr) :
                    Parameter(tlv_addr_, tlv_size_, addr_, length_),
                    compound(compound_)
                {
                }
            };

            // MessageFactory private members:
            const uint8_t*  _msg_base;             // Addresse of raw TLV message
            size_t          _msg_length;           // Size of raw TLV message
            const Protocol* _protocol;             // Associated protocol definition
            tlv::Error      _error_status;         // Error status or OK
            uint16_t        _error_info;           // Associated error information
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
            void checkParamSize(TAG, const ParameterMultimap::const_iterator&) const;
        };

        // Template specializations for performance.
        //! @cond nodoxygen
        template<> inline bool MessageFactory::get<bool>(TAG tag) const {return get<uint8_t>(tag) != 0;}
        template<> inline size_t MessageFactory::dataSize<bool>() const {return 1;}
        //! @endcond
    }
}

#include "tstlvMessageFactoryTemplate.h"
