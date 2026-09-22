//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to perform various transformations on a PMT.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTablePlugin.h"
#include "tsServiceDiscovery.h"
#include "tsAudioLanguageOptions.h"

namespace ts {
    //!
    //! Plugin to perform various transformations on a PMT.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PMTPlugin: public AbstractTablePlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PMTPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of a new component to add
        struct NewPID {
            PID     pid;
            uint8_t type;

            // Constructor.
            NewPID(PID pid_ = PID_NULL, uint8_t stype_ = 0) :
                pid(pid_),
                type(stype_)
            {
            }
        };

        // Map of new descriptors to add per component.
        using DescriptorListPtr = std::shared_ptr<DescriptorList>;
        using DescriptorListByPID = std::map<PID, DescriptorListPtr>;

        // PMTPlugin instance fields
        ServiceDiscovery     _service {duck, nullptr};   // Service of PMT to modify
        std::vector<PID>     _removed_pids {};           // Set of PIDs to remove from PMT
        std::vector<DID>     _removed_desc_tags {};      // Set of descriptor tags to remove
        std::vector<uint8_t> _removed_stream_types {};   // Set of stream types to remove
        std::vector<REGID>   _removed_registrations {};  // Set of registrations for which PID's are removed
        std::list<NewPID>    _added_pids {};             // List of PID to add
        std::map<PID,PID>    _moved_pids {};             // List of renamed PID's in PMT (key=old, value=new)
        bool                 _set_servid = false;        // Set a new service id
        uint16_t             _new_servid = 0;            // New service id
        bool                 _set_pcrpid = false;        // Set a new PCR PID
        PID                  _new_pcrpid = PID_NULL;     // New PCR PID
        PDS                  _pds = 0;                   // Private data specifier for removed descriptors
        bool                 _add_stream_id = false;     // Add stream_identifier_descriptor on all components
        bool                 _ac3_atsc2dvb = false;      // Modify AC-3 signaling from ATSC to DVB method
        bool                 _eac3_atsc2dvb = false;     // Modify Enhanced-AC-3 signaling from ATSC to DVB method
        bool                 _cleanup_priv_desc = false; // Remove private desc without preceding PDS desc
        DescriptorList       _add_descs {nullptr};       // List of descriptors to add at program level
        DescriptorListByPID  _add_pid_descs {};          // Lists of descriptors to add by PID
        AudioLanguageOptionsVector _languages {};        // Audio languages to set
        std::vector<PID>     _sort_pids {};              // Sorting order of PIDs in PMT
        UStringVector        _sort_languages {};         // Sorting order of audio and subtitles PIDs in PMT

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;

        // Add a descriptor for a given PID in _add_pid_descs.
        void addComponentDescriptor(PID pid, const AbstractDescriptor& desc);

        // Decode an option "pid/value[/hexa]". Hexa is allowed only if hexa is non zero.
        template<typename INT>
        bool decodeOptionForPID(const UChar* parameter_name, size_t parameter_index, PID& pid, INT& value, ByteBlock* hexa = nullptr, INT value_max = std::numeric_limits<INT>::max());

        // Decode options like --set-stream-identifier which add a simple descriptor in a component.
        template<typename DESCRIPTOR, typename INT>
        bool decodeComponentDescOption(const UChar* parameter_name);
    };
}
