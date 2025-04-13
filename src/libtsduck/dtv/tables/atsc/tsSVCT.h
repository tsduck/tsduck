//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Satellite Virtual Channel Table (SVCT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsService.h"

namespace ts {
    //!
    //! Representation of an ATSC Satellite Virtual Channel Table (SVCT)
    //! @see ATSC A/81, section 9.9.1.
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL SVCT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a virtual channel.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Channel : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Channel);
            TS_DEFAULT_ASSIGMENTS(Channel);
        public:
            UString  short_name {};              //!< Channel short name (up to 7 characters).
            uint16_t major_channel_number = 0;   //!< 10 bits, major channel number.
            uint16_t minor_channel_number = 0;   //!< 10 bits, minor channel number.
            uint8_t  modulation_mode = 0;        //!< 6 bits, modulation mode.
            uint64_t carrier_frequency = 0;      //!< Carrier frequency in Hz. Warning: coded in units of 100 Hz.
            uint32_t carrier_symbol_rate = 0;    //!< Carrier symbol rate in symbols/second.
            uint8_t  polarization = 0;           //!< 2 bits, polarization.
            uint8_t  FEC_Inner = 0;              //!< FEC inner.
            uint16_t channel_TSID = 0;           //!< Transport stream id of the TS carrying the channel.
            uint16_t program_number = 0;         //!< Program number (aka. service id) of the channel.
            uint8_t  ETM_location = 0;           //!< 2 bits, location of Extended Text Message.
            bool     hidden = false;             //!< Hidden service.
            bool     hide_guide = false;         //!< Hide associated program guide information.
            uint8_t  service_type = 0;           //!< 6 bits, ATSC service type.
            uint16_t source_id = 0;              //!< ATSC source id.
            uint8_t  feed_id = 0;                //!< Feed id.

            //!
            //! Constructor.
            //! @param [in] table Parent SVCT.
            //!
            explicit Channel(const AbstractTable* table);

            //!
            //! Collect all informations about the service.
            //! @param [in,out] service A service description to update.
            //!
            void updateService(Service& service) const;
        };

        //!
        //! List of channels.
        //!
        using ChannelList = AttachedEntryList<Channel>;

        // SVCT public members:
        uint8_t        protocol_version = 0;  //!< ATSC protocol version.
        uint8_t        SVCT_subtype = 0;      //!< SVCT format, only 0 is defined.
        uint8_t        SVCT_id = 0;           //!< SVCT identifier.
        ChannelList    channels;              //!< List of channels which are described in this SVCT.
        DescriptorList descs;                 //!< Top-level descriptor list.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        SVCT(uint8_t version = 0, bool is_current = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        SVCT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        SVCT(const SVCT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        SVCT& operator=(const SVCT& other) = default;

        //!
        //! Collect all informations about all services in the SVCT.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] services A list of service descriptions. Existing services
        //! are updated with the informations from the SDT. New entries are created for
        //! other services.
        //!
        void updateServices(DuckContext& duck, ServiceList& services) const;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        virtual DescriptorList* topLevelDescriptorList() override;
        virtual const DescriptorList* topLevelDescriptorList() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
