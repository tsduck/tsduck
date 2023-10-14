//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  This class maps PID's with CA system ids.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSectionDemux.h"
#include "tsCADescriptor.h"
#include "tsAlgorithm.h"

namespace ts {
    //!
    //! This class maps PID's with CA system ids.
    //! @ingroup mpeg
    //!
    //! All packets are passed through this object. It tracks the location of all
    //! EMM and ECM PID's and records the corresponding CAS attributes.
    //!
    class TSDUCKDLL CASMapper : private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(CASMapper);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside this object.
        //!
        CASMapper(DuckContext& duck);

        //!
        //! Reset the CAS mapper.
        //!
        void reset();

        //!
        //! This method feeds the CAS mapper with a TS packet.
        //! @param [in] pkt A new transport stream packet.
        //!
        void feedPacket(const TSPacket& pkt) { _demux.feedPacket(pkt); }

        //!
        //! Filter PSI tables based on current/next indicator.
        //! @param [in] current Use "current" tables. This is true by default.
        //! @param [in] next Use "next" tables. This is false by default.
        //!
        void setCurrentNext(bool current, bool next) { _demux.setCurrentNext(current, next); }

        //!
        //! Track / untrack invalid section version numbers.
        //! By default, if a section version does not change, the section is ignored.
        //! When this tracking is enabled, the content of the sections are tracked and
        //! a table is demuxed when a section version does not change but the content
        //! changes. This is considered as an error according to MPEG rules.
        //! @param [in] on Track invalid section versions. This is false by default.
        //!
        void trackInvalidSectionVersions(bool on) { _demux.trackInvalidSectionVersions(on); }

        //!
        //! Check if a PID is a known CA PID.
        //! @param [in] pid A PID to check.
        //! @return True if @ pid is a known ECM or EMM PID.
        //!
        bool knownPID(PID pid) const { return Contains(_pids, pid); }

        //!
        //! Get the CAS id of a CA PID (ECM or EMM).
        //! @param [in] pid A PID to check.
        //! @return The CAS id or zero if the PID is not known.
        //!
        uint16_t casId(PID pid) const;

        //!
        //! Check if a PID carries ECM's.
        //! @param [in] pid A PID to check.
        //! @return True if the PID carries ECM's, false otherwise.
        //!
        bool isECM(PID pid) const;

        //!
        //! Check if a PID carries EMM's.
        //! @param [in] pid A PID to check.
        //! @return True if the PID carries EMM's, false otherwise.
        //!
        bool isEMM(PID pid) const;

        //!
        //! Get the CA_descriptor which describes a CA PID (ECM or EMM).
        //! @param [in] pid A PID to check.
        //! @param [out] desc A safe pointer to the associated CA_descriptor.
        //! @return True if the CA_descriptor was found, false otherwise.
        //!
        bool getCADescriptor(PID pid, CADescriptorPtr& desc) const;

    private:
        // Description of one CA PID.
        class PIDDescription
        {
        public:
            uint16_t        cas_id;  //!< CA system id.
            bool            is_ecm;  //!< True for ECM, false for EMM.
            CADescriptorPtr ca_desc; //!< Corresponding CA descriptor.

            PIDDescription(uint16_t cas_id_ = 0, bool is_ecm_ = false, const CADescriptorPtr& ca_desc_ = CADescriptorPtr()) :
                cas_id(cas_id_),
                is_ecm(is_ecm_),
                ca_desc(ca_desc_)
            {
            }
        };

        // Map of key=PID to value=PIDDescription.
        typedef std::map<PID,PIDDescription> PIDDescriptionMap;

        // Explore a descriptor list and record EMM and ECM PID's.
        void analyzeCADescriptors(const DescriptorList& descs, bool is_ecm);

        // CAMapper private fields.
        DuckContext&      _duck;
        SectionDemux      _demux;
        PIDDescriptionMap _pids {};

        // Implementation of TableHandlerInterface
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
    };
}
