//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        CASSelectionArgs() = default;

        // Public fields, by options.
        bool      pass_ecm = false;  //!< Pass PIDs containing ECM.
        bool      pass_emm = false;  //!< Pass PIDs containing EMM.
        uint16_t  min_cas_id = 0;    //!< Minimum CA system id for ECM or EMM.
        uint16_t  max_cas_id = 0;    //!< Maximum CA system id for ECM or EMM.
        uint32_t  cas_oper = 0;      //!< CA operator id (depends on the CAS).

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
