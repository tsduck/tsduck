//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definition of the API of a tsp plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsAbortInterface.h"
#include "tsReport.h"
#include "tsTSPacket.h"
#include "tsEnumeration.h"

namespace ts {

    //!
    //! Each plugin has one of the following types
    //! @ingroup plugin
    //!
    enum PluginType {
        INPUT_PLUGIN,     //!< Input plugin.
        OUTPUT_PLUGIN,    //!< Output plugin.
        PROCESSOR_PLUGIN  //!< Packet processor plugin.
    };

    //!
    //! Displayable names of plugin types.
    //!
    TSDUCKDLL extern const Enumeration PluginTypeNames;


    //-------------------------------------------------------------------------
    //! TSP callback for plugins.
    //-------------------------------------------------------------------------
    //!
    //! @ingroup plugin
    //!
    //! Each plugin has an associated TSP object to communicate with the
    //! Transport Stream Processor main executable.
    //!
    //! Message output
    //! --------------
    //!
    //! A shared library must exclusively use the tsp object for text
    //! display and must never use @c std::cout, @c printf or alike. When
    //! called in multi-threaded context, the supplied tsp object is
    //! thread-safe and asynchronous (the methods return to the
    //! caller without waiting for the message to be printed).
    //!
    //! "Joint termination" support
    //! ---------------------------
    //!
    //! A plugin can decide to terminate tsp on its own (returning end of
    //! input, output error or @link ts::ProcessorPlugin::TSP_END@endlink). The termination is unconditional,
    //! regardless of the state of the other plugins.
    //!
    //! The idea behind "joint termination" is to terminate tsp when several
    //! plugins have jointly terminated their processing.
    //!
    //! First, a plugin must decide to use "joint termination". This is
    //! usually done in method start(), using useJointTermination (bool)
    //! when the option -\-joint-termination is specified on the command line.
    //!
    //! When the plugin has completed its work, it reports this using
    //! jointTerminate().
    //!
    class TSDUCKDLL TSP: public Report, public AbortInterface
    {
    public:
        //!
        //! Plugin API Version Number
        //!
        //! Important: Must be incremented each time the tsp plugin abstract
        //! interfaces are modified. All shared libraries shall export a global
        //! @c int data named @c tspInterfaceVersion which contains the current
        //! interface version at the time the library is built.
        //!
        static const int API_VERSION = 8;

        //!
        //! Get the current input bitrate in bits/seconds.
        //! @return The current input bitrate in bits/seconds or zero if unknown.
        //!
        BitRate bitrate() const {return _tsp_bitrate;}

        //!
        //! Check if the current plugin environment should use defaults for real-time.
        //! @return True if the current plugin environment should use defaults for real-time.
        //!
        bool realtime() const {return _use_realtime;}

        //!
        //! Check for aborting application.
        //!
        //! The plugin may invoke this method to check if the application is
        //! aborting for some reason (user interrupt for instance).
        //! @return True if the tsp application is currently aborting.
        //!
        virtual bool aborting() const override {return _tsp_aborting;}

        //!
        //! Activates or deactivates "joint termination".
        //!
        //! This method activates or deactivates "joint termination" for the
        //! calling plugin. It should be invoked during the plugin's start().
        //! @param [in] on True to activate or false to deactivate joint termination.
        //!
        virtual void useJointTermination(bool on) = 0;

        //!
        //! Signaling "joint termination".
        //!
        //! This method is used by the plugin to declare that its execution is
        //! potentially terminated in the context of "joint termination".
        //! After invoking this method, any packet which is processed by
        //! the plugin may be ignored by tsp.
        //!
        virtual void jointTerminate() = 0;

        //!
        //! Check if the calling plugin uses "joint termination".
        //! @return True if the calling plugin uses "joint termination".
        //!
        virtual bool useJointTermination() const = 0;

        //!
        //! Check if the calling plugin has already declared "joint termination".
        //! @return True if the calling plugin has already declared "joint termination".
        //!
        virtual bool thisJointTerminated() const = 0;

    protected:
        bool          _use_realtime;  //!< The plugin should use realtime defaults.
        BitRate       _tsp_bitrate;   //!< TSP input bitrate.
        volatile bool _tsp_aborting;  //!< TSP is currently aborting.

        //!
        //! Constructor for subclasses.
        //! @param [in] max_severity Initial maximum severity of reported messages.
        //!
        TSP(int max_severity);

    private:
        // Inaccessible operations
        TSP() = delete;
        TSP(const TSP&) = delete;
        TSP& operator=(const TSP&) = delete;
    };


