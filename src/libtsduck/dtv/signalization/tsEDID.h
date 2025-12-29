//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  "Extended Descriptor Id", a synthetic value for identifying descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTID.h"
#include "tsDID.h"
#include "tsXDID.h"
#include "tsPDS.h"
#include "tsREGID.h"
#include "tsStandards.h"

namespace ts {
    //!
    //! Extended MPEG descriptor id.
    //! @ingroup libtsduck mpeg
    //!
    //! For convenience, it is sometimes useful to identify descriptors using
    //! an "extended DID" because a descriptor tag is not always enough to
    //! uniquely identify a descriptor.
    //!
    //! A descriptor usually falls into one of the following categories:
    //!
    //! - Regular descriptor (tag in the range 0x00-0x3F for MPEG, 0x40-7F for DVB,
    //!   unspecified for other standards). This descriptor is usually associated to
    //!   a standard (MPEG, DVB, ATSC, ISDB, etc.)
    //! - Private MPEG descriptor (unofficial name). Must be associated with a 32-bit
    //!   registration id, or REGID, in a preceding MPEG registration_descriptor. This
    //!   descriptor can be in the same descriptor list or a higher descriptor list.
    //! - Private DVB descriptor (tag in the range 0x80-0xFF). Must be associated with
    //!   a 32-bit private data specifier or PDS, in a preceding private_data_specifier
    //!   descriptor in the same descriptor list.
    //! - Dual private descriptor: Can be used either as private MPEG or private DVB
    //!   descriptor. The same 32-bit value is used as MPEG registration id and DVB
    //!   private data specifier.
    //! - MPEG or DVB extension descriptor (tag == 0x3F or 0x7F). Must be
    //!   associated with a 8-bit tag extension.
    //! - Table-specific descriptor. Its tag reuses a tag which is otherwise used by some
    //!   regular descriptor, typically in the MPEG-defined range 0x00-0x3F. Such a descriptor
    //!   can only be found in one or more specific tables such as AIT, INT or UNT (DVB). In
    //!   these tables, the descriptor tag is interpreted as this table-specific descriptor.
    //!   Of course, the regular descriptor which normally uses this tag value cannot be
    //!   present in these tables. The descriptor tag must be associated with an 8-bit table id
    //!   (and the associated standard). If the table-specific descriptor is used in several
    //!   tables, up to 4 different table ids can be set in the EDID. Table-specific descriptors
    //!   for more than 4 tables are not supported (in practice, the maximum is 2 tables).
    //!
    //! The "extended DID" or EDID is a 64-bit value as used in the file tsDID.names.
    //! The C++ class EDID encapsulates this value, nothing more.
    //!
    //! The EDID 64-bit value is structured as follow: 0xSSSSTTRRRRRRRRDD.
    //!
    //! - 0xSSSS, 16 bits: Mask of associated standards. See enum class Standards.
    //! - 0xTT, 8 bits: Descriptor type. The possible values are:
    //!   - 0x00: Regular descriptor for the specified standard.
    //!   - 0x01: Private descriptor, needs a registration id or private data specifier.
    //!   - 0x02: Extension descriptor. The descriptor tag must be 0x3F (MPEG) or 0x7F (DVB).
    //!   - 0x03: Table-specific descriptor.
    //!   - 0xFF: Invalid.
    //! - 0xRRRRRRRR, 32 bits: Registration value. It depends on the descriptor type.
    //!   - 0x00: Unused, registration value = 0xFFFFFFFF.
    //!   - 0x01: Registration value = 32-bit REGID and/or PDS.
    //!   - 0x02: Registration value = 0xFFFFFFVV where 0xVV is the extended descriptor tag.
    //!   - 0x03: Registration value = 0xYYXXWWVV where 0xVV, 0xWW, 0xXX, 0xYY are the parent
    //!           table ids, in increasing order. The unused table id values are set to 0xFF.
    //!   - other: Unused, registration value = 0xFFFFFFFF.
    //! - 0xDD, 8 bits: Descriptor id or tag.
    //!
    class TSDUCKDLL EDID
    {
    private:
        uint64_t _edid = 0xFFFFFFFFFFFFFFFF;
    public:
        //!
        //! Default constructor.
        //! The initial value is invalid.
        //!
        EDID() = default;

