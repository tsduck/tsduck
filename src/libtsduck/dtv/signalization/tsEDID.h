//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  "Extended Descriptor Id", a synthetic value for identifying descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPSI.h"

namespace ts {
    //!
    //! Extended MPEG descriptor id.
    //! @ingroup mpeg
    //!
    //! For convenience, it is sometimes useful to identify descriptors using
    //! an "extended DID" because a descriptor tag is not always enough to
    //! uniquely identify a descriptor.
    //!
    //! A descriptor usually falls into one of the following categories:
    //! - Standard MPEG or DVB descriptor (tag in the range 0x00-0x7F).
    //! - Private DVB descriptor (tag in the range 0x80-0xFF). Must be
    //!   associated with a 32-bit private data specifier or PDS.
    //! - DVB extension descriptor (tag == 0x7F). Must be associated with
    //!   an 8-bit tag extension.
    //! - DVB table-specific descriptor (tag in the MPEG-defined range 0x00-0x3F).
    //!   Must be associated with an 8-bit table id. Such a descriptor uses a
    //!   reserved standard tag but its meaning changes in the context of a
    //!   specific table such as AIT, INT or UNT. Standard MPEG descriptors
    //!   cannot be used in such tables.
    //!
    class TSDUCKDLL EDID
    {
    private:
        // For simplicity of implementation of operators, we pack everything in a 64-bit value:
        // - 32-bit: PDS (or PDS_NULL)
        // - 8-bit: unused (0xFF)
        // - 8-bit: table-id for table-specific descriptors (or TID_NULL)
        // - 8-bit: tag extension (or EDID_NULL)
        // - 8-bit: descriptor tag (DID)
        uint64_t _edid = 0xFFFFFFFFFFFFFFFF;

        // Private constructor from 64-bit value.
        EDID(uint64_t edid) : _edid(edid) {}

    public:
        //!
        //! Default constructor.
        //!
        EDID() = default;

        //!
        //! Build the EDID for a standard MPEG or DVB descriptor.
        //! @param [in] did Descriptor tag.
        //! @return The corresponding EDID.
        //!
        static EDID Standard(DID did) { return EDID(0xFFFFFFFFFFFFFF00 | (did & 0xFF)); }

        //!
        //! Build the EDID for a private DVB descriptor.
        //! @param [in] did Descriptor tag.
        //! @param [in] pds Associated private data specifier.
        //! @return The corresponding EDID.
        //!
        static EDID Private(DID did, PDS pds) { return EDID((uint64_t(pds) << 32) | 0x00000000FFFFFF00 | (did & 0xFF)); }

        //!
        //! Build the EDID for a DVB extension descriptor.
        //! @param [in] ext Associated tag extension. The descriptor tag is implicitly DID_DVB_EXTENSION.
        //! @return The corresponding EDID.
        //!
        static EDID ExtensionDVB(DID ext) { return EDID(0xFFFFFFFFFFFF0000 | (uint64_t(ext & 0xFF) << 8) | uint64_t(DID_DVB_EXTENSION)); }

        //!
        //! Build the EDID for an MPEG extension descriptor.
        //! @param [in] ext Associated tag extension. The descriptor tag is implicitly DID_MPEG_EXTENSION.
        //! @return The corresponding EDID.
        //!
        static EDID ExtensionMPEG(DID ext) { return EDID(0xFFFFFFFFFFFF0000 | (uint64_t(ext & 0xFF) << 8) | uint64_t(DID_MPEG_EXTENSION)); }

        //!
        //! Build the EDID for a table-specific descriptor.
        //! @param [in] did Descriptor tag.
        //! @param [in] tid Associated required table id.
        //! @return The corresponding EDID.
        //!
        static EDID TableSpecific(DID did, TID tid) { return EDID(0xFFFFFFFFFF00FF00 | (uint64_t(tid & 0xFF) << 16) | (did & 0xFF)); }

        //!
        //! Check if the extended descriptor id is valid.
        //! @return True if valid.
        //!
        bool isValid() const { return did() != 0xFF; }

        //!
        //! Check if the descriptor is a standard one.
        //! @return True if the descriptor is a standard one.
        //!
        bool isStandard() const { return (_edid & 0xFFFFFFFFFFFFFF00) == 0xFFFFFFFFFFFFFF00; }

        //!
        //! Get the descriptor id (aka tag).
        //! @return The descriptor id.
        //!
        DID did() const { return DID(_edid & 0xFF); }

        //!
        //! Check if the descriptor is a DVB private one.
        //! @return True if the descriptor is a DVB private one.
        //!
        bool isPrivateDescriptor() const { return pds() != PDS_NULL; }

        //!
        //! Get the private data specifier.
        //! @return The private data specifier or PDS_NULL if this is not a private descriptor.
        //!
        PDS pds() const { return did() >= 0x80 ? PDS((_edid >> 32) & 0xFFFFFFFF) : PDS(PDS_NULL); }

        //!
        //! Check if the descriptor is a DVB extension descriptor.
        //! @return True if the descriptor is a DVB extension descriptor.
        //!
        bool isExtensionDVB() const { return didExtDVB() != EDID_NULL; }

        //!
        //! Check if the descriptor is an MPEG extension descriptor.
        //! @return True if the descriptor is an MPEG extension descriptor.
        //!
        bool isExtensionMPEG() const { return didExtMPEG() != MPEG_EDID_NULL; }

        //!
        //! Get the DVB descriptor tag extension.
        //! @return The descriptor tag extension or EDID_NULL if this is not a DVB extension descriptor.
        //!
        DID didExtDVB() const { return did() == DID_DVB_EXTENSION ? DID((_edid >> 8) & 0xFF) : DID(EDID_NULL); }

        //!
        //! Get the MPEG descriptor tag extension.
        //! @return The descriptor tag extension or MPEG_EDID_NULL if this is not an MPEG extension descriptor.
        //!
        DID didExtMPEG() const { return did() == DID_MPEG_EXTENSION ? DID((_edid >> 8) & 0xFF) : DID(MPEG_EDID_NULL); }

        //!
        //! Check if the descriptor is table-specific.
        //! @return True if the descriptor is table-specific.
        //!
        bool isTableSpecific() const { return tableId() != TID_NULL; }

        //!
        //! Get the required table-id for a table-specific descriptor.
        //! @return The table id or TID_NULL if this is not a table-specific descriptor.
        //!
        TID tableId() const { return TID((_edid >> 16) & 0xFF); }

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object == @a e.
        //!
        bool operator==(const EDID& e) const { return _edid == e._edid; }
        TS_UNEQUAL_OPERATOR(EDID)

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object < @a e.
        //!
        bool operator<(const EDID& e) const { return _edid <  e._edid; }

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object <= @a e.
        //!
        bool operator<=(const EDID& e) const { return _edid <= e._edid; }

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object > @a e.
        //!
        bool operator>(const EDID& e) const { return _edid >  e._edid; }

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object >= @a e.
        //!
        bool operator>=(const EDID& e) const { return _edid >= e._edid; }
    };
}
