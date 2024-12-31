//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup mpeg
//!  Names of various MPEG entities.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNamesFile.h"
#include "tsCAS.h"

namespace ts {

    class DuckContext;

    //!
    //! Namespace for functions returning Digital TV names.
    //!
    namespace names {
        //!
        //! Name of service type (in Service Descriptor).
        //! @param [in] st Service type (in Service Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ServiceType(uint8_t st, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Running Status (in SDT).
        //! @param [in] rs Running Status (in SDT).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString RunningStatus(uint8_t rs, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of content name (in Content Descriptor).
        //! @param [in] duck TSDuck execution context (used to select from other standards).
        //! @param [in] c Content name.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString Content(const DuckContext& duck, uint8_t c, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Bouquet Id.
        //! @param [in] id Bouquet Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString BouquetId(uint16_t id, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Original Network Id.
        //! @param [in] id Original Network Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString OriginalNetworkId(uint16_t id, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Network Id.
        //! @param [in] id Network Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString NetworkId(uint16_t id, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Data broadcast id (in Data Broadcast Id Descriptor).
        //! @param [in] id Data broadcast id (in Data Broadcast Id Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DataBroadcastId(uint16_t id, NamesFlags flags = NamesFlags::NAME);
    }
}
