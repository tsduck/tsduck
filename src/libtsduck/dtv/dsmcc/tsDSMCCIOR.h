//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC Interoperable Object Reference (IOR) structure.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDSMCCTaggedProfile.h"

namespace ts {
    //!
    //! Representation of Interoperable Object Reference (IOR) structure
    //! @see ISO/IEC 13818-6
    //! @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.3.1
    //!
    class TSDUCKDLL DSMCCIOR
    {
    public:
        ByteBlock type_id {};                              //!< U-U Objects type_id.
        std::list<DSMCCTaggedProfile> tagged_profiles {};  //!< List of tagged profiles.

        //!
        //! Default constructor.
        //!
        DSMCCIOR() = default;

        //!
        //! Clear values.
        //!
        void clear();

        //!
        //! Serialize the IOR structure.
        //! @param [in,out] buf Serialization buffer.
        //!
        void serialize(PSIBuffer& buf) const;

        //!
        //! Deserialize the IOR structure.
        //! @param [in,out] buf Deserialization buffer.
        //!
        void deserialize(PSIBuffer& buf);

        //!
        //! Display the IOR structure.
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the IOR.
        //! @param [in] margin Left margin content.
        //!
        static void Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin);

        //!
        //! This method converts an IOR to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML IOR.
        //!
        void toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! This method decodes an XML IOR.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The XML element containing the IOR.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* element);
    };
}  // namespace ts
