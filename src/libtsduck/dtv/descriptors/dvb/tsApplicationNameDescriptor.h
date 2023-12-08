//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an application_name_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractMultilingualDescriptor.h"

namespace ts {
    //!
    //! Representation of an application_name_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 101 812, 10.7.4.1.
    //! @see ETSI TS 102 809, 5.3.5.6.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ApplicationNameDescriptor : public AbstractMultilingualDescriptor
    {
    public:
        //!
        //! Default constructor.
        //!
        ApplicationNameDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ApplicationNameDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Virtual destructor.
        //!
        virtual ~ApplicationNameDescriptor() override;
    };
}
