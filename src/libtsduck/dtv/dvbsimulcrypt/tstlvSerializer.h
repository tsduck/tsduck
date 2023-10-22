//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Serialization of TLV messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlv.h"
#include "tsByteBlock.h"

namespace ts {
    namespace tlv {
        //!
        //! Serialization of TLV messages.
        //! @ingroup tlv
        //!
        //! A DVB message is serialized in TLV into a ByteBlock.
        //! A Serializer is always associated to a ByteBlock.
        //!
        class TSDUCKDLL Serializer
        {
        private:
            // Private members:
            ByteBlockPtr _bb {};      // Associated binary block
            int _length_offset {-1};  // Location of TLV "length" field

        public:
            //!
            //! Constructor.
            //! Associates an existing message block.
            //! @param [in] bb Safe pointer to an existing message block.
            //! The messages will be serialized in this block.
            //!
            Serializer(const ByteBlockPtr& bb) : _bb(bb) {}

            //!
            //! Constructor.
            //! Use the same message block as another Serializer.
            //! Useful to nest serializer when building compound TLV parameters.
            //! @param [in] s Another serializer, will use the same byte block for serialization.
            //!
            Serializer(const Serializer& s) : _bb(s._bb) {}

            //!
            //! Destructor.
            //! Close potential pending TLV.
            //!
            ~Serializer()
            {
                if (_length_offset >= 0) {
                    closeTLV();
                }
            }

        private:
            // Unreachable constructors and operators.
            Serializer() = delete;
            Serializer& operator=(const Serializer&) = delete;

        public:
            //!
            //! Open a TLV structure.
            //! The tag field and a placeholder for the length field are inserted.
            //! Cannot be nested in the same serializer.
            //! Use nested factories but not nested TLV into one serializer.
            //! @param [in] tag Message or parameter tag.
            //!
            void openTLV(TAG tag);

            //!
            //! Close a TLV structure.
            //! The length field is updated.
            //!
            void closeTLV();

            //!
            //! Insert an unsigned 8-bit integer value in the stream.
            //! @param [in] i Integer value to insert.
            //!
            void putUInt8(uint8_t i) {_bb->appendUInt8 (i);}

            //!
            //! Insert an unsigned 16-bit integer value in the stream.
            //! @param [in] i Integer value to insert.
            //!
            void putUInt16(uint16_t i) {_bb->appendUInt16(i);}

            //!
            //! Insert an unsigned 16-bit integer value in the stream.
            //! @param [in] i Integer value to insert.
            //!
            void putUInt32(uint32_t i) {_bb->appendUInt32(i);}

            //!
            //! Insert an unsigned 64-bit integer value in the stream.
            //! @param [in] i Integer value to insert.
            //!
            void putUInt64(uint64_t i) {_bb->appendUInt64(i);}

            //!
            //! Insert a signed 8-bit integer value in the stream.
            //! @param [in] i Integer value to insert.
            //!
            void putInt8(int8_t i) {_bb->appendInt8 (i);}

            //!
            //! Insert a signed 16-bit integer value in the stream.
            //! @param [in] i Integer value to insert.
            //!
            void putInt16(int16_t i) {_bb->appendInt16(i);}

            //!
            //! Insert a signed 32-bit integer value in the stream.
            //! @param [in] i Integer value to insert.
            //!
            void putInt32(int32_t i) {_bb->appendInt32(i);}

            //!
            //! Insert a signed 64-bit integer value in the stream.
            //! @param [in] i Integer value to insert.
            //!
            void putInt64(int64_t i) {_bb->appendInt64(i);}

            //!
            //! Insert a TLV field containing an unsigned 8-bit integer value in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] i Integer value to insert.
            //!
            void putUInt8(TAG tag, uint8_t i) {_bb->appendUInt16(tag); _bb->appendUInt16(1); _bb->appendUInt8 (i);}

            //!
            //! Insert a TLV field containing an unsigned 16-bit integer value in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] i Integer value to insert.
            //!
            void putUInt16(TAG tag, uint16_t i) {_bb->appendUInt16(tag); _bb->appendUInt16(2); _bb->appendUInt16(i);}

            //!
            //! Insert a TLV field containing an unsigned 32-bit integer value in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] i Integer value to insert.
            //!
            void putUInt32(TAG tag, uint32_t i) {_bb->appendUInt16(tag); _bb->appendUInt16(4); _bb->appendUInt32(i);}

            //!
            //! Insert a TLV field containing an unsigned 64-bit integer value in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] i Integer value to insert.
            //!
            void putUInt64(TAG tag, uint64_t i) {_bb->appendUInt16(tag); _bb->appendUInt16(8); _bb->appendUInt64(i);}

            //!
            //! Insert a TLV field containing a signed 8-bit integer value in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] i Integer value to insert.
            //!
            void putInt8(TAG tag, int8_t i) {_bb->appendUInt16(tag); _bb->appendUInt16(1); _bb->appendInt8 (i);}

            //!
            //! Insert a TLV field containing a signed 16-bit integer value in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] i Integer value to insert.
            //!
            void putInt16(TAG tag, int16_t i) {_bb->appendUInt16(tag); _bb->appendUInt16(2); _bb->appendInt16(i);}

            //!
            //! Insert a TLV field containing a signed 32-bit integer value in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] i Integer value to insert.
            //!
            void putInt32(TAG tag, int32_t i) {_bb->appendUInt16(tag); _bb->appendUInt16(4); _bb->appendInt32(i);}

            //!
            //! Insert a TLV field containing a signed 64-bit integer value in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] i Integer value to insert.
            //!
            void putInt64(TAG tag, int64_t i) {_bb->appendUInt16(tag); _bb->appendUInt16(8); _bb->appendInt64(i);}

