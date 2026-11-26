//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TSP callback for plugins.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsAbortInterface.h"
#include "tsPluginType.h"
#include "tsTS.h"

namespace ts {

    // Forward class declaration for private part.
    class Plugin;
    class Object;
    namespace tsp {
        class Core;
    }

    //!
    //! TSP callback for plugins.
    //! @ingroup libtsduck plugin
    //!
    //! Each plugin has an associated TSP object to communicate with the TSProcessor instance.
    //!
    //! Message output
    //! --------------
    //! A shared library must exclusively use the tsp object for text display and must never use @c std::cout,
    //! @c printf or alike. When called in multi-threaded context, the supplied tsp object is thread-safe and
    //! asynchronous (the methods return to the caller without waiting for the message to be printed).
    //!
    //! "Joint termination" support
    //! ---------------------------
    //! A plugin can decide to terminate tsp on its own (returning end of input, output error or
    //! @link ts::TSP_END @endlink). The termination is unconditional, regardless of the state of the other plugins.
    //!
    //! The idea behind "joint termination" is to terminate tsp when several plugins have jointly terminated their processing.
    //!
    //! First, a plugin must decide to use "joint termination". This is usually done in method start(),
    //! using useJointTermination (bool) when the option -\-joint-termination is specified on the command line.
    //!
    //! When the plugin has completed its work, it reports this using jointTerminate().
    //!
    class TSDUCKDLL TSP: public Report, public AbortInterface
    {
        TS_NOBUILD_NOCOPY(TSP);
    public:
        //!
        //! Get the current input bitrate in bits/seconds.
        //! @return The current input bitrate in bits/seconds or zero if unknown.
        //!
        BitRate bitrate() const { return _tsp_bitrate; }

        //!
        //! Get the plugin bitrate confidence.
        //! @return The level of confidence of the bitrate value as returned by the previous call to bitrate().
        //!
        BitRateConfidence bitrateConfidence() const { return _tsp_bitrate_confidence; }

        //!
        //! Access the shared library through the plugin interface.
        //! @return Address of the plugin interface.
        //!
        virtual Plugin* plugin() const = 0;

        //!
        //! Get the plugin name.
        //! @return The plugin name. This is typically the name which is used in the commmand line.
        //!
        virtual UString pluginName() const = 0;

        //!
        //! Get the plugin index in the processing chain.
        //! @return The plugin index. For a TS processor, this is typically 0 for the input plugin
        //! and the number of plugins minus one for the output plugin. For an input switcher, this
        //! is in input index for input plugins and the number of plugins minus one for the output plugin.
        //!
        virtual size_t pluginIndex() const = 0;

        //!
        //! Get the number of plugins in the processing chain.
        //! @return The number of plugins in the processing chain.
        //!
        virtual size_t pluginCount() const = 0;

        //!
        //! Get total number of packets previously processed in the plugin object.
        //! For input and output plugins, this is the number of successfully read or written packets.
        //! For processor plugins, this is the number of packets which were submitted to the plugin
        //! object (ie. excluding previously dropped packets but including packets which were dropped
        //! by the current plugin).
        //! @return The total number of packets in this plugin object.
        //!
        PacketCounter pluginPackets() const { return _plugin_packets; }

        //!
        //! Get total number of packets in the execution of the plugin thread.
        //! This includes the number of extra stuffing or dropped packets.
        //! @return The total number of packets in this plugin thread.
        //!
        PacketCounter totalPacketsInThread() const { return _total_packets; }

        //!
        //! Check if the current plugin environment should use defaults for real-time.
        //! @return True if the current plugin environment should use defaults for real-time.
        //!
        bool realtime() const { return _use_realtime; }

        //!
        //! Set a timeout for the reception of packets by the current plugin.
        //! For input plugins, this is the timeout for the availability of free space in input buffer.
        //!
        //! When the timeout is triggered, the method handlePacketTimeout() is invoked in the plugin.
        //! If the method returns true, the application continues waiting for packets.
        //! If the method returns false, the plugin is aborted.
        //!
        //! @param [in] timeout Maximum number of milliseconds to wait for packets in the buffer.
        //! Infinite timeout when negative. The default timeout is infinite.
        //!
        void setPacketTimeout(cn::milliseconds timeout) { _tsp_timeout = timeout; }