        //!
        //! Constructor from a 64-bit EDID value.
        //! @param [in] edid 64-bit EDID value.
        //!
        explicit constexpr EDID(uint64_t edid) : _edid(edid) {}

        //! @cond nodoxygen
        auto operator<=>(const EDID&) const = default;
        //! @endcond

        //!
        //! Enumeration of descriptor types.
        //!
        enum class Type : uint8_t {
            REGULAR    = 0x00,  //!< Regular descriptor for the specified standard.
            PRIVATE    = 0x01,  //!< Private MPEG or DVB descriptor, needs a REGID or PDS.
            EXTENDED   = 0x02,  //!< MPEG or DVB extension descriptor. The descriptor tag must be 0x3F or 0x7F.
            TABLE_SPEC = 0x03,  //!< Table-specific descriptor.
            INVALID    = 0xFF,  //!< Invalid.
        };

        //!
        //! Build the EDID for a regular descriptor.
        //! @param [in] did Descriptor tag.
        //! @param [in] std Relevant standards.
        //! @return The corresponding EDID.
        //!
        static constexpr EDID Regular(DID did, Standards std)
        {
            return EDID(0x000000FFFFFFFF00 |
                        (uint64_t(Type::REGULAR) << 40) |
                        (uint64_t(std) << 48) |
                        (did & 0xFF));
        }

        //!
        //! Build the EDID for a private MPEG descriptor.
        //! @param [in] did Descriptor tag.
        //! @param [in] regid Associated registration id.
        //! @return The corresponding EDID.
        //!
        static constexpr EDID PrivateMPEG(DID did, REGID regid)
        {
            return EDID((uint64_t(Type::PRIVATE) << 40) |
                        (uint64_t(Standards::MPEG) << 48) |
                        (uint64_t(regid) << 8) |
                        (did & 0xFF));
        }

        //!
        //! Build the EDID for a private DVB descriptor.
        //! @param [in] did Descriptor tag.
        //! @param [in] pds Associated private data specifier.
        //! @return The corresponding EDID.
        //!
        static constexpr EDID PrivateDVB(DID did, PDS pds)
        {
            return EDID((uint64_t(Type::PRIVATE) << 40) |
                        (uint64_t(Standards::DVB) << 48) |
                        (uint64_t(pds) << 8) |
                        (did & 0xFF));
        }

        //!
        //! Build the EDID for a dual private descriptor.
        //! This kind of descriptor can be used in a DVB context using a private data specifier descriptor
        //! or in a simple MPEG context using a registration descriptor. In both cases, the value of the
        //! private data specifier and the registration id are identical.
        //! @param [in] did Descriptor tag.
        //! @param [in] pds Associated registration id and private data specifier.
        //! @return The corresponding EDID.
        //!
        static constexpr EDID PrivateDual(DID did, PDS pds)
        {
            return EDID((uint64_t(Type::PRIVATE) << 40) |
                        (uint64_t(Standards::MPEG | Standards::DVB) << 48) |
                        (uint64_t(pds) << 8) |
                        (did & 0xFF));
        }

        //!
        //! Build the EDID for an MPEG extension descriptor.
        //! @param [in] ext Associated tag extension. The descriptor tag is implicitly DID_MPEG_EXTENSION.
        //! @return The corresponding EDID.
        //!
        static constexpr EDID ExtensionMPEG(DID ext)
        {
            return EDID(0x000000FFFFFF0000 |
                        (uint64_t(Type::EXTENDED) << 40) |
                        (uint64_t(Standards::MPEG) << 48) |
                        (uint64_t(ext) << 8) |
                        uint64_t(DID_MPEG_EXTENSION));
        }

