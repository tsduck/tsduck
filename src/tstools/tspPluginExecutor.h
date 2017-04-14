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
//
//  Transport stream processor: Execution context of a plugin
//
//
//  Data model
//  ----------
//  There is a global buffer for TS packets. The input thread writes incoming
//  packets here. All packet processors update them and the output thread
//  picks them for the same place.
//
//  The buffer is an array of ts::TSPacket. It is managed in a circular way.
//  It is divided into logical areas, one per processor (including input
//  and output). These logical areas are sliding windows which move when
//  packets are processed.
//
//  Each sliding window is defined by the index of its first packet
//  (_pkt_first) and its size in packets (_pkt_cnt).
//
//  Flat (non-circular) view of the buffer:
//
//        output->_pkt_first            proc_1->_pkt_first
//        |                             |
//        |          proc_N->_pkt_first |          input->_pkt_first
//        |          |                  |          |
//        V          V                  V          V
//       +----------+----------+-------+----------+---------+
//  +->  |  output  |  proc N  |  ...  |  proc 1  |  input  |  ->-+
//  |    +----------+----------+-------+----------+---------+     |
//  |                                                             |
//  +-------------------------------------------------------------+
//
//  When a thread terminates the processing of a bunch of packets, it moves
//  up its first index and, consequently, decreases the sizes of its area
//  and accordingly increases the size of the area of the next processor.
//
//  The modification of the starting index and size of any area must be
//  performed under the protection of the global mutex. There is one global
//  mutex for simplicity. The resulting bottleneck is not so important since
//  updating a few pointers is fast.
//
//  When the sliding window of a processor is empty, the processor thread
//  sleeps on its "_to_do" condition variable. Consequently, when a thread
//  passes packets to the next processor (ie. increases the size of the sliding
//  window of the next processor), it must notify the _to_do condition variable
//  of the next thread.
//
//  When a packet processor decides to drop a packet, the synchronization
//  byte (first byte of the packet, normally 0x47) is reset to zero. When
//  a packet processor or the output processor encounters a packet starting
//  with a zero byte, it ignores it.
//
//  All PluginExecutors are chained in a ring. The first one is input and
//  the last one is output. The output points back to the input so that the
//  output processor can easily pass free packets to be reused by the input
//  processor.
//
//  The "_input_end" indicates that there is no more packet to process
//  after those in the processor's area. This condition is signaled by
//  the previous processor in the chain. All processors, except the output
//  processor, may signal this condition to their successor.
//
//  The "_aborted" indicates that the current processor has encountered an
//  error and has ceased to accept packets. This condition is checked by
//  the previous processor in the chain (which, in turn, will declare itself
//  as aborted). All processors, except the input processor may signal this
//  condition. In case of error, all processors should also declare an
//  "_input_end" to their successor.
//
//----------------------------------------------------------------------------

#pragma once
#include "tspOptions.h"
#include "tspJointTermination.h"
#include "tsPlugin.h"
#include "tsPluginSharedLibrary.h"
#include "tsResidentBuffer.h"
#include "tsUserInterrupt.h"
#include "tsRingNode.h"
#include "tsCondition.h"
#include "tsMutex.h"
#include "tsThread.h"

namespace ts {
    namespace tsp {

        class PluginExecutor:
            public RingNode,
            public JointTermination,
            public Thread,
            public PluginSharedLibrary
        {
        public:
            // TS packet are accessed in a memory-resident buffer.
            typedef ResidentBuffer<TSPacket> PacketBuffer;

            // Constructor
            PluginExecutor(Options* options,
                           const Options::PluginOptions* pl_options,
                           const ThreadAttributes& attributes,
                           Mutex& global_mutex);

            // Destructor
            virtual ~PluginExecutor();

            // Set the initial state of the buffer. Must be executed in
            // synchronous environment, before starting all executor threads.
            void initBuffer(PacketBuffer* buffer,     // Packet buffer
                            size_t        pkt_first,  // Starting index of packets area
                            size_t        pkt_cnt,    // Size of packets area
                            bool          input_end,  // No more packet after current ones
                            bool          aborted,    // Processor error, aborted
                            BitRate       bitrate);   // Input bitrate (set by previous proc)

            // Change the report method
            void setReport (ReportInterface* rep) {_report = rep;}
            
            // This method sets the current processor in an abort state.
            void setAbort();

            // Each plugin defines its own usage of the stack. The PluginExector
            // class and its subclasses have their own addition stack usage.
            static const size_t STACK_SIZE_OVERHEAD = 32 * 1024; // 32 kB

            // Access the shared library API
            Plugin* plugin() {return _shlib;}

        protected:
            std::string   _name;            // Plugin name
            Plugin*       _shlib;           // Shared library API
            PacketBuffer* _buffer;          // Description of shared packet buffer

            // This method signals that the specified number of packets have been
            // processed by this processor."Input_end" means "this processor will
            // no longer produce packets" and "aborted" means "this processor has
            // encountered an error and will cease to accept packets".
            void passPackets (size_t count,     // of packets to pass to next processor
                              BitRate bitrate,  // pass to next processor
                              bool input_end,   // pass to next processor
                              bool aborted);    // set to current processor

            // This method makes the calling processor thread waiting for packets
            // to process or some error condition. Always return a contiguous array
            // of packets. If the circular buffer wrap-over occurs in the middle of
            // the caller's area, only return the first part, up the buffer's highest
            // address. The next call to wait_work will return the second part.
            void waitWork (size_t& pkt_first,
                           size_t& pkt_cnt,
                           BitRate& bitrate,
                           bool& input_end,
                           bool& aborted);    // get from next processor

            // Inherited from ReportInterface (via TSP)
            virtual void writeLog (int severity, const std::string& msg);

        private:
            ReportInterface* _report;   // Common report interface for all plugins
            Condition        _to_do;    // Notify processor to do something

            // The following private data must be accessed exclusively under the
            // protection of the global mutex.
            size_t  _pkt_first;  // Starting index of packets area
            size_t  _pkt_cnt;    // Size of packets area
            bool    _input_end;  // No more packet after current ones
            BitRate _bitrate;    // Input bitrate (set by previous plugin)
        };
    }
}
