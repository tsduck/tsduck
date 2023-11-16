//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Factory class for TLV messages
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvProtocol.h"
#include "tsMemory.h"

namespace ts {
    namespace tlv {

        class MessageFactory;

        //!
        //! Safe pointer for MessageFactory (not thread-safe).
        //!
        typedef SafePtr <MessageFactory, ts::null_mutex> MessageFactoryPtr;

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
            MessageFactory(const void* addr, size_t size, const Protocol& protocol);

            //!
            //! Constructor: Analyze a TLV message in memory.
            //! @param [in] bb Binary TLV message.
            //! @param [in] protocol The message is validated according to this protocol.
            //!
            MessageFactory(const ByteBlock &bb, const Protocol& protocol);

            //!
            //! Get the "error status" resulting from the analysis of the message.
            //! @return The error status. If not OK, there is no valid message.
            //!
            tlv::Error errorStatus() const { return _error_status; }

            //!
            //! Get the "error information" resulting from the analysis of the message.
            //! @return The error information.
            //!
            uint16_t errorInformation() const { return _error_info; }

            //!
            //! Get the message tag.
            //! @return The message tag.
            //!
            TAG commandTag() const { return _command_tag; }

            //!
            //! Get the protocol version number.
            //! @return The protocol version number.
            //!
            VERSION protocolVersion() const { return _protocol_version; }

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
            class Parameter
            {
            public:
                const void* tlv_addr = nullptr;  //!< Address of parameter TLV structure.
                size_t      tlv_size = 0;        //!< Size of parameter TLV structure.
                const void* addr = nullptr;      //!< Address of parameter value.
                LENGTH      length = 0;          //!< Length of parameter value.

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
            size_t count(TAG tag) const { return _params.count(tag); }

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
                MessageFactoryPtr compound {}; // for compound TLV parameter

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
            const uint8_t*  _msg_base = nullptr;           // Addresse of raw TLV message
            size_t          _msg_length = 0;               // Size of raw TLV message
            const Protocol& _protocol;                     // Associated protocol definition
            tlv::Error      _error_status {OK};            // Error status or OK
            uint16_t        _error_info = 0;               // Associated error information
            bool            _error_info_is_offset = false; // Error info is an offset in message
            VERSION         _protocol_version = 0;
            TAG             _command_tag = 0;

            // Location of actual parameters. Point into the message block.
            typedef std::multimap <TAG, ExtParameter> ParameterMultimap;
            ParameterMultimap _params {};

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


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Internal method: Check the size of a parameter.
template <typename T>
void ts::tlv::MessageFactory::checkParamSize(TAG tag, const ParameterMultimap::const_iterator& it) const
{
    const size_t expected = dataSize<T>();
    if (it->second.length != expected) {
        throw DeserializationInternalError(UString::Format(u"Bad size for parameter 0x%X in message, expected %d bytes, found %d", {tag, expected, it->second.length}));
    }
}

// Get first occurence of an integer parameter:
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::tlv::MessageFactory::get(TAG tag) const
{
    const auto it = _params.find(tag);
    if (it == _params.end()) {
        throw DeserializationInternalError(UString::Format(u"No parameter 0x%X in message", {tag}));
    }
    else {
        checkParamSize<INT>(tag, it);
        return GetInt<INT>(it->second.addr);
    }
}

// Get all occurences of an integer parameter.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::tlv::MessageFactory::get(TAG tag, std::vector<INT>& param) const
{
    // Reinitialize result vector
    param.clear();
    param.reserve(_params.count(tag));
    // Fill vector with parameter values
    const auto last = _params.upper_bound (tag);
    for (auto it = _params.lower_bound(tag); it != last; ++it) {
        checkParamSize<INT>(tag, it);
        param.push_back(GetInt<INT>(it->second.addr));
    }
}

// Get a compound TLV parameter using a derived class of Message.
template <class MSG>
void ts::tlv::MessageFactory::getCompound(TAG tag, MSG& param) const
{
    MessagePtr gen;
    getCompound (tag, gen);
    MSG* msg = dynamic_cast<MSG*> (gen.pointer());
    if (msg == 0) {
        throw DeserializationInternalError(UString::Format(u"Wrong compound TLV type for parameter 0x%X", {tag}));
    }
    param = *msg;
}

// Get all occurences of a compound TLV parameter using a derived class of Message.
template <class MSG>
void ts::tlv::MessageFactory::getCompound(TAG tag, std::vector<MSG>& param) const
{
    // Reinitialize result vector
    param.clear();
    // Fill vector with parameter values
    auto it = _params.lower_bound(tag);
    const auto last = _params.upper_bound(tag);
    for (int i = 0; it != last; ++it, ++i) {
        if (it->second.compound.isNull()) {
            throw DeserializationInternalError(UString::Format(u"Occurence %d of parameter 0x%X not a compound TLV", {i, tag}));
        }
        else {
            MessagePtr gen;
            it->second.compound->factory(gen);
            MSG* msg = dynamic_cast<MSG*> (gen.pointer());
            if (msg == 0) {
                throw DeserializationInternalError(UString::Format(u"Wrong compound TLV type for occurence %d of parameter 0x%X", {i, tag}));
            }
            param.push_back(*msg);
        }
    }
}
