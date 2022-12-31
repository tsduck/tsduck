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
//!  Command line arguments to select CAS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDescriptorList.h"
#include "tsNullReport.h"
#include "tsCAT.h"
#include "tsPMT.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Command line arguments to select Conditional Access Systems.
    //! @ingroup cmd
    //!
    class TSDUCKDLL CASSelectionArgs
    {
    public:
        //!
        //! Constructor.
        //!
        CASSelectionArgs();

        // Public fields, by options.
        bool      pass_ecm;    //!< Pass PIDs containing ECM.
        bool      pass_emm;    //!< Pass PIDs containing EMM.
        uint16_t  min_cas_id;  //!< Minimum CA system id for ECM or EMM.
        uint16_t  max_cas_id;  //!< Maximum CA system id for ECM or EMM.
        uint32_t  cas_oper;    //!< CA operator id (depends on the CAS).

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);

        //!
        //! Check if the specified CAS id matches the selection criteria.
        //! @param [in] cas A CAS id.
        //! @return True if @a cas matches the selection criteria.
        //!
        bool casMatch(uint16_t cas) const;

        //!
        //! Check if the specified operator id matches the selection criteria.
        //! @param [in] oper An operator id.
        //! @return True if @a oper matches the selection criteria.
        //!
        bool operatorMatch(uint32_t oper) const;

        //!
        //! Analyze all CA_descriptors in a descriptor list and locate all matching PID's.
        //! @param [in,out] pids All patching PID's are added in this PID set.
        //! @param [in] dlist A list of descriptors.
        //! @param [in] tid Table id of the table from which the descriptor comes.
        //! @param [in,out] report Where to log debug messages.
        //! @return The number of matching PID's. Note that some of them may have been
        //! already in @a pids, so this may not be the number of @e added PID's.
        //!
        size_t addMatchingPIDs(PIDSet& pids, const DescriptorList& dlist, TID tid, Report& report = NULLREP) const;

        //!
        //! Analyze all CA_descriptors in a CAT and locate all matching EMM PID's.
        //! @param [in,out] pids All patching PID's are added in this PID set.
        //! @param [in] cat A CAT.
        //! @param [in,out] report Where to log debug messages.
        //! @return The number of matching PID's. Note that some of them may have been
        //! already in @a pids, so this may not be the number of @e added PID's.
        //!
        size_t addMatchingPIDs(PIDSet& pids, const CAT& cat, Report& report = NULLREP) const;

        //!
        //! Analyze all CA_descriptors in a PMT and locate all matching ECM PID's.
        //! @param [in,out] pids All patching PID's are added in this PID set.
        //! @param [in] pmt a PMT.
        //! @param [in,out] report Where to log debug messages.
        //! @return The number of matching PID's. Note that some of them may have been
        //! already in @a pids, so this may not be the number of @e added PID's.
        //!
        size_t addMatchingPIDs(PIDSet& pids, const PMT& pmt, Report& report = NULLREP) const;

    private:
        // List of predefined known CAS:
        struct PredefinedCAS {
            const UChar* name;
            uint16_t     min;
            uint16_t     max;
        };
        static const std::vector<PredefinedCAS> _predefined_cas;
    };
}
