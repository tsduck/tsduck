//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A base class to lookup the context of MPEG PSI/SI sections.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsCAS.h"
#include "tsStandards.h"

namespace ts {
    //!
    //! A base class to lookup the context of MPEG PSI/SI sections.
    //! @ingroup libtsduck mpeg
    //!
    //! The interface of the base class is used to understand the context of a section.
    //! When getting a binary section, for instance, it is not always easy to determine
    //! the true nature of the section, because the 8-bit table_id can be used by
    //! different types of sections. The SectionContext provides the PID, the standards,
    //! or CAS id which disambiguate the resolution.
    //!
    //! This class can be derived to provide dynamic ways of locating the context.
    //!
    class TSDUCKDLL SectionContext
    {
        TS_RULE_OF_FIVE(SectionContext, );
    private:
        PID _pid;
        CASID _casid;
        Standards _standards;
    public:
        //!
        //! Constructor.
        //! @param [in] pid Optional PID. This value is returned by the default implementation of getPID().
        //! @param [in] standards Optional set of standards. This value is returned by the default implementation of getStandards().
        //! @param [in] casid Optional Conditional Access System id. This value is returned by the default implementation of getCAS().
        //!
        SectionContext(PID pid = PID_NULL, Standards standards = Standards::NONE, CASID casid = CASID_NULL) :
            _pid(pid), _casid(casid), _standards(standards) {}

        //!
        //! Get the PID where the section is located.
        //! The default implementation returns the PID parameter which was given to the constructor.
        //! @return The PID of the section or PID_NULL if unknown.
        //!
        virtual PID getPID() const;

        //!
        //! Get the standards where the section is located.
        //! The default implementation returns the standards parameter which was given to the constructor.
        //! @return The standards where the section is located.
        //!
        virtual Standards getStandards() const;

        //!
        //! Get the Conditional Access System id in the context where the section is located.
        //! The default implementation returns the CASID parameter which was given to the constructor.
        //! @return The contextual CAS id or CASID_NULL if unknown.
        //!
        virtual CASID getCAS() const;
    };
}
