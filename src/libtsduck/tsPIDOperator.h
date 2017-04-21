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
//!  Association of a PID and an "operator" id (CAS-specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDescriptorList.h"

namespace ts {

    struct TSDUCKDLL PIDOperator
    {
        // Public members
        PID    pid;    // ECM/EMM PID
        uint32_t oper;   // Operator id

        // Constructor
        PIDOperator (PID pid_ = 0, uint32_t oper_ = 0) : pid (pid_), oper (oper_) {}

        // Comparison operator for containers
        bool operator< (const PIDOperator& po) const {return oper == po.oper ? pid < po.pid : oper < po.oper;}
    };

    class TSDUCKDLL PIDOperatorSet: public std::set<PIDOperator>
    {
    public:
        typedef std::set<PIDOperator> SuperClass;

        // Constructors
        PIDOperatorSet () : SuperClass() {}
        PIDOperatorSet (const PIDOperatorSet& set) : SuperClass (set) {}

        // Search first ECM/EMM PID for a specific operator, return O if not found
        PID pidForOper (uint32_t oper) const;

        // Add MediaGuard info from a list of descriptors from a PMT
        void addMediaGuardPMT (const DescriptorList& dlist);

        // Add MediaGuard info from a list of descriptors from a CAT
        void addMediaGuardCAT (const DescriptorList& dlist);

        // Add SafeAccess info from a list of descriptors from a CAT
        void addSafeAccessCAT (const DescriptorList& dlist);
    };
}
