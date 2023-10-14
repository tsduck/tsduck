//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        PIDOperatorSet() = default;

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        PIDOperatorSet(const PIDOperatorSet& other) = default;

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
