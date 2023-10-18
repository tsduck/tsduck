//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DSM-CC NPT_reference_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {

    //!
    //! Representation of a DSM-CC NPT_reference_descriptor.
    //! @see ISO/IEC 13818-6, 8.1.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL NPTReferenceDescriptor : public AbstractDescriptor
    {
    public:
        // NPTReferenceDescriptor public members:
        bool     post_discontinuity = false;  //!< Post discontinuity indicator.
        uint8_t  content_id = 0;              //!< 7 bits, optional content id.
        uint64_t STC_reference = 0;           //!< 33 bits, reference System Time Clock (STC), PCR value in PTS units, ie. PCR/300.
        uint64_t NPT_reference = 0;           //!< 33 bits, reference Normal Play Time (NPT).
        uint16_t scale_numerator = 0;         //!< Numerator of NPT/STC rate change.
        uint16_t scale_denominator = 0;       //!< Denominator of NPT/STC rate change.

        //!
        //! Default constructor.
        //!
        NPTReferenceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        NPTReferenceDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Recompute the NPT/STC scale using another NPT_reference_descriptor.
        //! The fields @a scale_numerator and @a scale_denominator are recomputed
        //! from the difference between the two NPT refereces.
        //! @param [in] other_reference Another NPT_reference_descriptor.
        //! The other reference may preceed or follow this reference in time, indifferently.
        //! @param [in] force If true (the default), always recompute the scale.
        //! If false, do not modify the scale if it already exists.
        //! @see ISO/IEC 13818-6, 8.1.2.
        //!
        void computeScale(const NPTReferenceDescriptor& other_reference, bool force = true);

        //!
        //! Convert an NPT into STC (PTS or DTS) using the references from this descriptor.
        //! @param [in] npt A Normal Play Time (NPT) value.
        //! @return The corresponding System Time Clock (STC, a PTS or DTS value) or
        //! zero if this descriptor does not contain a valid reference or scale.
        //!
        uint64_t nptToSTC(uint64_t npt) const;

        //!
        //! Convert an NPT into PCR using the references from this descriptor.
        //! @param [in] npt A Normal Play Time (NPT) value.
        //! @return The corresponding Program Clock Reference (PCR) or
        //! zero if this descriptor does not contain a valid reference or scale.
        //!
        uint64_t nptToPCR(uint64_t npt) const;

        //!
        //! Convert a PCR into NPT using the references from this descriptor.
        //! @param [in] pcr A Program Clock Reference (PCR) value.
        //! @return The corresponding Normal Play Time (NPT) or
        //! zero if this descriptor does not contain a valid reference or scale.
        //!
        uint64_t pcrToNPT(uint64_t pcr) const;

        //!
        //! Convert an STC (PTS or DTS) into NPT using the references from this descriptor.
        //! @param [in] stc A System Time Clock (STC) value, ie. a PTS or DTS value.
        //! @return The corresponding Normal Play Time (NPT) or
        //! zero if this descriptor does not contain a valid reference or scale.
        //!
        uint64_t stcToNPT(uint64_t stc) const;

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