        //!
        //! Check for aborting application.
        //! Implementation of AbortInterface.
        //! The plugin may invoke this method to check if the application is
        //! aborting for some reason (user interrupt for instance).
        //! @return True if the tsp application is currently aborting.
        //!
        virtual bool aborting() const override;

        //!
        //! Signal a plugin event.
        //! If the application has registered plugin events for this kind of events, they will be invoked.
        //! @param [in] event_code A plugin-defined 32-bit code describing the event type.
        //! There is no predefined list of event codes. Plugin should define their own codes.
        //! @param [in] plugin_data Address of optional plugin-specific data. It can be a null pointer.
        //! Each plugin may defined subclasses of Object to pass specific data to application handlers
        //! which are aware of this plugin.
        //!
        virtual void signalPluginEvent(uint32_t event_code, Object* plugin_data = nullptr) const = 0;

        //!
        //! Activates or deactivates "joint termination".
        //! This method activates or deactivates "joint termination" for the calling plugin.
        //! It should be invoked during the plugin's start(). Only packet processing plugins
        //! can use joint termination. The concept has no meaning in the input or output cases.
        //! @param [in] on True to activate or false to deactivate joint termination.
        //! @return True on success, false on error (if the plugin is an input or output one).
        //!
        bool useJointTermination(bool on);

        //!
        //! Check if the calling plugin uses "joint termination".
        //! @return True if the calling plugin uses "joint termination".
        //!
        bool useJointTermination() const { return _use_jt; }

        //!
        //! Signaling "joint termination".
        //! This method is used by the plugin to declare that its execution is potentially terminated in the context of "joint termination".
        //! After invoking this method, any packet which is processed by the plugin may be ignored by tsp.
        //!
        void jointTerminate();

        //!
        //! Check if the calling plugin has already declared "joint termination".
        //! @return True if the calling plugin has already declared "joint termination".
        //!
        bool thisJointTerminated() const { return _jt_completed; }

        //!
        //! Virtual desctructor.
        //!
        virtual ~TSP() override;

    protected:
        std::shared_ptr<tsp::Core> _core;            //!< Pointer to the transport stream processor core API.
        bool              _use_realtime = false;     //!< The plugin should use realtime defaults.
        BitRate           _tsp_bitrate = 0;          //!< TSP input bitrate.
        BitRateConfidence _tsp_bitrate_confidence = BitRateConfidence::LOW;  //!< TSP input bitrate confidence.
        cn::milliseconds  _tsp_timeout = cn::milliseconds(-1);  //!< Timeout when waiting for packets, infinite if negative.
        std::atomic_bool  _tsp_aborting = false;     //!< TSP is currently aborting.

        //!
        //! Constructor for subclasses.
        //! @param [in] core Shared pointer to the transport stream processor core API.
        //! @param [in] prefix The prefix to prepend to all messages.
        //! @param [in] type Plugin type.
        //!
        TSP(const std::shared_ptr<tsp::Core>& core, const UString& prefix, PluginType type);

        //!
        //! Account for more processed packets in this plugin object.
        //! @param [in] incr Add this number of processed packets in the plugin object.
        //!
        void addPluginPackets(size_t incr) {_plugin_packets += incr; _total_packets += incr;}

        //!
        //! Account for more processed packets in this plugin thread, but excluded from plugin object.
        //! @param [in] incr Add this number of processed packets in the plugin thread.
        //!
        void addNonPluginPackets(size_t incr) {_total_packets += incr;}

        //!
        //! Restart accounting for plugin session.
        //! Typically invoked when the plugin is restarted.
        //!
        void restartPluginSession() { _plugin_packets = 0; }

    private:
        PluginType    _plugin_type;           // Plugin type.
        bool          _use_jt = false;        // Use "joint termination"
        bool          _jt_completed = false;  // Completed, for "joint termination"
        PacketCounter _total_packets = 0;     // Total processed packets in the plugin thread.
        PacketCounter _plugin_packets = 0;    // Total processed packets in the plugin object.

        // A dirty hack for the default implementation of ts::ProcessorPlugin::processPacketWindow().
        friend class ProcessorPlugin;
    };
}
