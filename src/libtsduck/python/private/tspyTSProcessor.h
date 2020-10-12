//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2020, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup python
//!  TSDuck Python bindings: encapsulates TSProcessor objects for Python.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tspy.h"

TS_PUSH_WARNING()
TS_MSC_NOWARNING(4091) // '__declspec(dllexport)': ignored on left of 'struct type' when no variable is declared

//!
//! Create a new instance of TSProcessor.
//! @param [in] report A previously allocated instance of Report.
//! @return A new TSProcessor instance.
//!
TSDUCKPY void* tspyNewTSProcessor(void* report);

//!
//! Delete a previously allocated instance of TSProcessor.
//! @param [in] tsp A previously allocated instance of TSProcessor.
//!
TSDUCKPY void tspyDeleteTSProcessor(void* tsp);

//!
//! Argument structure (plain C structure) for start parameters.
//! Use same names as ts::TSProcessorArgs but only long's to avoid interface issues.
//!
TSDUCKPY struct tspyTSProcessorArgs
{
    long monitor;                  //!< Run a resource monitoring thread (bool).
    long ignore_joint_termination; //!< Ignore "joint termination" options in plugins (bool).
    long buffer_size;              //!< Size in bytes of the global TS packet buffer.
    long max_flushed_packets;      //!< Max processed packets before flush.
    long max_input_packets;        //!< Max packets per input operation.
    long initial_input_packets;    //!< Initial number of input packets to read before starting the processing (zero means default).
    long add_input_stuffing_0;     //!< Add input stuffing: add @a add_input_stuffing_0 null packets ...
    long add_input_stuffing_1;     //!< ... every @a add_input_stuffing_1 input packets.
    long add_start_stuffing;       //!< Add null packets before actual input.
    long add_stop_stuffing;        //!< Add null packets after end of actual input.
    long bitrate;                  //!< Fixed input bitrate (user-specified).
    long bitrate_adjust_interval;  //!< Bitrate adjust interval in (milliseconds).
    long receive_timeout;          //!< Timeout on input operations (in milliseconds).
};

//!
//! Start the TS processing.
//! @param [in] tsp A previously allocated instance of TSProcessor.
//! @param [in] args TS processing options.
//! @param [in] plugins Address of a buffer containing a UTF-16 string with all plugins.
//! Strings are separated with fake character 0xFFFF. First string is application name.
//! Strings '-I', '-P' and '-O' identify plugin types.
//! @param [in] plugins_size Size in bytes of the @a plugins buffer.
//! @return True on success, false on failure to start.
//!
TSDUCKPY bool tspyStartTSProcessor(void* tsp, const tspyTSProcessorArgs* args, const uint8_t* plugins, size_t plugins_size);

//!
//! Abort the processing.
//! @param [in] tsp A previously allocated instance of TSProcessor.
//!
TSDUCKPY void tspyAbortTSProcessor(void* tsp);

//!
//! Suspend the calling thread until TS processing is completed.
//! @param [in] tsp A previously allocated instance of TSProcessor.
//!
TSDUCKPY void tspyWaitTSProcessor(void* tsp);

TS_POP_WARNING()
