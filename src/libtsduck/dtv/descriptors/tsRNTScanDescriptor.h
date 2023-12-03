//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a RAR_over_IP_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an RNT_scan_descriptor
    //!
    //! This descriptor cannot be present in other tables than a RNT
    //! because its tag reuses a DVB-defined one.
    //!
    //! @see ETSI TS 102 323 clause 5.3.7.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL RNTScanDescriptor: public AbstractDescriptor
    {
    public:
        //!
        //! Scan triplet.
        //!
        class ScanTriplet
        {
            TS_DEFAULT_COPY_MOVE(ScanTriplet);
        public:
            uint16_t transport_stream_id = 0;   //!< The value of transport stream id of the transport stream referenced by this entry.
            uint16_t original_network_id = 0;   //!< The value of original network id of the transport stream referenced by this entry.
            uint8_t  scan_weighting = 0;        //!< The intended order of tuning to other transport streams to acquire RNTs.

            //!
            //! Default constructor.
            //!
            ScanTriplet();
            //!
            //! Constructor from a binary descriptor
            //! @param [in] buf A binary descriptor to deserialize.
            //!
            ScanTriplet(PSIBuffer& buf) : ScanTriplet() { deserialize(buf); }

            //! @cond nodoxygen
            void clearContent();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        std::vector<ScanTriplet> RNTreferences {};  //!< References to transport streams that carrying RNTs.

        //!
        //! Default constructor.
        //!
        RNTScanDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        RNTScanDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
