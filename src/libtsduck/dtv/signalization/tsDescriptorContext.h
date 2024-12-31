//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A class to lookup the context of MPEG PSI/SI descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsREGID.h"
#include "tsTID.h"
#include "tsPDS.h"
#include "tsCAS.h"
#include "tsStandards.h"

namespace ts {

    class DuckContext;
    class DescriptorList;

    //!
    //! A class to lookup the context of MPEG PSI/SI descriptors.
    //! @ingroup mpeg
    //!
    //! The interface of the base class is used to understand the context of a descriptor.
    //! When getting a binary descriptor, for instance, it is not always easy to determine
    //! the true nature of the descriptor, because the 8-bit descriptor tag can be used by
    //! different types of descriptors. The DescriptorContext provides the table id, the
    //! standards, CAS id, private specifier, or registration id which disambiguate the
    //! resolution.
     //!
    class TSDUCKDLL DescriptorContext
    {
        TS_NOBUILD_NOCOPY(DescriptorContext);
    public:
        //!
        //! Constructor, using default values.
        //! @param [in,out] duck A reference to the TSDuck execution context.
        //! The DuckContext must be valid until the destruction of the DescriptorContext object.
        //! @param [in] tid Optional table id. This value is returned by getTableId().
        //! @param [in] standards Optional set of standards. This value is returned by getStandards().
        //! @param [in] casid Optional Conditional Access System id. This value is returned by getCAS().
        //! @param [in] regids Optional list of registration ids. This value is returned by getREGIDs().
        //! @param [in] pds Optional DVB private data specifier. This value is returned by getPDS().
        //!
        DescriptorContext(const DuckContext& duck, TID tid = TID_NULL, Standards standards = Standards::NONE, CASID casid = CASID_NULL, const REGIDVector& regids = REGIDVector(), PDS pds = PDS_NULL);

        //!
        //! Constructor, using a descriptor list.
        //! @param [in,out] duck A reference to the TSDuck execution context.
        //! The DuckContext must be valid until the destruction of the DescriptorContext object.
        //! @param [in] dlist A reference to the descriptor list to search. The referenced object must
        //! remain valid as long as the context object references it.
        //! @param [in] index Index of the last descriptor to search in the list. If greater than
        //! than the list size, stop at the last descriptor in the list.
        //! @param [in] casid Optional Conditional Access System id. This value is returned by getCAS().
        //!
        DescriptorContext(const DuckContext& duck, const DescriptorList& dlist, size_t index, CASID casid = CASID_NULL);

        //!
        //! Get a reference to the TSDuck execution context.
        //! @return A reference to the TSDuck execution context.
        //!
        const DuckContext& duck() { return _duck; }

        //!
        //! Get the table id of the table where the descriptor is located.
        //! @return The parent table id or TID_NULL if unknown.
        //!
        TID getTableId() const;

        //!
        //! Get the standards where the descriptor is located.
        //! @return The standards where the descriptor is located.
        //!
        Standards getStandards() const;

        //!
        //! Get the Conditional Access System id in the context where the descriptor is located.
        //! @return The contextual CAS id or CASID_NULL if unknown.
        //!
        CASID getCAS() const;

        //!
        //! Get the private data specifier in the context where the descriptor is located.
        //! @return The contextual PDS or PDS_NULL if unknown.
        //!
        PDS getPDS();

        //!
        //! Get the list of registration ids in the context where the descriptor is located.
        //! @param [out] regids Receive the list of applicable MPEG registration ids.
        //!
        void getREGIDs(REGIDVector& regids);

        //!
        //! Get the list of registration ids in the context where the descriptor is located.
        //! @return The list of applicable MPEG registration ids.
        //!
        REGIDVector getREGIDs();

