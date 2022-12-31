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
//!  Representation of a generic CA_descriptor.
//!  Specialized classes exist, depending on the CA_system_id.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsSafePtr.h"
#include "tsMutex.h"
#include "tsNullReport.h"
#include "tsTS.h"

namespace ts {

    class DescriptorList;

    //!
    //! Representation of a generic CA_descriptor.
    //! Specialized classes may exist, depending on the CA_system_id.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.16.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CADescriptor : public AbstractDescriptor
    {
    public:
        // CADescriptor public members:
        uint16_t  cas_id;        //!< CA system id.
        PID       ca_pid;        //!< PID for CA tables (ECM or EMM).
        ByteBlock private_data;  //!< CA-specific private data.

        //!
        //! Default constructor.
        //! @param [in] cas_id CA system id.
        //! @param [in] ca_pid PID for CA tables (ECM or EMM).
        //!
        CADescriptor(uint16_t cas_id = 0, PID ca_pid = PID_NULL);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CADescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Decode a command-line CA_descriptor and fills this object with it.
        //! @param [in] value CA descriptor in command-line form: casid/pid[/private-data]
        //! The mandatory parts, casid and pid, are integer values, either decimal or hexadecimal.
        //! The optional private data must be a suite of hexadecimal digits.
        //! @param [in,out] report Where to report errors (typically badly formed parameters).
        //! @return True on success, false on error.
        //!
        bool fromCommmandLine(const UString& value, Report& report = NULLREP);

        //!
        //! Static method to decode command-line CA_descriptor and add them in a descriptor list.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside this object.
        //! @param [in,out] dlist Descriptor list. The new CA descriptors are added in the list.
        //! @param [in] values List of CA descriptors in command-line form: casid/pid[/private-data]
        //! @return True on success, false on error.
        //! @see fromCommmandLine()
        //!
        static bool AddFromCommandLine(DuckContext& duck, DescriptorList& dlist, const UStringVector& values);

        //!
        //! Static method to search a CA_descriptor by ECM/EMM PID in a descriptor list.
        //! @param [in] dlist Descriptor list to search
        //! @param [in] pid ECM/EMM PID to search.
        //! @param [in] start_index Start searching at this index.
        //! @return The index of the descriptor in the list or its count() if no such descriptor is found.
        //!
        static size_t SearchByPID(const DescriptorList& dlist, PID pid, size_t start_index = 0);

        //!
        //! Static method to search a CA_descriptor by CA system id in a descriptor list.
        //! @param [in] dlist Descriptor list to search
        //! @param [in] casid CA system id to search.
        //! @param [in] start_index Start searching at this index.
        //! @return The index of the descriptor in the list or its count() if no such descriptor is found.
        //!
        static size_t SearchByCAS(const DescriptorList& dlist, uint16_t casid, size_t start_index = 0);

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };

    //!
    //! Safe pointer to a CADescriptor (thread-safe).
    //!
    typedef SafePtr<CADescriptor,Mutex> CADescriptorPtr;
}
