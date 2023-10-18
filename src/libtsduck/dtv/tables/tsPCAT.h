//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Partial Content Announcement Table (PCAT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ISDB Partial Content Announcement Table (PCAT).
    //! @see ARIB STD-B10, Part 2, 5.2.12
    //! @ingroup table
    //!
    class TSDUCKDLL PCAT : public AbstractLongTable
    {
    public:
        //!
        //! Schedule entry.
        //!
        class TSDUCKDLL Schedule
        {
        public:
            Schedule() = default;  //!< Constructor
            Time   start_time {};  //!< Event start_time in UTC (or JST in Japan).
            Second duration = 0;   //!< Event duration in seconds.
        };

        //!
        //! List of schedule entries.
        //!
        typedef std::list<Schedule> ScheduleList;

        //!
        //! Content version entry.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL ContentVersion : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(ContentVersion);
            TS_DEFAULT_ASSIGMENTS(ContentVersion);
        public:
            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            ContentVersion(const AbstractTable* table);

            uint16_t     content_version = 0;        //!< Content version.
            uint16_t     content_minor_version = 0;  //!< Content minor version.
            uint8_t      version_indicator = 0;      //!< 2 bits, how to use the version.
            ScheduleList schedules {};               //!< List of schedules.
        };

        //!
        //! List of content versions.
        //!
        typedef EntryWithDescriptorsList<ContentVersion> ContentVersionList;

        // PCAT public members:
        uint16_t           service_id = 0;           //!< Service id.
        uint16_t           transport_stream_id = 0;  //!< Transport stream id.
        uint16_t           original_network_id = 0;  //!< Original network id.
        uint32_t           content_id = 0;           //!< Content id.
        ContentVersionList versions;                 //!< List of content versions.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        PCAT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        PCAT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        PCAT(const PCAT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        PCAT& operator=(const PCAT& other) = default;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
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