    //-------------------------------------------------------------------------
    //! Base class of all @c tsp plugins.
    //-------------------------------------------------------------------------
    //!
    //! @ingroup plugin
    //!
    //! Plugin is a subclass of Args; each constructor is expected to define
    //! the syntax, help and option definitions for the command line.
    //!
    //! A shared library is invoked into a multi-threaded environment.
    //! It must be thread-safe. It may define its maximum stack usage.
    //!
    class TSDUCKDLL Plugin: public Args
    {
    public:
        //!
        //! Default stack usage in bytes for the thread executing a plugin.
        //!
        static const size_t DEFAULT_STACK_USAGE = 128 * 1024;

        //!
        //! Define the maximum stack usage for the thread executing the plugin.
        //! If the method is not implemented by a subclass, the default value
        //! is @link DEFAULT_STACK_USAGE @endlink (128 kB).
        //! @return The maximum stack usage in bytes for the thread executing the plugin.
        //!
        virtual size_t stackUsage() const {return DEFAULT_STACK_USAGE;}

        //!
        //! The main application invokes getOptions() only once, at application startup.
        //! Optionally implemented by subclasses to analyze the command line options.
        //! A plugin may ignore getOptions() and analyzes the command line options as
        //! part of the start() method. However, if a plugin is started later, command
        //! line errors may be reported too late.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool getOptions() {return true;}

        //!
        //! The main application invokes start() to start the plugin.
        //! Optionally implemented by subclasses.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool start() {return true;}

        //!
        //! The main application invokes stop() to terminate the plugin.
        //! Optionally implemented by subclasses.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool stop() {return true;}

        //!
        //! Get the plugin bitrate.
        //!
        //! The main application may invoke getBitrate() at any time.
        //! The semantics depends on the capability.
        //! - Input plugin: Returns the current input bitrate of the device.
        //!   Useful for real-time devices only.
        //! - Output plugin: Returns the current output bitrate which is used
        //!   by the device.
        //! - Packet processing plugin: Return the current bitrate at the output
        //!   of the packet processor. This can be used by packet processors
        //!   which influence the bitrate by removing packets or introducing
        //!   delays.
        //!
        //! Optionally implemented by subclasses. By default, return that the
        //! plugin is not aware of the bitrate.
        //!
        //! @return Plugin bitrate in bits/second. Shall return 0 on error or unknown bitrate.
        //!
        virtual BitRate getBitrate() {return 0;}

        //!
        //! Tell if the plugin is a real time one.
        //!
        //! Some plugin behave more accurately when the responsiveness of the
        //! environment is more accurate. Typically, input and output on tuners,
        //! modulators or ASI devices are real-time plugins. On the opposite,
        //! working on offline disk files is not.
        //!
        //! This method shall be implemented by real-time plugins and shall return
        //! true. The default implementation returns false.
        //!
        //! A plugin should be defined as real-time by design, not based on the
        //! interpretation of command-line parameters. Typically, the method
        //! isRealTime() is invoked before starting the plugin and consequently
        //! before the plugin has the opportunity to analyze its command-line
        //! parameters.
        //!
        //! @return True if the plugin usually requires real-time responsiveness.
        //!
        virtual bool isRealTime() {return false;}

        //!
        //! Get the plugin type.
        //! @return The plugin type.
        //!
        virtual PluginType type() const = 0;

        //!
        //! Constructor.
        //!
        //! @param [in] to_tsp Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //!
        Plugin(TSP* to_tsp, const UString& description = UString(), const UString& syntax = UString());

        //!
        //! Virtual destructor.
        //!
        virtual ~Plugin() override {}

    protected:
        TSP* tsp; //!< The TSP callback structure can be directly accessed by subclasses.

        // Report implementation.
        virtual void writeLog(int severity, const UString& message) override;

    private:
        // Inaccessible operations
        Plugin() = delete;
        Plugin(const Plugin&) = delete;
        Plugin& operator=(const Plugin&) = delete;
    };


    //-------------------------------------------------------------------------
    //! Input @c tsp plugin interface.
    //-------------------------------------------------------------------------
    //!
    //! @ingroup plugin
    //!
    //! All shared libraries providing input capability shall return
    //! an object implementing this abstract interface.
    //!
    class TSDUCKDLL InputPlugin : public Plugin
    {
    public:
        //!
        //! Packet reception interface.
        //!
        //! The main application invokes receive() to get input packets.
        //! This method reads complete 188-byte TS packets in
        //! the buffer (never read partial packets).
        //!
        //! @param [out] buffer Address of the buffer for incoming packets.
        //! @param [in] max_packets Size of @a buffer in number of packets.
        //! @return The number of actually received packets (in the range
        //! 1 to @a max_packets). Returning zero means error or end of input.
        //!
        virtual size_t receive(TSPacket* buffer, size_t max_packets) = 0;

