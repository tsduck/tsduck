//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract superclass for input tsp plugins working in push mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"
#include "tsThread.h"
#include "tsTSPacketQueue.h"

namespace ts {
    //!
    //! Abstract superclass for input tsp plugins working in push mode.
    //! @ingroup libtsduck plugin
    //!
    //! An input plugin works in "pull" mode by default. When @c tsp needs mode
    //! input packets, it calls the receive() method of the plugin to "pull" packets.
    //!
    //! Some input devices, however, naturally work in "push" mode. Some code loops
    //! and pushes new packets somewhere when some are available. When this looping
    //! or dispatching code is a third-party one, it is impossible to modify it and
    //! make is work in pull mode.
    //!
    //! This class is a bridge between push mode and pull mode. Input plugins which
    //! prefer to work in push mode should inherit from this class.
    //!
    //! Note: This class was originally developed to support HTTP-based input plugins.
    //! Because the "curl_easy" interface was used on Unix, these plugins had to work
    //! in push mode. Now, we use these plugins use the "curl_multi" interface and
    //! the PushInputPlugin is no longer used in TSDuck. It remains here jsut in case
    //! some future input plugin has to work in push mode.
    //!
    class TSDUCKDLL PushInputPlugin : public InputPlugin
    {
        TS_NOBUILD_NOCOPY(PushInputPlugin);
    public:
        //!
        //! Destructor.
        //!
        virtual ~PushInputPlugin() override;

    protected:
        //!
        //! Packet reception interface.
        //!
        //! The concrete class shall implement this method to process input.
        //! This method is called only once and should loop until end of input.
        //! When packets are available, processInput() shall invoke pushPackets().
        //!
        //! This method shall return immediately in any of the following cases:
        //! - End of input.
        //! - pushPackets() returns false, meaning that termination is requested.
        //! - tsp->aborting() return true, meaning that the program was interrupted.
        //! - Unrecoverable input error.
        //!
        virtual void processInput() = 0;

        //!
        //! Plugin start method.
        //! If a subclass overrides start(), it should invoke the superclass at the beginning of its start() method.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool start() override;

        //!
        //! Plugin stop method.
        //! If a subclass overrides stop(), it should invoke the superclass at the end of its stop() method.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool stop() override;

        //!
        //! Tune the TS packet buffer between the "push" subclass and the "pull" superclass.
        //! This method must be called from the start() method in the superclass.
        //! @param [in] count Size of the intermediate buffer in packets.
        //!
        void setQueueSize(size_t count);

        //!
        //! Push packet to the @c tsp chain.
        //! This method must be called from the processInput() method in the superclass.
        //! @param [in] buffer Address of the buffer containing incoming packets.
        //! @param [in] mdata Address of the buffer containing incoming packets metadata.
        //! @param [in] count Size of @a buffer in number of packets.
        //! @return True on success, false on error or requested termination.
        //!
        virtual bool pushPackets(const TSPacket* buffer, const TSPacketMetadata* mdata, size_t count);

        //!
        //! Constructor.
        //!
        //! @param [in] tsp_ Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //!
        PushInputPlugin(TSP* tsp_, const UString& description = UString(), const UString& syntax = UString());

        // Implementation of plugin API
        virtual bool abortInput() override;

    protected:
        //!
        //! Invoked by subclass, typically in processInput(), to check if the plugin was interrupted on purpose.
        //! @return True if the plugin was interrupted on purpose.
        //!
        bool isInterrupted() const { return _interrupted; }

    private:
        // Internal thread which receives TS packets.
        class Receiver : public Thread
        {
            TS_NOBUILD_NOCOPY(Receiver);
        public:
            // Constructor & destructor.
            Receiver(PushInputPlugin* plugin);
            virtual ~Receiver() override;
        protected:
            virtual void main() override;
        private:
            PushInputPlugin* _plugin;
        };

        // Plugin private data.
        Receiver      _receiver {this};
        bool          _started = false;
        volatile bool _interrupted = false;
        TSPacketQueue _queue {};

        // Standard input routine, now hidden from subclasses.
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
    };
}
