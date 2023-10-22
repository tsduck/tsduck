//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Iterator for common AVC/HEVC/VVC video access units.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCodecType.h"
#include "tsPSI.h"
#include "tsAVC.h"

namespace ts {
    //!
    //! Iterator for common AVC/HEVC/VVC video access units.
    //! @ingroup mpeg
    //!
    //! Some H.26x video coding formats use a common access unit bitstream
    //! format. This class is an iterator over the payload of a PES packet
    //! (possibly truncated) to locate each access unit.
    //!
    //! This class can be used with:
    //!  - AVC, Advanced Video Coding, ISO 14496-10, ITU-T Rec. H.264.
    //!  - HEVC, High Efficiency Video Coding, ITU-T Rec. H.265.
    //!  - VVC, Versatile Video Coding, ITU-T Rec. H.266.
    //!
    class TSDUCKDLL AccessUnitIterator
    {
        TS_NOBUILD_NOCOPY(AccessUnitIterator);
    public:
        //!
        //! Constructor.
        //! The current access unit is set to the first one.
        //! @param [in] data Address of data to explore, typically a PES packet payload or the start of one.
        //! This data area must not be modified during the lifetime of this object.
        //! @param [in] size Data size in bytes.
        //! @param [in] stream_type Optional stream type, as found in the PMT. Used as a hint.
        //! @param [in] default_format Default encoding format if it cannot be determined from @a stream_type.
        //! If @a stream_type and @a default_format are both unspecified, access unit types values cannot be
        //! extracted.
        //!
        AccessUnitIterator(const uint8_t* data, size_t size, uint8_t stream_type = ST_NULL, CodecType default_format = CodecType::UNDEFINED);

        //!
        //! Check if the video format is valid.
        //! @return True if the video format is valid.
        //! False if @a stream_type and @a default_format were both unspecified and no usual NALunit header is found.
        //!
        bool isValid() const { return _valid; }

        //!
        //! Get the video format.
        //! @return The video format.
        //!
        CodecType videoFormat() const { return _format; }

        //!
        //! Get the address of the current access unit.
        //! @return The address of the current access unit or a null pointer on error or end of data.
        //!
        const uint8_t* currentAccessUnit() const { return _nalunit; }

        //!
        //! Get the offset of the current access unit inside the data area.
        //! @return The offset of the current access unit inside the data area.
        //!
        size_t currentAccessUnitOffset() const { return _nalunit == nullptr ? 0 : _nalunit - _data; }

        //!
        //! Get the size of the current access unit.
        //! @return The size in bytes of the current access unit.
        //!
        size_t currentAccessUnitSize() const { return _nalunit_size; }

        //!
        //! Get the size of the header of the current access unit.
        //! @return The size in bytes of the header of the current access unit.
        //! This is usually 1 or 2 byes, depending on the video codec.
        //!
        size_t currentAccessUnitHeaderSize() const { return _nalunit_header_size; }

        //!
        //! Get the index of the current access unit.
        //! @return The index of the current access unit. This is zero for first
        //! access unit and so on. When atEnd() is true, this is the number of
        //! access units which were found in the PES packet.
        //!
        size_t currentAccessUnitIndex() const { return _nalunit_index; }

        //!
        //! Get the current access unit type.
        //! @return The current access unit. Return AVC_AUT_INVALID if the video format is undefined.
        //!
        uint8_t currentAccessUnitType() const { return _nalunit_type; }

        //!
        //! Check if the current access unit is a Supplemental Enhancement Information (SEI).
        //! The syntax of the SEI access unit is common between AVC, HEVC and VVC but the
        //! access unit type is different. This method checks if the current access unit is
        //! an SEI for the current video format.
        //! @return True if the current access unit is an SEI.
        //!
        bool currentAccessUnitIsSEI() const;

        //!
        //! Iterate to the next access unit.
        //! @return True on success, false when the end of the data area is reached.
        //!
        bool next();

        //!
        //! Check if the last access unit was passed.
        //! @return True if the last access unit was passed.
        //!
        bool atEnd() const { return _nalunit == nullptr; }

        //!
        //! Reset the exploration of the data area at the beginning.
        //!
        void reset();

    private:
        const uint8_t* const _data = nullptr;
        const size_t   _data_size = 0;
        bool           _valid = false;
        CodecType      _format = CodecType::UNDEFINED;
        const uint8_t* _nalunit = nullptr;
        size_t         _nalunit_size = 0;
        size_t         _nalunit_header_size = 0;
        size_t         _nalunit_index = 0;
        uint8_t        _nalunit_type = AVC_AUT_INVALID;
    };
}
