//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  "eXtension Descriptor Id" for extended descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDID.h"

namespace ts {
    //!
    //! eXtension Descriptor Id.
    //! @ingroup libtsduck mpeg
    //!
    //! Most descriptors are identified by an 8-bit tag or DID. MPEG and DVB use the concept of
    //! "extension descriptor", with one single DID value and several extension descriptor ids.
    //! The XDID class encapsulates the two values to create a 16-bits unique identifier.
    //!
    //! Note: this type is different from EDID. The XDID takes is intrinsicly linked to the
    //! descriptor content: the DID and the optional extension descriptor id. The EDID class
    //! is a wider concept which integrate contextual environments such as private descriptors
    //! or table-specific descriptor. The EDID is a superset of the XDID.
    //!
    class TSDUCKDLL XDID
    {
    private:
        uint16_t _xdid; // msb: did, lsb: extension descriptor id (or FF)
    public:
        //!
        //! Constructor.
        //! @param [in] did Descriptor id.
        //! @param [in] edid Extension descriptor id.
        //!
        explicit XDID(DID did = DID_NULL, DID edid = XDID_NULL) : _xdid(uint16_t(uint16_t(did) << 8) | edid) {}

        //! @cond nodoxygen
        auto operator<=>(const XDID&) const = default;
        //! @endcond

        //!
        //! Get the descriptor id.
        //! @return The descriptor id.
        //!
        DID did() const { return DID(_xdid >> 8); }

        //!
        //! Get the extension descriptor id.
        //! @return The extension descriptor id.
        //!
        DID xdid() const { return DID(_xdid & 0xFF); }

        //!
        //! Check if the XDID is an MPEG extension descriptor.
        //! @return True if the XDID is an MPEG extension descriptor.
        //!
        bool isExtensionMPEG() const { return did() == DID_MPEG_EXTENSION; }

        //!
        //! Check if the XDID is a DVB extension descriptor.
        //! @return True if the XDID is a DVB extension descriptor.
        //!
        bool isExtensionDVB() const { return did() == DID_DVB_EXTENSION; }

        //!
        //! Check if the XDID is any form of extension descriptor.
        //! @return True if the XDID is any form of extension descriptor.
        //!
        bool isExtension() const { return isExtensionMPEG() || isExtensionDVB(); }

        //!
        //! Convert to a string object.
        //! Note: The XDID class does not implement StringifyInterface because we don't want to
        //! make it virtual and keep the instance size small, without vtable pointer.
        //! @return This object, converted as a string.
        //!
        UString toString() const;
    };
}
