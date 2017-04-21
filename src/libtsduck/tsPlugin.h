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
    // TSP callback: Each plugin has an associated TSP object to communicate
    // with the Transport Stream Processor main executable.
    //
    // A shared library must exclusively use the tsp object for text
    // display and must never use std::cout, ::printf or alike. When
    // called in multi-threaded context, the supplied tsp object is
    // thread-safe and asynchronous (the methods return to the
    // caller without waiting for the message to be printed).
    //-------------------------------------------------------------------------

    class TSDUCKDLL TSP: public ReportInterface, public AbortInterface
    {
    public:

        // API Version Number
        //
        // Important: Must be incremented each time the tsp plugin abstract
        // interfaces are modified. All shared libraries shall export a global
        // int data named "tspInterfaceVersion" which contains the current
        // interface version at the time the library is built.
        static const int API_VERSION = 5;

        // Current input bitrate in b/s (if known).
        BitRate bitrate() const {return _tsp_bitrate;}

        // The plugin may invoke this method to check if the application is
        // aborting for some reason (user interrupt for instance).
        virtual bool aborting() const {return _tsp_aborting;}

        // "Joint termination" support:
        //
        // A plugin can decide to terminate tsp on its own (returning end of
        // input, output error or TSP_END). The termination is unconditional,
        // regardless of the state of the other plugins.
        //
        // The idea behind "joint termination" is to terminate tsp when several
        // plugins have jointly terminated their processing.
        //
        // First, a plugin must decide to use "joint termination". This is
        // usually done in method start(), using useJointTermination (bool)
        // when the option --joint-termination is specified on the command line.
        //
        // When the plugin has completed its work, it reports this using
        // jointTerminate().

        // This method activates or deactivates "joint termination" for the
        // calling plugin. It should be invoked during the plugin's start().
        virtual void useJointTermination(bool on) = 0;

        // This method is used by the plugin to declare that its execution is
        // potentially terminated in the context of "joint termination".
        // After invoking this method, any packet which is processed by
        // the plugin may be ignored by tsp.
        virtual void jointTerminate() = 0;

        // Check if the calling plugin uses "joint termination".
        virtual bool useJointTermination() const = 0;

        // Check if the calling plugin has already declared "joint termination".
        virtual bool thisJointTerminated() const = 0;

    protected:
        BitRate _tsp_bitrate;
        bool volatile _tsp_aborting;

        // Constructor is reserved to subclasses
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
    // Base class of all plugins.
    // Plugin is a subclass of Args; each constructor is expected to define
    // the syntax, help and option definitions for the command line.
    //-------------------------------------------------------------------------

    class TSDUCKDLL Plugin: public Args
    {
    public:

        // A shared library is invoked into a multi-threaded environment.
        // It must be thread-safe. It may define its maximum stack usage
        // using the method stackUsage. The returned value is in bytes.
        // If the method is not implemented, the following default value
        // of 128 kB is used.

        static const size_t DEFAULT_STACK_USAGE = 128 * 1024;

        virtual size_t stackUsage() const {return DEFAULT_STACK_USAGE;}

        // The main application invokes start() to start the plugin.
        // The command-line arguments have been previously loaded and
        // analyzed by the main application using one of the
        // Args::analyze() methods of the plugin.
        // Shall return true on success, false on error.

        virtual bool start() = 0;

        // The main application will invoke stop to terminate the plugin.
        // Shall return true on success, false on error.

        virtual bool stop() = 0;

        // The main application may invoke getBitrate at any time.
        // The semantic depends on the capability.
        // - Input: Returns the current input bitrate of the device.
        //   Useful for real-time devices only.
        // - Output: Returns the current output bitrate which is used
        //   by the device.
        // - Packet processing: Return the current bitrate at the output
        //   of the packet processor. This can be used by packet processors
        //   which influence the bitrate by removing packets or introducing
        //   delays.
        // The returned value is in bits/second.
        // Shall return 0 on error or unknown bitrate.

        virtual BitRate getBitrate() = 0;

        // Constructor/ destructor

        Plugin(TSP* tsp_,
               const std::string& description_ = "",
               const std::string& syntax_ = "",
               const std::string& help_ = "") :
            Args(description_, syntax_, help_), tsp (tsp_) {}

        virtual ~Plugin() {}

    protected:
        // The TSP callback structure can be directly accessed by subclasses.
        TSP* tsp;

        // Force message to go through tsp
        virtual void writeLog(int severity, const std::string& message)
        {
            tsp->log (severity, message);
        }

    private:
        // Inaccessible operations
        Plugin() = delete;
        Plugin(const Plugin&) = delete;
        Plugin& operator=(const Plugin&) = delete;
    };


    //-------------------------------------------------------------------------
    // Input libraries interface.
    // All shared libraries providing input capability shall return
    // an object implementing the following abstract interface.
    //-------------------------------------------------------------------------

    class TSDUCKDLL InputPlugin : public Plugin
    {
    public:

        // The main application invokes receive to get input packets.
        // This method reads complete 188-byte TS packets in
        // the buffer (never read partial packets) and return
        // the number of actually received packets (in the range
        // 1 to max_packets). Returning zero means error or end
        // of input.

        virtual size_t receive(TSPacket* buffer, size_t max_packets) = 0;

        // Constructor/ destructor

        InputPlugin(TSP* tsp_,
                    const std::string& description_ = "",
                    const std::string& syntax_ = "",
                    const std::string& help_ = "") :
            Plugin(tsp_, description_, syntax_, help_) {}

        virtual ~InputPlugin() {}
    };

    // All shared libraries providing input capability shall export
    // a global function named "tspNewInput" with the following profile.
    // When invoked, it shall allocate a new object implementing
    // ts::InputPlugin.

    typedef InputPlugin* (*NewInputProfile)(const TSP*);


    //-------------------------------------------------------------------------
    // Output libraries interface
    // All shared libraries providing output capability shall return
    // an object implementing the following abstract interface.
    //-------------------------------------------------------------------------

    class TSDUCKDLL OutputPlugin : public Plugin
    {
    public:

        // The main application invokes send to output packets.
        // This methods writes complete 188-byte TS packets.
        // Shall return true on success, false on error.

        virtual bool send(const TSPacket* buffer, size_t packet_count) = 0;

        // Constructor/ destructor

        OutputPlugin(TSP* tsp_,
                     const std::string& description_ = "",
                     const std::string& syntax_ = "",
                     const std::string& help_ = "") :
            Plugin(tsp_, description_, syntax_, help_) {}

        virtual ~OutputPlugin() {}
    };

    // All shared libraries providing output capability shall export
    // a global function named "tspNewOutput" with the following profile.
    // When invoked, it shall allocate a new object implementing
    // ts::OutputPlugin.

    typedef OutputPlugin* (*NewOutputProfile)(const TSP*);


    //-------------------------------------------------------------------------
    // Packet processing libraries interface
    // All shared libraries providing packet processing shall return
    // an object implementing the following abstract interface.
    //-------------------------------------------------------------------------

    class TSDUCKDLL ProcessorPlugin : public Plugin
    {
    public:

        // The main application invokes processPacket to let the shared
        // library process one TS packet.
        //
        // Notes:
        //   - Dropping packets affect the output bitrate if the output device is
        //     a real-time one. With such devices, it is better to replace the
        //     undesired packet with a null packet.
        //   - Dropping a packet or changing its PID (including replacing a packet
        //     with a null one) affects the continuity counters of the other
        //     packets of the original PID.
        //   - If the method sets flush to true, the packet and all
        //     previously processed and buffered packets should be passed to the
        //     next processor as soon as possible.
        //   - If the method sets bitrate_changed to true, tsp should call
        //     the getBitrate() callback as soon as possible.

        enum Status {
            TSP_OK   = 0, // OK, pass packet to next processor or output
            TSP_END  = 1, // End of processing, tell everybody to terminate
            TSP_DROP = 2, // Drop this packet.
            TSP_NULL = 3, // Replace packet with a null packet
        };

        virtual Status processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed) = 0;

        // Constructor/ destructor

        ProcessorPlugin(TSP* tsp_,
                        const std::string& description_ = "",
                        const std::string& syntax_ = "",
                        const std::string& help_ = "") :
            Plugin(tsp_, description_, syntax_, help_) {}

        virtual ~ProcessorPlugin() {}
    };

    // All shared libraries providing packet processing shall export
    // a global function named "tspNewProcessor" with the following profile.
    // When invoked, it shall allocate a new object implementing
    // ts::ProcessorPlugin.

    typedef ProcessorPlugin* (*NewProcessorProfile)(const TSP*);
}