        //!
        //! Abort the input operation currently in progress.
        //!
        //! This method is typically invoked from another thread when the input
        //! plugin is waiting for input. When this method is invoked, the plugin
        //! shall abort the current input and place the input plugin in some
        //! "error" or "end of input" state. The only acceptable operation
        //! after an abortInput() is a stop().
        //!
        //! @return True when the operation was properly handled. False in case
        //! of fatal error or if not supported by the plugin.
        //!
        virtual bool abortInput() { return false; }

        //!
        //! Constructor.
        //!
        //! @param [in] tsp_ Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //!
        InputPlugin(TSP* tsp_, const UString& description = UString(), const UString& syntax = UString());

        //!
        //! Virtual destructor.
        //!
        virtual ~InputPlugin() override {}

        // Implementation of inherited interface.
        virtual PluginType type() const override { return INPUT_PLUGIN; }

    private:
        // Inaccessible operations
        InputPlugin() = delete;
        InputPlugin(const InputPlugin&) = delete;
        InputPlugin& operator=(const InputPlugin&) = delete;
    };

    //!
    //! Input plugin interface profile.
    //!
    //! All shared libraries providing input capability shall export
    //! a global function named @c tspNewInput with the following profile.
    //!
    //! @param [in] tsp Associated callback to @c tsp executable.
    //! @return A new allocated object implementing ts::InputPlugin.
    //!
    typedef InputPlugin* (*NewInputProfile)(TSP* tsp);


    //-------------------------------------------------------------------------
    //! Output @c tsp plugin interface.
    //-------------------------------------------------------------------------
    //!
    //! @ingroup plugin
    //!
    //! All shared libraries providing output capability shall return
    //! an object implementing this abstract interface.
    //!
    class TSDUCKDLL OutputPlugin : public Plugin
    {
    public:
        //!
        //! Packet output interface.
        //!
        //! The main application invokes send() to output packets.
        //! This methods writes complete 188-byte TS packets.
        //!
        //! @param [in] buffer Address of outgoing packets.
        //! @param [in] packet_count Number of packets to send from @a buffer.
        //! @return True on success, false on error.
        //!
        virtual bool send(const TSPacket* buffer, size_t packet_count) = 0;

        //!
        //! Constructor.
        //!
        //! @param [in] tsp_ Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //!
        OutputPlugin(TSP* tsp_, const UString& description = UString(), const UString& syntax = UString());

        //!
        //! Virtual destructor.
        //!
        virtual ~OutputPlugin() override {}

        // Implementation of inherited interface.
        virtual PluginType type() const override { return OUTPUT_PLUGIN; }

    private:
        // Inaccessible operations
        OutputPlugin() = delete;
        OutputPlugin(const OutputPlugin&) = delete;
        OutputPlugin& operator=(const OutputPlugin&) = delete;
    };

    //!
    //! Output plugin interface profile.
    //!
    //! All shared libraries providing output capability shall export
    //! a global function named @c tspNewOutput with the following profile.
    //!
    //! @param [in] tsp Associated callback to @c tsp executable.
    //! @return A new allocated object implementing ts::OutputPlugin.
    //!
    typedef OutputPlugin* (*NewOutputProfile)(TSP* tsp);


    //-------------------------------------------------------------------------
    //! Packet processing @c tsp plugin interface.
    //-------------------------------------------------------------------------
    //!
    //! @ingroup plugin
    //!
    //! All shared libraries providing packet processing capability shall return
    //! an object implementing this abstract interface.
    //!
    class TSDUCKDLL ProcessorPlugin : public Plugin
    {
    public:
        //!
        //! Status of a packet processing.
        //! Returned by processPacket() after processing one packet.
        //!
        enum Status {
            TSP_OK = 0,    //!< OK, pass packet to next processor or output.
            TSP_END = 1,   //!< End of processing, tell everybody to terminate.
            TSP_DROP = 2,  //!< Drop this packet.
            TSP_NULL = 3   //!< Replace this packet with a null packet.
        };

