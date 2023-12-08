//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an Eutelsat_channel_number_descriptor.
//!  This is a private descriptor, must be preceded by the Eutelsat PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an Eutelsat_channel_number_descriptor.
    //! @ingroup descriptor
    //!
    //! This is a private descriptor, must be preceded by the Eutelsat PDS.
    //! See document "Via Eutelsat Fransat set-top-box specification",
    //! Version 0.0.7, October 2009, section 2.2.3.
    //!
    class TSDUCKDLL EutelsatChannelNumberDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t onetw_id = 0;    //!< Original network id.
            uint16_t ts_id = 0;       //!< Transport stream id.
            uint16_t service_id = 0;  //!< Service id.
            uint16_t ecn = 0;         //!< Eutelsat channel number.

            //!
            //! Constructor
            //! @param [in] onetw_id_ Original network id.
            //! @param [in] ts_id_ Transport stream id.
            //! @param [in] service_id_ Service id.
            //! @param [in] ecn_ Eutelsat channel number.
            //!
            Entry(uint16_t onetw_id_ = 0, uint16_t ts_id_ = 0, uint16_t service_id_ = 0, uint16_t ecn_ = 0);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of services entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 31;

        // EutelsatChannelNumberDescriptor public members:
        EntryList entries {};  //!< List of service entries.

        //!
        //! Default constructor.
        //!
        EutelsatChannelNumberDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EutelsatChannelNumberDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;
        virtual bool merge(const AbstractDescriptor& desc) override;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
