//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Perform various transformations on an EIT PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSectionDemux.h"
#include "tsPacketizer.h"
#include "tsTSPacket.h"
#include "tsService.h"
#include "tsTransportStreamId.h"

namespace ts {
    //!
    //! Perform various transformations on an EIT PID.
    //! @ingroup mpeg
    //!
    //! The object is continuously invoked for all packets in a TS.
    //! Packets from one specific PID, the EIT PID, are replaced.
    //! The various required transformations on the EIT's are performed.
    //!
    //! More generally, there are several input PID's and one output PID.
    //! All EIT sections from any input PID are merged into one single
    //! output PID. All input PID's are overwritten by packets for the
    //! output PID (or null packets).
    //!
    //! By default, there is only one input PID which is also used as
    //! output PID. This is PID 0x12, the standard DVB PID for EIT's.
    //!
    class TSDUCKDLL EITProcessor :
        private SectionHandlerInterface,
        private SectionProviderInterface
    {
        TS_NOBUILD_NOCOPY(EITProcessor);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside this object.
        //! @param [in] pid The PID containing EIT's to process.
        //! This PID is used as only input PID and output PID.
        //!
        explicit EITProcessor(DuckContext& duck, PID pid = PID_EIT);

        //!
        //! Change the single PID containing EIT's to process.
        //! @param [in] pid The PID containing EIT's to process.
        //! This PID is used as only input PID and output PID.
        //!
        void setPID(PID pid);

        //!
        //! Set one single input PID without altering the output PID.
        //! @param [in] pid The single input PID.
        //!
        void setInputPID(PID pid);

        //!
        //! Change the output PID without altering the input PID's.
        //! @param [in] pid The output PID.
        //!
        void setOutputPID(PID pid);

        //!
        //! Clear the set of input PID's.
        //!
        void clearInputPIDs();

        //!
        //! Add an input PID without altering the output PID.
        //! @param [in] pid An input PID to add.
        //!
        void addInputPID(PID pid);

        //!
        //! Reset the EIT processor to default state.
        //! The input and output PID's are unchanged.
        //!
        void reset();

        //!
        //! Process one packet from the stream.
        //! @param [in,out] pkt A TS packet from the stream. If the packet belongs
        //! to the EIT PID, it is updated with the new content.
        //!
        void processPacket(TSPacket& pkt);

        //!
        //! Remove all EIT's for a given transport stream.
        //! @param [in] ts_id Id of the transport stream to remove (any original network id).
        //!
        void removeTS(uint16_t ts_id);

        //!
        //! Remove all EIT's for a given transport stream.
        //! @param [in] ts Transport stream id and original network id to remove.
        //!
        void removeTS(const TransportStreamId& ts);

        //!
        //! Rename all EIT's for a given transport stream.
        //! @param [in] old_ts_id Id of the transport stream to rename (any original network id).
        //! @param [in] new_ts_id New transport stream id (original network id unchanged).
        //!
        void renameTS(uint16_t old_ts_id, uint16_t new_ts_id);

        //!
        //! Rename all EIT's for a given transport stream.
        //! @param [in] old_ts Transport stream id and original network id to rename.
        //! @param [in] new_ts New transport stream id and original network id.
        //!
        void renameTS(const TransportStreamId& old_ts, const TransportStreamId& new_ts);

        //!
        //! Keep all EIT's for a given service in the current transport stream (EIT Actual).
        //! @param [in] service_id Id of the service to keep in EIT Actual.
        //!
        //! Note: Keeping services always prevails over removing them. This means that if
        //! keepService() is called once or more, all services are removed except the
        //! explicitly kept ones and removeService() is ignored.
        //!
        void keepService(uint16_t service_id);

        //!
        //! Keep all EIT's for a given service.
        //! @param [in] service Description of the service to keep.
        //! @see keepService(uint16_t)
        //!
        void keepService(const Service& service);

        //!
        //! Remove all EIT's for a given service in the current transport stream (EIT Actual).
        //! @param [in] service_id Id of the service to remove in EIT Actual.
        //! @see keepService(uint16_t)
        //!
        void removeService(uint16_t service_id);

        //!
        //! Remove all EIT's for a given service.
        //! @param [in] service Description of the service to remove.
        //! @see keepService(uint16_t)
        //!
        void removeService(const Service& service);

        //!
        //! Check if some service filtering is set (keep or remove specific services).
        //! @return True if some service filtering is set (keep or remove specific services).
        //!
        bool filterServices() const { return !_kept.empty() || !_removed.empty(); }

        //!
        //! Rename all EIT's for a given service.
        //! @param [in] old_service Description of the service to rename.
        //! @param [in] new_service New description of the service.
        //!
        void renameService(const Service& old_service, const Service& new_service);

        //!
        //! Remove all EIT's with a table id in a given list.
        //! @param [in] tids List of all table ids to remove.
        //!
        void removeTableIds(std::initializer_list<TID> tids);

        //!
        //! Remove all EIT Other.
        //!
        void removeOther();

        //!
        //! Remove all EIT Actual.
        //!
        void removeActual();

        //!
        //! Remove all EIT Schedule.
        //!
        void removeSchedule();

        //!
        //! Remove all EIT Present/Following.
        //!
        void removePresentFollowing();

        //!
        //! Add an offset to all start times of all events in all EIT's.
        //! @param [in] offset The number of milliseconds to add to each start time. Can be negative.
        //! @param [in] date_only If true, update the date field only, do not modify the hour/minute/second.
        //!
        void addStartTimeOffet(MilliSecond offset, bool date_only = false);

        //!
        //! Set the maximum number of buffered sections.
        //!
        //! A few number of EIT sections may be temporarily buffered.
        //!
        //! Each EIT section is completely loaded, modified and reinjected, replacing input packets.
        //! If the currently reinjected EIT section is very large and, at the same time, a lot of
        //! small EIT sections are received, they must be buffered. This is normally a transient
        //! situation. Since the number of reinjected sections is at most identical to the input
        //! number and since EIT sections are never enlarged, there is no global overflow.
        //!
        //! This method is used to adjust the maximum number of buffered sections.
        //!
        //! @param [in] count New maximum number of buffered sections.
        //!
        void setMaxBufferedSections(size_t count);

        //!
        //! Minimum number of buffered sections.
        //! @see setMaxBufferedSections()
        //!
        static constexpr size_t MIN_BUFFERED_SECTIONS = 10;

        //!
        //! Default number of buffered sections.
        //! @see setMaxBufferedSections()
        //!
        static constexpr size_t DEFAULT_BUFFERED_SECTIONS = 1000;

        //!
        //! Get the maximum number of buffered sections.
        //! @return The maximum number of buffered sections.
        //! @see setMaxBufferedSections()
        //!
        size_t getMaxBufferedSections() const { return _max_buffered_sections; }

        //!
        //! Get the current number of buffered sections.
        //! @return The current number of buffered sections.
        //! @see setMaxBufferedSections()
        //!
        size_t getCurrentBufferedSections() const { return _sections.size(); }

    private:
        DuckContext&          _duck;
        PIDSet                _input_pids {};
        PID                   _output_pid = PID_NULL;
        MilliSecond           _start_time_offset = 0;
        bool                  _date_only = false;
        size_t                _max_buffered_sections {DEFAULT_BUFFERED_SECTIONS};
        SectionDemux          _demux;
        Packetizer            _packetizer;
        std::list<SectionPtr> _sections {};
        std::set<TID>         _removed_tids {};
        std::list<Service>    _removed {};
        std::list<Service>    _kept {};
        std::list<std::pair<Service,Service>> _renamed {};

        // Check if a service matches a DVB triplet.
        // The service must have at least a service id or transport id.
        static bool Match(const Service& srv, uint16_t srv_id, uint16_t ts_id, uint16_t net_id);

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override;
    };
}
