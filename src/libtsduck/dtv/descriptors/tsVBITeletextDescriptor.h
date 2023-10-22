//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a VBI_teletext_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTeletextDescriptor.h"

namespace ts {
    //!
    //! Representation of a VBI_teletext_descriptor.
    //! This descriptor has the same structure as a teletext_descriptor.
    //! @see ETSI EN 300 468, 6.2.48.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL VBITeletextDescriptor : public TeletextDescriptor
    {
    public:
        //!
        //! Default constructor.
        //!
        VBITeletextDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        VBITeletextDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Virtual destructor
        //!
        virtual ~VBITeletextDescriptor() override;
    };
}
