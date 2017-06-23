//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Abstract base class for DVB delivery system descriptors
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTunerParameters.h"
#include "tsMPEG.h"

namespace ts {

    //!
    //! Abstract base class for DVB delivery system descriptors.
    //!
    class TSDUCKDLL AbstractDeliverySystemDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Get the delivery system.
        //! @return The delivery system.
        //!
        DeliverySystem deliverySystem() const {return _system;}

        //!
        //! Fill a tuner parameters object with data from the descriptor.
        //! @param [in,out] parameters Address of a tuner parameter.
        //! The parameters are updated with data from the descriptor.
        //! Other fields are left unmodified.
        //! @return True on success, false on error (invalid descriptor,
        //! incorrect type of tuner parameters).
        //!
        virtual bool toTunerParameters(TunerParameters* parameters) const = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractDeliverySystemDescriptor() {}

    protected:
        //!
        //! The delivery system can be modified by subclasses only
        //!
        DeliverySystem _system;

        //!
        //! Protected constructor for subclasses.
        //! @param [in] tag Descriptor tag.
        //!
        AbstractDeliverySystemDescriptor(DID tag, DeliverySystem system);

    private:
        // Unreachable constructors and operators.
        AbstractDeliverySystemDescriptor() = delete;
    };
}
