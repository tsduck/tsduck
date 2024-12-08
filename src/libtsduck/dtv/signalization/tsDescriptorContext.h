//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A base class to lookup the context of MPEG PSI/SI descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsREGID.h"
#include "tsTID.h"
#include "tsPDS.h"
#include "tsCAS.h"
#include "tsStandards.h"

namespace ts {
    //!
    //! A base class to lookup the context of MPEG PSI/SI descriptors.
    //! @ingroup mpeg
    //!
    //! The interface of the base class is used to understand the context of a descriptor.
    //! When getting a binary descriptor, for instance, it is not always easy to determine
    //! the true nature of the descriptor, because the 8-bit descriptor tag can be used by
    //! different types of descriptors. The DescriptorContext provides the table id, the
    //! standards, CAS id, private specifier, or registration id which disambiguate the
    //! resolution.
    //!
    //! This class can be derived to provide dynamic ways of locating the current private
    //! specifier or registration id for instance.
    //!
    class TSDUCKDLL DescriptorContext
    {
        TS_RULE_OF_FIVE(DescriptorContext, );
    private:
        TID _tid;
        CASID _casid;
        Standards _standards;
        REGID _regid;
        PDS _pds;
    public:
        //!
        //! Constructor.
        //! @param [in] tid Optional table id. This value is returned by the default implementation of getTableId().
        //! @param [in] standards Optional set of standards. This value is returned by the default implementation of getStandards().
        //! @param [in] casid Optional Conditional Access System id. This value is returned by the default implementation of getCAS().
        //! @param [in] regid Optional registration id. This value is returned by the default implementation of getPrivateIds().
        //! @param [in] pds Optional DVB private data specifier. This value is returned by the default implementation of getPrivateIds().
        //!
        DescriptorContext(TID tid = TID_NULL, Standards standards = Standards::NONE, CASID casid = CASID_NULL, REGID regid = REGID_NULL, PDS pds = PDS_NULL) :
            _tid(tid), _casid(casid), _standards(standards), _regid(regid), _pds(pds) {}

        //!
        //! Get the table id of the table where the descriptor is located.
        //! The default implementation returns the TID parameter which was given to the constructor.
        //! @return The parent table id or TID_NULL if unknown.
        //!
        virtual TID getTableId() const;

        //!
        //! Get the standards where the descriptor is located.
        //! The default implementation returns the standards parameter which was given to the constructor.
        //! @return The standards where the descriptor is located.
        //!
        virtual Standards getStandards() const;

        //!
        //! Get the Conditional Access System id in the context where the descriptor is located.
        //! The default implementation returns the CASID parameter which was given to the constructor.
        //! @return The contextual CAS id or CASID_NULL if unknown.
        //!
        virtual CASID getCAS() const;

        //!
        //! Lookup registration identifiers for private descriptors.
        //!
        //! This virtual method searches the closest MPEG registration id and/or DVB private data specifier,
        //! starting from a position which is defined by the object implementing this interface.
        //!
        //! The idea of this interface is to save time and perform the search for private identifiers when
        //! necessary only.
        //!
        //! The default implementation searches nothing and returns the REGID and PDS values which were given to the constructor.
        //!
        //! @param [out] regid If non-null, the pointed REGID will receive the closest MPEG registration id
        //! or REGID_NULL when none if found. In the case of descriptor lists, registration ids are searched
        //! first in the current descriptor list, and then in the "top-level" descriptor list of the table,
        //! if there is one. If @a regid is null, no search for MPEG registration id is performed.
        //! @param [out] pds If non-null, the pointed PDS will receive the closest DVB private data specifier
        //! or PDS_NULL when none if found. In the case of descriptor lists, PDS are searched in the current
        //! descriptor list only. If @a pds is null, no search for DVB PDS is performed
        //! @return True when @a regid and @a pds are non-null, and a valid value was found for the two, and
        //! the MPEG registration id was closer than the DVB private data specifier. Return false in all other
        //! cases.
        //!
        virtual bool getPrivateIds(REGID* regid, PDS* pds) const;
    };
}
