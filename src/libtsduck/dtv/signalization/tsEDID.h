//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
    //! - Standard descriptor (tag in the range 0x00-0x3F for MPEG, 0x40-7F for DVB,
    //!   unspecified for other standards).
    //! - Private MPEG descriptor (unofficial name). Must be associated with a 32-bit
    //!   registration id, or REGID, in a preceding MPEG registration_descriptor. This
    //!   descriptor can be in the same descriptor list or a higher descriptor list.
    //! - Private DVB descriptor (tag in the range 0x80-0xFF). Must be associated with
    //!   a 32-bit private data specifier or PDS, in a preceding private_data_specifier
    //!   descriptor in the same descriptor list.
    //! - MPEG or DVB extension descriptor (tag == 0x3F or 0x7F). Must be
    //!   associated with a 8-bit tag extension.
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
        // - 32 bits: context-id, PDS or REGID (or PDS_NULL)
        // - 1 bit: 0 = context-id is a REGID (MPEG-private), 1 = not a REGID
        // - 1 bit: 0 = context-id is a PDS (DVB-private), 1 = not a PDS
        // - 6 bits: unused (0x3F)
        // - 8 bits: table-id for table-specific descriptors (or TID_NULL)
        // - 8 bits: tag extension (or EDID_NULL)
        // - 8 bits: descriptor tag (DID)
        static constexpr uint64_t HAS_REGID = 0x0000000080000000;
        static constexpr uint64_t HAS_PDS   = 0x0000000040000000;

        // The 64-bit extended id. This is the only instance field, an EDID is just an encapsulated uint64_t.
        uint64_t _edid = 0xFFFFFFFFFFFFFFFF;

        // Private constructor from a 64-bit value.
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
        //! Build the EDID for a private MPEG descriptor.
        //! @param [in] did Descriptor tag.
        //! @param [in] regid Associated registration id.
        //! @return The corresponding EDID.
        //!
        static EDID PrivateMPEG(DID did, PDS regid) { return EDID((uint64_t(regid) << 32) | (0x00000000FFFFFF00 & ~HAS_REGID) | (did & 0xFF)); }

        //!
        //! Build the EDID for a private DVB descriptor.
        //! @param [in] did Descriptor tag.
        //! @param [in] pds Associated private data specifier.
        //! @return The corresponding EDID.
        //!
        static EDID PrivateDVB(DID did, PDS pds) { return EDID((uint64_t(pds) << 32) | (0x00000000FFFFFF00 & ~HAS_PDS) | (did & 0xFF)); }

        //!
        //! Build the EDID for a dual private descriptor.
        //! This kind of descriptor can be used in a DVB context using a private data specifier descriptor
        //! or in a simple MPEG context using a registration descriptor. In both cases, the value of the
        //! private data specifier and the registration id are identical.
        //! @param [in] did Descriptor tag.
        //! @param [in] pds Associated registration id and private data specifier.
        //! @return The corresponding EDID.
        //!
        static EDID PrivateDual(DID did, PDS pds) { return EDID((uint64_t(pds) << 32) | (0x00000000FFFFFF00 & ~(HAS_REGID | HAS_PDS)) | (did & 0xFF)); }

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
        //! Check if the descriptor is a MPEG or dual private descriptor.
        //! @return True if the descriptor is a MPEG or dual private descriptor.
        //!
        bool isPrivateMPEG() const { return (_edid & HAS_REGID) == 0; }

        //!
        //! Check if the descriptor is a DVB or dual private descriptor.
        //! @return True if the descriptor is a DVB or dual private descriptor.
        //!
        bool isPrivateDVB() const { return (_edid & HAS_PDS) == 0; }

        //!
        //! Check if the descriptor is a dual private descriptor (used as MPEG or DVB private descriptor).
        //! @return True if the descriptor is a dual private descriptor.
        //!
        bool isPrivateDual() const { return (_edid & (HAS_REGID | HAS_PDS)) == 0; }

        //!
        //! Get the MPEG registration identifier.
        //! @return The MPEG registration identifier or REGID_NULL if this is not a private descriptor.
        //!
        PDS regid() const { return (_edid & HAS_REGID) ? PDS(REGID_NULL) : PDS((_edid >> 32) & 0xFFFFFFFF); }

        //!
        //! Get the DVB private data specifier.
        //! @return The DVB private data specifier or PDS_NULL if this is not a private descriptor.
        //!
        PDS pds() const { return (_edid & HAS_PDS) ? PDS(PDS_NULL) : PDS((_edid >> 32) & 0xFFFFFFFF); }

        //!
        //! Check if the descriptor is a DVB extension descriptor.
        //! @return True if the descriptor is a DVB extension descriptor.
        //!
        bool isExtensionDVB() const { return didExtDVB() != EDID_NULL; }

        //!
        //! Check if the descriptor is an MPEG extension descriptor.
        //! @return True if the descriptor is an MPEG extension descriptor.
        //!
        bool isExtensionMPEG() const { return didExtMPEG() != EDID_NULL; }

        //!
        //! Get the DVB descriptor tag extension.
        //! @return The descriptor tag extension or EDID_NULL if this is not a DVB extension descriptor.
        //!
        DID didExtDVB() const { return did() == DID_DVB_EXTENSION ? DID((_edid >> 8) & 0xFF) : DID(EDID_NULL); }

        //!
        //! Get the MPEG descriptor tag extension.
        //! @return The descriptor tag extension or EDID_NULL if this is not an MPEG extension descriptor.
        //!
        DID didExtMPEG() const { return did() == DID_MPEG_EXTENSION ? DID((_edid >> 8) & 0xFF) : DID(EDID_NULL); }

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