//----------------------------------------------------------------------------
//  Helper macros for shared libraries
//----------------------------------------------------------------------------

// All shared libraries must invoke this macro once.

#define TSPLUGIN_DECLARE_VERSION                          \
    extern "C" {                                          \
        TS_DLL_EXPORT                                   \
        int tspInterfaceVersion = ts::TSP::API_VERSION; \
    }

// The following macros declare the plugin allocation routines.
// Shall be used by shared libraries which provide the corresponding
// capabilities.

#define TSPLUGIN_DECLARE_INPUT(type)                            \
    extern "C" {                                                \
        TS_DLL_EXPORT                                         \
        ts::InputPlugin* tspNewInput (ts::TSP* tsp)         \
        {                                                       \
            return new type (tsp);                              \
        }                                                       \
    }

#define TSPLUGIN_DECLARE_OUTPUT(type)                           \
    extern "C" {                                                \
        TS_DLL_EXPORT                                         \
        ts::OutputPlugin* tspNewOutput (ts::TSP* tsp)       \
        {                                                       \
            return new type (tsp);                              \
        }                                                       \
    }

#define TSPLUGIN_DECLARE_PROCESSOR(type)                        \
    extern "C" {                                                \
        TS_DLL_EXPORT                                         \
        ts::ProcessorPlugin* tspNewProcessor (ts::TSP* tsp) \
        {                                                       \
            return new type (tsp);                              \
        }                                                       \
    }
