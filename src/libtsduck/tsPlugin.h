//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsReportInterface.h"
#include "tsTSPacket.h"

namespace ts {

    //-------------------------------------------------------------------------
    //! TSP callback for plugins.
    //-------------------------------------------------------------------------
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
    class TSDUCKDLL TSP: public ReportInterface, public AbortInterface
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
        static const int API_VERSION = 5;

        //!
        //! Get the current input bitrate in bits/seconds.
        //! @return The current input bitrate in bits/seconds or zero if unknown.
        //!
        BitRate bitrate() const {return _tsp_bitrate;}

        //!
        //! Check for aborting application.
        //!
        //! The plugin may invoke this method to check if the application is
        //! aborting for some reason (user interrupt for instance).
        //! @return True if the tsp application is currently aborting.
        //!
        virtual bool aborting() const {return _tsp_aborting;}

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
        BitRate       _tsp_bitrate;   //!< TSP input bitrate.
        volatile bool _tsp_aborting;  //!< TSP is currently aborting.

        //!
        //! Constructor for subclasses.
        //! @param [in] verbose If true, set initial report level to Verbose.
        //! @param [in] debug_level If greater than zero, set initial report to that level and ignore @a verbose.
        //!
        TSP(bool verbose, int debug_level) :
            ReportInterface(verbose, debug_level),
            _tsp_bitrate(0),
            _tsp_aborting(false)
        {
        }

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
        //! The main application invokes start() to start the plugin.
        //! The command-line arguments have been previously loaded and
        //! analyzed by the main application using one of the
        //! Args::analyze() methods of the plugin.
        //!
        //! Must be implemented by subclasses.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool start() = 0;

        //!
        //! The main application invokes stop() to terminate the plugin.
        //!
        //! Must be implemented by subclasses.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool stop() = 0;

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
        //! Must be implemented by subclasses.
        //! @return Plugin bitrate in bits/second. Shall return 0 on error or unknown bitrate.
        //!
        virtual BitRate getBitrate() = 0;

        //!
        //! Constructor.
        //!
        //! @param [in] to_tsp Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] help A multi-line string describing the usage of options and parameters.
        //!
        Plugin(TSP* to_tsp,
               const std::string& description = "",
               const std::string& syntax = "",
               const std::string& help = "") :
            Args(description, syntax, help), tsp(to_tsp) {}

        //!
        //! Virtual destructor.
        //!
        virtual ~Plugin() {}

    protected:
        TSP* tsp; //!< The TSP callback structure can be directly accessed by subclasses.

        // ReportInterface implementation.
        virtual void writeLog(int severity, const std::string& message)
        {
            // Force message to go through tsp
            tsp->log(severity, message);
        }

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
        //! Constructor.
        //!
        //! @param [in] tsp Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] help A multi-line string describing the usage of options and parameters.
        //!
        InputPlugin(TSP* tsp,
                    const std::string& description = "",
                    const std::string& syntax = "",
                    const std::string& help = "") :
            Plugin(tsp, description, syntax, help) {}

        //!
        //! Virtual destructor.
        //!
        virtual ~InputPlugin() {}
    };

    //!
    //! Input plugin interface profile.
    //!
    //! All shared libraries providing input capability shall export
    //! a global function named @c tspNewInput with the following profile.
    //! When invoked, it shall allocate a new object implementing
    //! ts::InputPlugin.
    //!
    //! @param [in] tsp Associated callback to @c tsp executable.
    //!
    typedef InputPlugin* (*NewInputProfile)(const TSP* tsp);


    //-------------------------------------------------------------------------
    //! Output @c tsp plugin interface.
    //-------------------------------------------------------------------------
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
        //! @param [in] tsp Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] help A multi-line string describing the usage of options and parameters.
        //!
        OutputPlugin(TSP* tsp,
                     const std::string& description = "",
                     const std::string& syntax = "",
                     const std::string& help = "") :
            Plugin(tsp, description, syntax, help) {}

        //!
        //! Virtual destructor.
        //!
        virtual ~OutputPlugin() {}
    };

    //!
    //! Output plugin interface profile.
    //!
    //! All shared libraries providing output capability shall export
    //! a global function named "tspNewOutput" with the following profile.
    //! When invoked, it shall allocate a new object implementing
    //! ts::OutputPlugin.
    //!
    //! @param [in] tsp Associated callback to @c tsp executable.
    //!
    typedef OutputPlugin* (*NewOutputProfile)(const TSP* tsp);


    //-------------------------------------------------------------------------
    //! Packet processing @c tsp plugin interface.
    //-------------------------------------------------------------------------
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
        //! @param [in] tsp Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] help A multi-line string describing the usage of options and parameters.
        //!
        ProcessorPlugin(TSP* tsp,
                        const std::string& description = "",
                        const std::string& syntax = "",
                        const std::string& help = "") :
            Plugin(tsp, description, syntax, help) {}

        //!
        //! Virtual destructor.
        //!
        virtual ~ProcessorPlugin() {}
    };

    //!
    //! Packet processing plugin interface profile.
    //!
    //! All shared libraries providing packet processing shall export
    //! a global function named "tspNewProcessor" with the following profile.
    //! When invoked, it shall allocate a new object implementing
    //! ts::ProcessorPlugin.
    //!
    //! @param [in] tsp Associated callback to @c tsp executable.
    //!
    typedef ProcessorPlugin* (*NewProcessorProfile)(const TSP* tsp);
}


//----------------------------------------------------------------------------
//  Helper macros for shared libraries
//----------------------------------------------------------------------------

//!
//! @hideinitializer
//! Export the plugin API version number out of the shared library.
//! All @c tsp plugin shared libraries must invoke this macro once.
//!
#define TSPLUGIN_DECLARE_VERSION                        \
    extern "C" {                                        \
        TS_DLL_EXPORT                                   \
        int tspInterfaceVersion = ts::TSP::API_VERSION; \
    }

//!
//! @hideinitializer
//! Export input plugin interface out of the shared library.
//! This macro declares the plugin allocation routine.
//! Shall be used by shared libraries which provide input capability.
//! @param type Name of a subclass of ts::InputPlugin implementing the plugin.
//!
#define TSPLUGIN_DECLARE_INPUT(type)               \
    extern "C" {                                   \
        TS_DLL_EXPORT                              \
        ts::InputPlugin* tspNewInput(ts::TSP* tsp) \
        {                                          \
            return new type(tsp);                  \
        }                                          \
    }

//!
//! @hideinitializer
//! Export output plugin interface out of the shared library.
//! This macro declares the plugin allocation routine.
//! Shall be used by shared libraries which provide output capability.
//! @param type Name of a subclass of ts::OutputPlugin implementing the plugin.
//!
#define TSPLUGIN_DECLARE_OUTPUT(type)                 \
    extern "C" {                                      \
        TS_DLL_EXPORT                                 \
        ts::OutputPlugin* tspNewOutput(ts::TSP* tsp)  \
        {                                             \
            return new type(tsp);                     \
        }                                             \
    }

//!
//! @hideinitializer
//! Export packet processing plugin interface out of the shared library.
//! This macro declares the plugin allocation routine.
//! Shall be used by shared libraries which provide packet processing capability.
//! @param type Name of a subclass of ts::ProcessorPlugin implementing the plugin.
//!
#define TSPLUGIN_DECLARE_PROCESSOR(type)                   \
    extern "C" {                                           \
        TS_DLL_EXPORT                                      \
        ts::ProcessorPlugin* tspNewProcessor(ts::TSP* tsp) \
        {                                                  \
            return new type(tsp);                          \
        }                                                  \
    }