        //!
        //! Build the EDID for a DVB extension descriptor.
        //! @param [in] ext Associated tag extension. The descriptor tag is implicitly DID_DVB_EXTENSION.
        //! @return The corresponding EDID.
        //!
        static constexpr EDID ExtensionDVB(DID ext)
        {
            return EDID(0x000000FFFFFF0000 |
                        (uint64_t(Type::EXTENDED) << 40) |
                        (uint64_t(Standards::DVB) << 48) |
                        (uint64_t(ext) << 8) |
                        uint64_t(DID_DVB_EXTENSION));
        }

        //!
        //! Build the EDID for an MPEG or DVB extension descriptor.
        //! @param [in] xdid Extension descriptor id. If this is not an extension descriptor, create
        //! a regular descriptor EDID without standard.
        //! @return The corresponding EDID.
        //!
        static EDID Extension(XDID xdid);

        //!
        //! Build the EDID for a table-specific descriptor.
        //! @param [in] did Descriptor tag.
        //! @param [in] std Relevant standards for the table.
        //! @param [in] tid1 Associated required table id.
        //! @param [in] tid2 Optional second table id.
        //! @param [in] tid3 Optional third table id.
        //! @param [in] tid4 Optional fourth table id.
        //! @return The corresponding EDID.
        //!
        static EDID TableSpecific(DID did, Standards std, TID tid1, TID tid2 = TID_NULL, TID tid3 = TID_NULL, TID tid4 = TID_NULL);

        //!
        //! Check if the extended descriptor id is valid.
        //! @return True if valid.
        //!
        bool isValid() const { return did() != 0xFF; }

        //!
        //! Get the descriptor type.
        //! @return The descriptor type.
        //!
        Type type() const { return Type((_edid >> 40) & 0xFF); }

        //!
        //! Get the associated standards.
        //! @return The mask of associated standards.
        //!
        Standards standards() const { return Standards(_edid >> 48); }

        //!
        //! Check if the descriptor is a regular one.
        //! @return True if the descriptor is a standard one.
        //!
        bool isRegular() const { return type() == Type::REGULAR; }

        //!
        //! Get the descriptor id (aka tag).
        //! @return The descriptor id.
        //!
        DID did() const { return DID(_edid & 0xFF); }

        //!
        //! Check if the descriptor is a MPEG or dual private descriptor.
        //! @return True if the descriptor is a MPEG or dual private descriptor.
        //!
        bool isPrivateMPEG() const { return type() == Type::PRIVATE && bool(standards() & Standards::MPEG); }

        //!
        //! Check if the descriptor is a DVB or dual private descriptor.
        //! @return True if the descriptor is a DVB or dual private descriptor.
        //!
        bool isPrivateDVB() const { return type() == Type::PRIVATE && bool(standards() & Standards::DVB); }

        //!
        //! Check if the descriptor is a dual private descriptor (can be used as MPEG or DVB private descriptor).
        //! @return True if the descriptor is a dual private descriptor.
        //!
        bool isPrivateDual() const { return type() == Type::PRIVATE && (standards() & (Standards::MPEG | Standards::DVB)) == (Standards::MPEG | Standards::DVB); }

        //!
        //! Get the MPEG registration identifier.
        //! @return The MPEG registration identifier or REGID_NULL if this is not a private descriptor.
        //!
        REGID regid() const { return isPrivateMPEG() ? REGID(_edid >> 8) : REGID(REGID_NULL); }

        //!
        //! Get the DVB private data specifier.
        //! @return The DVB private data specifier or PDS_NULL if this is not a private descriptor.
        //!
        PDS pds() const { return isPrivateDVB() ? PDS(_edid >> 8) : PDS(PDS_NULL); }

