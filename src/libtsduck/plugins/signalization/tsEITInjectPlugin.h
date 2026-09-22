//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to generate and inject EIT's in a transport stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsEITGenerator.h"
#include "tsPollFiles.h"
#include "tsThread.h"

namespace ts {
    //!
    //! Plugin to generate and inject EIT's in a transport stream.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL EITInjectPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(EITInjectPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Default interval in milliseconds between two poll operations.
        static constexpr cn::milliseconds DEFAULT_POLL_INTERVAL = cn::milliseconds(500);

        // Default minimum file stability delay.
        static constexpr cn::milliseconds DEFAULT_MIN_STABLE_DELAY = cn::milliseconds(500);

        // Stack size of listener threads.
        static constexpr size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;

        // File listener internal thread.
        class FileListener: public Thread, private PollFilesListener
        {
            TS_NOBUILD_NOCOPY(FileListener);
        public:
            FileListener(EITInjectPlugin* plugin);
            virtual ~FileListener() override;
            void stop();

        private:
            EITInjectPlugin* const _plugin;
            PollFiles              _poller;
            volatile bool          _terminate;

            // Implementation of Thread.
            virtual void main() override;

            // Implementation of PollFilesListener.
            virtual bool handlePolledFiles(const PolledFileList& files) override;
            virtual bool updatePollFiles(UString& wildcard, cn::milliseconds& poll_interval, cn::milliseconds& min_stable_delay) override;
        };

        // Command line options:
        bool                 _delete_files = false;
        bool                 _wait_first_batch = false;
        bool                 _use_system_time = false;
        Time                 _start_time {};
        PID                  _eit_pid = PID_EIT;
        EITOptions           _eit_options = EITOptions::GEN_ALL;
        BitRate              _eit_bitrate = 0;
        UString              _files {};
        int                  _ts_id = -1;
        cn::milliseconds     _poll_interval {};
        cn::milliseconds     _min_stable_delay {};
        cn::seconds          _data_offset {};
        cn::seconds          _input_offset {};
        EITRepetitionProfile _eit_profile {};

        // Working data.
        FileListener  _file_listener {this};
        EITGenerator  _eit_gen {duck, PID_EIT};
        volatile bool _check_files = false;    // there are files in _polled_files
        std::mutex    _polled_files_mutex {};  // exclusive access to _polled_files
        UStringList   _polled_files {};        // accessed by two threads, protected by mutex above.

        // Specific support for deterministic start (wfb = wait first batch, non-regression testing).
        volatile bool _wfb_received = false;   // First batch was received.
        std::mutex    _wfb_mutex {};           // Mutex waiting for _wfb_received.
        std::condition_variable _wfb_cond {};  // Condition waiting for _wfb_received.

        // Load files in the context of the plugin thread.
        void loadFiles();

        // Read a chrone option, using its current version as default value.
        template <class Rep, class Period>
        void updateChronoValue(cn::duration<Rep, Period>& value, const UChar* name)
        {
            getChronoValue(value, name, value);
        }
    };
}
