//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DTG logical_channel_descriptor.
//!  This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLogicalChannelDescriptor.h"

namespace ts {
    //!
    //! Representation of a DTG logical_channel_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
    //! @see The D-Book 7 Part A (DTG), section 8.5.3.6
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DTGLogicalChannelDescriptor : public AbstractLogicalChannelDescriptor
    {
    public:
        //!
        //! Default constructor.
        //!
        DTGLogicalChannelDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DTGLogicalChannelDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Destructor.
        //!
        virtual ~DTGLogicalChannelDescriptor() override;
    };
}