        //!
        //! Set the current descriptor list where to search for private identifiers, PDS or REGID.
        //! When the descriptor list is a second-level one (eg. component-level in a PMT), the
        //! top-level descriptor list of the same table (eg. program-level in a PMT) is automatically
        //! retrieved to look for registration ids.
        //! @param [in] dlist Address of the descriptor list to search. The pointed object must
        //! remain valid as long as the context object references it. If null, reset the descriptor
        //! list and revert to the default values which were passed to the constructor.
        //! @param [in] index Index of the last descriptor to search in the list. If greater than
        //! than the list size, stop at the last descriptor in the list.
        //!
        void setCurrentDescriptorList(const DescriptorList* dlist, size_t index = NPOS);

        //!
        //! Set the current descriptor list where to search for private identifiers, PDS or REGID.
        //! Unlike setCurrentDescriptorList(), the descriptor list is unstructured, this is just
        //! a memory area. Furthermore, when the descriptor list is a second-level one (eg. component-level
        //! in a PMT), the top-level descriptor list of the same table (eg. program-level in a PMT) cannot
        //! be automatically retrieved. In that case, the application needs to call setTopLevelRawDescriptorList()
        //! to establish where to search for higher-level registration ids.
        //! @param [in] data Address of the descriptor list to search. The pointed memory must
        //! remain valid as long as the context object references it. If null, reset the descriptor
        //! list and revert to the default values which were passed to the constructor.
        //! @param [in] size Size in bytes of the memory area. This is usually not the complete size of the
        //! descriptor list. The size shall end at the descriptor for which we need to establish a context
        //! (see parameter @a index in setCurrentDescriptorList()).
        //! @see setTopLevelRawDescriptorList()
        //!
        void setCurrentRawDescriptorList(const void* data, size_t size);

        //!
        //! Set the top-level descriptor list where to search for private identifiers, PDS or REGID.
        //! @param [in] data Address of the descriptor list to search. The pointed memory must
        //! remain valid as long as the context object references it. If null, reset the descriptor
        //! list and revert to the default values which were passed to the constructor.
        //! @param [in] size Size in bytes of the memory area. Unlike setCurrentRawDescriptorList(),
        //! this must be the total size of the top-level descriptor list.
        //! @see setCurrentRawDescriptorList()
        //!
        void setTopLevelRawDescriptorList(const void* data, size_t size);

        //!
        //! Move the current raw descriptor list as top-level descriptor list.
        //! The REGID analysis is preserved and moved to the top list.
        //! @see setCurrentRawDescriptorList()
        //! @see setTopLevelRawDescriptorList()
        //!
        void moveRawDescriptorListToTop();

    private:
        const DuckContext& _duck;
        TID                _tid = TID_NULL;
        CASID              _casid = CASID_NULL;
        Standards          _standards = Standards::NONE;
        REGIDVector        _default_regids {};         // As specified in the constructor.
        PDS                _default_pds = PDS_NULL;    // As specified in the constructor.
        PDS                _low_pds = PDS_NULL;        // As searched in lower-level descriptor list.
        REGIDVector        _top_regids {};             // As searched in top-level descriptor list.
        REGIDVector        _low_regids {};             // As searched in lower-level descriptor list.
        const DescriptorList* _dlist = nullptr;        // Current descriptor list (structured).
        size_t             _dlist_index = 0;           // List index to searched in _dlist.
        const uint8_t*     _top_dlist = nullptr;       // Top-level descriptor list (unstructured).
        size_t             _top_size = 0;              // Size in bytes of _top_dlist.
        const uint8_t*     _low_dlist = nullptr;       // Lower-level descriptor list (unstructured).
        size_t             _low_size = 0;              // Size in bytes of _low_dlist.
        bool               _use_defaults = true;       // No descriptor list set, use values from constructor.
        bool               _low_pds_valid = false;     // Lower-level descriptor list not searched yet.
        bool               _top_regids_valid = false;  // Top-level descriptor list not searched yet.
        bool               _low_regids_valid = false;  // Lower-level descriptor list not searched yet.

        // Update registration ids from a descriptor list.
        void updateREGIDs(REGIDVector& regids, const DescriptorList& dlist, size_t max_index, bool is_low);
        void updateREGIDs(REGIDVector& regids, const uint8_t* data, size_t size, bool is_low);
    };
}
