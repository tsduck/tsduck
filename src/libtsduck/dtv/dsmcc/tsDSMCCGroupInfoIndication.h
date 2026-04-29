//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC GroupInfoIndication structure (DSI userInfo for data carousels).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDSMCCCompatibilityDescriptor.h"

namespace ts {
    //!
    //! Representation of a DSM-CC GroupInfoIndication structure.
    //!
    //! Carried as the userInfo of a DownloadServerInitiate (DSI) message when the
    //! download is a data carousel (rather than an object carousel). One entry
    //! per group, each with its own groupCompatibility() descriptor.
    //!
    //! @see ISO/IEC 13818-6
    //! @see ETSI EN 301 192 V1.7.1 (2021-08), 10.1.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL DSMCCGroupInfoIndication
    {
    public:
        //!
        //! One group entry inside a GroupInfoIndication.
        //!
        class TSDUCKDLL Group
        {
        public:
            Group() = default;                                       //!< Default constructor.
            uint32_t group_id = 0;                                   //!< Identifier of the group.
            uint32_t group_size = 0;                                 //!< Size of the group, in bytes.
            DSMCCCompatibilityDescriptor group_compatibility {};     //!< Per-group compatibilityDescriptor.
            ByteBlock group_info {};                                 //!< Application-defined groupInfo bytes.
        };

        std::list<Group> groups {};      //!< List of groups.
        ByteBlock private_data {};       //!< Trailing privateData bytes.

        //!
        //! Default constructor.
        //!
        DSMCCGroupInfoIndication() = default;

        //!
        //! Clear the structure.
        //!
        void clear();

        //!
        //! Serialize the GroupInfoIndication.
        //! @param [in,out] buf Serialization buffer.
        //!
        void serialize(PSIBuffer& buf) const;

        //!
        //! Deserialize the GroupInfoIndication.
        //! @param [in,out] buf Deserialization buffer.
        //!
        void deserialize(PSIBuffer& buf);

        //!
        //! Display the GroupInfoIndication.
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the GroupInfoIndication.
        //! @param [in] margin Left margin content.
        //!
        static void Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin);

        //!
        //! Convert the GroupInfoIndication to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML element.
        //!
        void toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! Decode the GroupInfoIndication from XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The XML GroupInfoIndication element.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* element);
    };
}  // namespace ts