        //!
        //! Get the MPEG registration identifier or DVB private data specifier.
        //! @return The REGID/PDS or REGID_NULL if this is not a private descriptor.
        //!
        REGID privateId() const { return type() == Type::PRIVATE ? REGID(_edid >> 8) : REGID(REGID_NULL); }

        //!
        //! Check if the descriptor is an MPEG extension descriptor.
        //! @return True if the descriptor is an MPEG extension descriptor.
        //!
        bool isExtensionMPEG() const { return type() == Type::EXTENDED && bool(standards() & Standards::MPEG); }

        //!
        //! Check if the descriptor is a DVB extension descriptor.
        //! @return True if the descriptor is a DVB extension descriptor.
        //!
        bool isExtensionDVB() const { return type() == Type::EXTENDED && bool(standards() & Standards::DVB); }

        //!
        //! Check if the descriptor is an MPEG or DVB extension descriptor.
        //! @return True if the descriptor is an MPEG or DVB extension descriptor.
        //!
        bool isExtension() const { return type() == Type::EXTENDED; }

        //!
        //! Get the MPEG descriptor tag extension.
        //! @return The descriptor tag extension or XDID_NULL if this is not an MPEG extension descriptor.
        //!
        DID didExtMPEG() const { return did() == DID_MPEG_EXTENSION ? DID((_edid >> 8) & 0xFF) : DID(XDID_NULL); }

        //!
        //! Get the DVB descriptor tag extension.
        //! @return The descriptor tag extension or XDID_NULL if this is not a DVB extension descriptor.
        //!
        DID didExtDVB() const { return did() == DID_DVB_EXTENSION ? DID((_edid >> 8) & 0xFF) : DID(XDID_NULL); }

        //!
        //! Get the MPEG or DVB descriptor tag extension.
        //! @return The descriptor tag extension or XDID_NULL if this is not an extension descriptor.
        //!
        DID didExtension() const { return type() == Type::EXTENDED ? DID((_edid >> 8) & 0xFF) : DID(XDID_NULL); }

        //!
        //! Check if the descriptor is table-specific.
        //! @return True if the descriptor is table-specific.
        //!
        bool isTableSpecific() const { return type() == Type::TABLE_SPEC; }

        //!
        //! Get the required table-ids for a table-specific descriptor.
        //! @return The set of possible table ids, empty if this is not a table-specific descriptor.
        //!
        std::set<TID> tableIds() const;

        //!
        //! Check if the descriptor is table-specific and matches a given table id.
        //! @param [in] tid A table id to test.
        //! @param [in] std Relevant standards.
        //! @return True if the descriptor is table-specific for @a tid.
        //!
        bool matchTableSpecific(TID tid, Standards std) const;

        //!
        //! Check if the descriptor is a regular one and matches at least one standard.
        //! @param [in] std Relevant standards to test.
        //! @return True if the descriptor is a regular one and matches at least one standard in @a std.
        //! If the regular descriptor has declared no standard, then it matches by default.
        //! If the regular descriptor has declared standards but none of them are in @a std, return
        //! true if all its standards are compatible with @a std. This is the way we discover new
        //! standards in the transport stream.
        //!
        bool matchRegularStandards(Standards std) const;

        //!
        //! Build an eXtension Descriptor Id from the EDID.
        //! @return The corresponding eXtension Descriptor Id.
        //!
        XDID xdid() const;

        //!
        //! Get the 64-bit encoded EDID value.
        //! There is not real usage for this in an application, except looking up names into the DTV names file.
        //! @return The 64-bit encoded EDID value.
        //!
        uint64_t encoded() const { return _edid; }

        //!
        //! Convert to a string object.
        //! Note: The EDID class does not implement StringifyInterface because we don't want to
        //! make it virtual and keep the instance size small, without vtable pointer.
        //! @return This object, converted as a string.
        //!
        UString toString() const;
    };
}