            //!
            //! Insert a TLV field containing a vector of unsigned 8-bit integer values in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] v Vector of integer values to insert.
            //!
            void putUInt8(TAG tag, const std::vector<uint8_t>& v);

            //!
            //! Insert a TLV field containing a vector of unsigned 16-bit integer values in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] v Vector of integer values to insert.
            //!
            void putUInt16(TAG tag, const std::vector<uint16_t>& v);

            //!
            //! Insert a TLV field containing a vector of unsigned 32-bit integer values in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] v Vector of integer values to insert.
            //!
            void putUInt32(TAG tag, const std::vector<uint32_t>& v);

            //!
            //! Insert a TLV field containing a vector of unsigned 64-bit integer values in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] v Vector of integer values to insert.
            //!
            void putUInt64(TAG tag, const std::vector<uint64_t>& v);

            //!
            //! Insert a TLV field containing a vector of signed 8-bit integer values in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] v Vector of integer values to insert.
            //!
            void putInt8(TAG tag, const std::vector<int8_t>& v);

            //!
            //! Insert a TLV field containing a vector of signed 16-bit integer values in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] v Vector of integer values to insert.
            //!
            void putInt16(TAG tag, const std::vector<int16_t>& v);

            //!
            //! Insert a TLV field containing a vector of signed 32-bit integer values in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] v Vector of integer values to insert.
            //!
            void putInt32(TAG tag, const std::vector<int32_t>& v);

            //!
            //! Insert a TLV field containing a vector of signed 64-bit integer values in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] v Vector of integer values to insert.
            //!
            void putInt64(TAG tag, const std::vector<int64_t>& v);

            //!
            //! Insert a boolean value in the stream.
            //! @param [in] val Boolean value to insert.
            //!
            void putBool(bool val) {putUInt8(val ? 1 : 0);}

            //!
            //! Insert a TLV field containing a boolean value in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] val Boolean value to insert.
            //!
            void putBool(TAG tag, bool val) {putUInt8(tag, val ? 1 : 0);}

            //!
            //! Insert a TLV field containing a vector of boolean values in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] val Vector of boolean values to insert.
            //!
            void putBool(TAG tag, const std::vector<bool>& val);

            //!
            //! Insert an integer value in the stream (template variant).
            //! @tparam INT Integer type.
            //! @param [in] i Integer value to insert.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            void put(INT i) {_bb->append<INT>(i);}

            //!
            //! Insert a TLV field containing an integer value in the stream (template variant).
            //! @tparam INT Integer type.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] i Integer value to insert.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            void put(TAG tag, INT i) {_bb->appendUInt16(tag); _bb->appendUInt16(sizeof(INT)); _bb->append<INT>(i);}

            //!
            //! Insert a TLV field containing a vector of integer values in the stream (template variant).
            //! @tparam INT Integer type.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] val Vector of integer values to insert.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            void put(TAG tag, const std::vector<INT>& val)
            {
                for (auto i : val) {
                    put<INT>(tag, i);
                }
            }

            //!
            //! Insert a string in the stream.
            //! @param [in] val String to insert.
            //!
            void put(const std::string& val)
            {
                put(val.data(), val.size());
            }

            //!
            //! Insert a TLV field containing a string in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] val String to insert.
            //!
            void put(TAG tag, const std::string& val)
            {
                _bb->appendUInt16(tag);
                _bb->appendUInt16(uint16_t(val.size()));
                _bb->append(val.data(), val.size());
            }

            //!
            //! Insert a TLV field containing a vector of strings in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] val Vector of strings to insert.
            //!
            void put(TAG tag, const std::vector<std::string>& val);

            //!
            //! Insert a byte block in the stream.
            //! @param [in] bl Byte block to insert.
            //!
            void put(const ByteBlock& bl)
            {
                _bb->append(bl.data(), bl.size());
            }

            //!
            //! Insert a TLV field containing a byte block in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] bl Byte block to insert.
            //!
            void put(TAG tag, const ByteBlock& bl)
            {
                _bb->appendUInt16(tag);
                _bb->appendUInt16(uint16_t(bl.size()));
                _bb->append(bl.data(), bl.size());
            }

            //!
            //! Insert raw data in the stream.
            //! @param [in] pval Address of data to insert.
            //! @param [in] len Length of data to insert.
            //!
            void put(const void *pval, size_t len)
            {
                _bb->append(pval, len);
            }

            //!
            //! Insert a TLV field containing raw data in the stream.
            //! @param [in] tag Message or parameter tag.
            //! @param [in] pval Address of data to insert.
            //! @param [in] len Length of data to insert.
            //!
            void put(TAG tag, const void *pval, size_t len)
            {
                _bb->appendUInt16(tag);
                _bb->appendUInt16(uint16_t(len));
                _bb->append(pval, len);
            }

            //!
            //! Convert to a string (for debug purpose).
            //! @return A string representing the internal state of the Serializer.
            //!
            UString toString() const;
        };

        // Template specializations for performance.
        //! @cond nodoxygen
        template<> inline void Serializer::put(bool val) {putBool(val);}
        template<> inline void Serializer::put(TAG tag, bool val) {putBool(tag, val);}
        template<> inline void Serializer::put(TAG tag, const std::vector<bool>& val) {putBool(tag, val);}
        //! @endcond
    }
}

//!
//! Output operator for ts::tlv::Serializer (for debug purpose).
//! @param [in,out] strm Output stream (text mode).
//! @param [in] ser Serializer to dump.
//! @return A reference to @a strm.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::tlv::Serializer& ser)
{
    return strm << ser.toString();
}
