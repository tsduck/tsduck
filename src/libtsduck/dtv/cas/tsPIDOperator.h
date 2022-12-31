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
//!  Association of a PID and an "operator" id (CAS-specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Association of a PID and an @e operator id (CAS-specific).
    //! @ingroup mpeg
    //!
    struct TSDUCKDLL PIDOperator
    {
        // Public members
        PID      pid;     //!< ECM/EMM PID
        bool     is_emm;  //!< True for EMM PID, false for ECM PID.
        uint16_t cas_id;  //!< CAS identifier.
        uint32_t oper;    //!< Operator id

        //!
        //! Constructor.
        //! @param [in] pid_ PID value.
        //! @param [in] is_emm_ True for EMM PID, false for ECM PID.
        //! @param [in] cas_id_ CAS identifier.
        //! @param [in] oper_ CAS-specific operator id.
        //!
        PIDOperator(PID pid_ = 0, bool is_emm_ = false, uint16_t cas_id_ = 0, uint32_t oper_ = 0);

        //!
        //! Comparison operator.
        //! Not really meaningfull but required to use this class in containers.
        //! @param [in] po Other pid/operator instance to compare.
        //! @return True if this object logically preceeds @a po.
        //!
        bool operator<(const PIDOperator& po) const;
    };

    //!
    //! Specialized set of PIDOperator.
    //!
    class TSDUCKDLL PIDOperatorSet: public std::set<PIDOperator>
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef std::set<PIDOperator> SuperClass;

        //!
        //! Default constructor.
        //!
        PIDOperatorSet() :
            SuperClass()
        {
        }

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        PIDOperatorSet(const PIDOperatorSet& other) :
            SuperClass(other)
        {
        }

        //!
        //! Search first ECM/EMM PID for a specific operator.
        //! @param [in] oper Operator id.
        //! @return The PID or 0 if not found.
        //!
        PID pidForOper(uint32_t oper) const;

        //!
        //! Add all known operator info from a list of descriptors from a CAT or PMT.
        //! @param [in] dlist Descriptor list.
        //! @param [in] is_cat True is @a dlist is taken from a CAT, false for a PMT.
        //!
        void addAllOperators(const DescriptorList& dlist, bool is_cat);

        //!
        //! Add MediaGuard info from a list of descriptors from a PMT.
        //! @param [in] dlist Descriptor list.
        //!
        void addMediaGuardPMT(const DescriptorList& dlist);

        //!
        //! Add MediaGuard info from a list of descriptors from a CAT.
        //! @param [in] dlist Descriptor list.
        //!
        void addMediaGuardCAT(const DescriptorList& dlist);

        //!
        //! Add SafeAccess info from a list of descriptors from a CAT.
        //! @param [in] dlist Descriptor list.
        //!
        void addSafeAccessCAT(const DescriptorList& dlist);

        //!
        //! Add Viaccess info from a list of descriptors from a CAT or PMT.
        //! @param [in] dlist Descriptor list.
        //! @param [in] is_cat True is @a dlist is taken from a CAT, false for a PMT.
        //!
        void addViaccess(const DescriptorList& dlist, bool is_cat);
    };
}