        //!
        //! Packet processing interface.
        //!
        //! The main application invokes processPacket() to let the shared
        //! library process one TS packet.
        //!
        //! Dropping packets affect the output bitrate if the output device is
        //! a real-time one. With such devices, it is better to replace the
        //! undesired packet with a null packet.
        //!
        //! Dropping a packet or changing its PID (including replacing a packet
        //! with a null one) affects the continuity counters of the other
        //! packets of the original PID.
        //!
        //! @param [in,out] pkt The TS packet to process.
        //! @param [in,out] flush Initially set to false. If the method sets @a flush to true,
        //! the packet and all previously processed and buffered packets should be passed to the
        //! next processor as soon as possible.
        //! @param [in,out] bitrate_changed Initially set to false. If the method sets
        //! @a bitrate_changed to true, tsp should call the getBitrate() callback as soon as possible.
        //! @return The processing status.
        //!
        virtual Status processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed) = 0;

        //!
        //! Constructor.
        //!
        //! @param [in] tsp_ Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //!
        ProcessorPlugin(TSP* tsp_, const UString& description = UString(), const UString& syntax = UString());

        //!
        //! Virtual destructor.
        //!
        virtual ~ProcessorPlugin() override {}

        // Implementation of inherited interface.
        virtual PluginType type() const override { return PROCESSOR_PLUGIN; }

    private:
        // Inaccessible operations
        ProcessorPlugin() = delete;
        ProcessorPlugin(const ProcessorPlugin&) = delete;
        ProcessorPlugin& operator=(const ProcessorPlugin&) = delete;
    };

    //!
    //! Packet processing plugin interface profile.
    //!
    //! All shared libraries providing packet processing shall export
    //! a global function named @c tspNewProcessor with the following profile.
    //!
    //! @param [in] tsp Associated callback to @c tsp executable.
    //! @return A new allocated object implementing ts::ProcessorPlugin.
    //!
    typedef ProcessorPlugin* (*NewProcessorProfile)(TSP* tsp);
}


//----------------------------------------------------------------------------
//  Helper macros for shared libraries
//----------------------------------------------------------------------------

//!
//! Export the plugin API version number out of the shared library.
//! All @c tsp plugin shared libraries must invoke this macro once.
//! @hideinitializer
//!
#if defined(DOXYGEN) || !defined(TSDUCK_STATIC_PLUGINS)
#define TSPLUGIN_DECLARE_VERSION                        \
    extern "C" {                                        \
        /** @cond nodoxygen */                          \
        TS_DLL_EXPORT                                   \
        int tspInterfaceVersion = ts::TSP::API_VERSION; \
        /** @endcond */                                 \
    }
#else
#define TSPLUGIN_DECLARE_VERSION
#endif

// Support macro for plugin declaration macros.
#if !defined(DOXYGEN)
#if defined(TSDUCK_STATIC_PLUGINS)
#define _TSPLUGIN_DECLARE_PLUGIN(name,type,suffix) \
    namespace {                                    \
        /** @cond nodoxygen */                     \
        ts::suffix##Plugin* tspNew##suffix(ts::TSP* tsp) { return new type(tsp); } \
        TS_UNUSED ts::PluginRepository::Register tspRegister##suffix(#name, &tspNew##suffix); \
        /** @endcond */                            \
    }
#else
#define _TSPLUGIN_DECLARE_PLUGIN(name,type,suffix) \
    extern "C" {                                   \
        /** @cond nodoxygen */                     \
        TS_DLL_EXPORT ts::suffix##Plugin* tspNew##suffix(ts::TSP* tsp) { return new type(tsp); } \
        /** @endcond */                            \
    }
#endif
#endif

//!
//! Export input plugin interface out of the shared library.
//! This macro declares the plugin allocation routine.
//! Shall be used by shared libraries which provide input capability.
//! @param name Plugin name. Only used with static link.
//! @param type Name of a subclass of ts::InputPlugin implementing the plugin.
//! @hideinitializer
//!
#define TSPLUGIN_DECLARE_INPUT(name,type) _TSPLUGIN_DECLARE_PLUGIN(name,type,Input)

//!
//! Export output plugin interface out of the shared library.
//! This macro declares the plugin allocation routine.
//! Shall be used by shared libraries which provide output capability.
//! @param name Plugin name. Only used with static link.
//! @param type Name of a subclass of ts::OutputPlugin implementing the plugin.
//! @hideinitializer
//!
#define TSPLUGIN_DECLARE_OUTPUT(name,type) _TSPLUGIN_DECLARE_PLUGIN(name,type,Output)

//!
//! Export packet processing plugin interface out of the shared library.
//! This macro declares the plugin allocation routine.
//! Shall be used by shared libraries which provide packet processing capability.
//! @param name Plugin name. Only used with static link.
//! @param type Name of a subclass of ts::ProcessorPlugin implementing the plugin.
//! @hideinitializer
//!
#define TSPLUGIN_DECLARE_PROCESSOR(name,type) _TSPLUGIN_DECLARE_PLUGIN(name,type,Processor)
