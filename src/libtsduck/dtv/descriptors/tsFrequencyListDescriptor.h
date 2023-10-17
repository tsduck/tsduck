//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a frequency_list_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsEnumeration.h"

namespace ts {
    //!
    //! Representation of a frequency_list_descriptor
    //! @see ETSI EN 300 468, 6.2.17.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL FrequencyListDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Maximum number of frequency entries to fit in 254 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 63;

        //!
        //! Type frequency in @a coding_type field.
        //!
        enum {
            UNDEFINED   = 0,  //!< Frequency coding not defined, assume Hz.
            SATELLITE   = 1,  //!< Encoded as satellite frequency.
            CABLE       = 2,  //!< Encoded as cable frequency.
            TERRESTRIAL = 3,  //!< Encoded as terrestrial frequency.
        };

        //!
        //! Enumeration description of coding types.
        //!
        static const Enumeration CodingTypeEnum;

        // FrequencyListDescriptor public members:
        uint8_t               coding_type = UNDEFINED;  //!< 2 bits, type of frequency (cable, satellite, etc.)
        std::vector<uint64_t> frequencies {};           //!< The list of centre frequencies.

        //!
        //! Default constructor.
        //!
        FrequencyListDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        FrequencyListDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Decode a frequency at a 4-byte data area.
        static uint64_t DecodeFrequency(uint8_t coding_type, PSIBuffer&);
    };
}
