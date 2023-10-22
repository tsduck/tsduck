//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Tag, Length, Value (TVL) syntax.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsCerrReport.h"

namespace ts {
    //!
    //! Representation of a Tag, Length, Value (TVL) syntax.
    //! @ingroup cmd
    //!
    //! An instance of this class describe how a part of a data area, typically
    //! in the payload of an MPEG section, is structured as a suite of TLV records.
    //! The complete suite of all contiguous TLV records is named the "TLV area"
    //! inside the larger data area.
    //!
    //! A TLV area is described by the following values:
    //!
    //! - The @e start index in the data area. This is the starting index of
    //!   the first @e tag field. A negative value for @e start means "auto".
    //!   The best match of a "TLV area" will be found automatically.
    //! - The @e size in bytes of the TLV area. A negative value means "auto",
    //!   up to the last consistent TLV record before the end of the data area.
    //! - The @e tag @e size in bytes. The valid sizes are 1 (default), 2 and 4.
    //! - The @e length @e size in bytes. The valid sizes are 1 (default), 2 and 4.
    //! - The byte order of the @e tag and @e length fields. The default is MSB.
    //!
    class TSDUCKDLL TLVSyntax
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] start Starting index of the "TLV area" in the "data area". A negative value means "auto".
        //! @param [in] size Size in bytes of the "TLV area". A negative value means "auto".
        //! @param [in] tagSize Size in bytes of the @e tag field. The valid sizes are 1 (default), 2 and 4.
        //! @param [in] lengthSize Size in bytes of the @e length field. The valid sizes are 1 (default), 2 and 4.
        //! @param [in] msb If true, the @e tag and @e length fields are represented in MSB-first.
        //! @param [in,out] report Where to report errors.
        //!
        TLVSyntax(int start = -1, int size = -1, size_t tagSize = 1, size_t lengthSize = 1, bool msb = true, Report& report = CERR);

        //!
        //! Set the values of a TLVSyntax object.
        //! @param [in] start Starting index of the "TLV area" in the "data area". A negative value means "auto".
        //! @param [in] size Size in bytes of the "TLV area". A negative value means "auto".
        //! @param [in] tagSize Size in bytes of the @e tag field. The valid sizes are 1 (default), 2 and 4.
        //! @param [in] lengthSize Size in bytes of the @e length field. The valid sizes are 1 (default), 2 and 4.
        //! @param [in] msb If true, the @e tag and @e length fields are represented in MSB-first.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error (invalid sizes).
        //!
        bool set(int start = -1, int size = -1, size_t tagSize = 1, size_t lengthSize = 1, bool msb = true, Report& report = CERR);

        //!
        //! Update the TLV syntax to automatically locate the TLV area.
        //!
        void setAutoLocation() { _start = _size = -1; }

        //!
        //! Get the size in bytes of the Tag field.
        //! @return The size in bytes of the Tag field.
        //!
        size_t getTagSize() const { return _tagSize; }

        //!
        //! Get the size in bytes of the Length field.
        //! @return The size in bytes of the Length field.
        //!
        size_t getLengthSize() const { return _lengthSize; }

        //!
        //! Set the values of a TLVSyntax object from a string representation.
        //! Typically used to ingest command line parameters.
        //! @param [in] s The string representation in the form "start,size,tagSize,lengthSize,order".
        //! The @e start and @e size fields can be set to "auto".
        //! The @e order field shall be either "msb" or lsb".
        //! All fields are optional. Missing fields receive the default value as defined in the constructor.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool fromString(const UString& s, Report& report = CERR);

        //!
        //! Extract a tag and length value from a data area.
        //! @param [in] data Address of the TLV record.
        //! @param [in] size Total size in bytes of the data area after @a data address.
        //! @param [out] tag Returned tag value.
        //! @param [out] length Returned length value.
        //! @return The size of the tag and length field, ie. the offset after @a data
        //! where the value field starts. Return 0 on error (if the TLV record does not fit).
        //!
        size_t getTagAndLength(const uint8_t* data, size_t size, uint32_t& tag, size_t& length) const;

        //!
        //! Locate the "TLV area" inside a data area.
        //! If the @e start or @e size are set to "auto", return the best match.
        //! @param [in] data Address of the TLV data area.
        //! @param [in] dataSize Total size in bytes of the data area.
        //! @param [out] tlvStart Starting offset of the "TLV area" after @a data.
        //! @param [out] tlvSize Size in bytes of the "TLV area". Zero if no suitable TLV area was found.
        //! @return True if TLV area found, false otherwise.
        //!
        bool locateTLV(const uint8_t* data, size_t dataSize, size_t& tlvStart, size_t& tlvSize) const;

        //!
        //! Comparison operator, typically used to sort containers of TLVSyntax.
        //! @param [in] other Other instance to compare.
        //! @return True if this object logically preceeds @a other.
        //!
        bool operator<(const TLVSyntax& other) const { return _start < other._start; }

    private:
        int    _start = 0;
        int    _size = 0;
        size_t _tagSize {1};
        size_t _lengthSize {1};
        bool   _msb = true;

        // Compute the size of the longest TLV area starting at tlvStart.
        size_t longestTLV(const uint8_t* data, size_t dataSize, size_t tlvStart) const;

        // Get an integer in the right byte order.
        uint32_t getInt(const uint8_t* data, size_t size) const;
    };

    //!
    //! A vector of TLVSyntax.
    //!
    typedef std::vector<TLVSyntax> TLVSyntaxVector;
}
