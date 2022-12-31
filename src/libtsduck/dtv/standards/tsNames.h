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
//!  @ingroup mpeg
//!  Names of various MPEG entities.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNamesFile.h"
#include "tsPSI.h"

namespace ts {

    class DuckContext;

    //!
    //! Namespace for functions returning Digital TV names.
    //!
    namespace names {
        //!
        //! Name of Table ID.
        //! @param [in] duck TSDuck execution context (used to select from conflicting standards).
        //! @param [in] tid Table id.
        //! @param [in] cas CAS id for EMM/ECM table ids.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString TID(const DuckContext& duck, uint8_t tid, uint16_t cas = CASID_NULL, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Descriptor ID.
        //! @param [in] did Descriptor id.
        //! @param [in] pds Private data specified if @a did >= 0x80.
        //! @param [in] tid Optional id of the enclosing table.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DID(uint8_t did, uint32_t pds = 0, uint8_t tid = 0xFF, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Check if a descriptor id has a specific name for a given table.
        //! @param [in] did Descriptor id.
        //! @param [in] tid Table id of the enclosing table.
        //! @return True if descriptor @a did has a specific name for table @a tid.
        //!
        TSDUCKDLL bool HasTableSpecificName(uint8_t did, uint8_t tid);

        //!
        //! Name of Extended descriptor ID.
        //! @param [in] edid Extended descriptor ID.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString EDID(uint8_t edid, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Private Data Specifier.
        //! @param [in] pds Private Data Specifier.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString PrivateDataSpecifier(uint32_t pds, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Stream type (in PMT).
        //! @param [in] st Stream type (in PMT).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString StreamType(uint8_t st, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of service type (in Service Descriptor).
        //! @param [in] st Service type (in Service Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ServiceType(uint8_t st, NamesFlags flags = NamesFlags::NAME);

        //!
        //! Name of Conditional Access System Id (in CA Descriptor).
        //! @param [in] duck TSDuck execution context (used to select from other standards).
        //! @param [in] casid Conditional Access System Id (in CA Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString CASId(const DuckContext& duck, uint16_t casid, NamesFlags flags = NamesFlags::NAME);

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
