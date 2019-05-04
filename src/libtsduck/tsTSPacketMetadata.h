//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  Metadata of an MPEG-2 transport packet for tsp plugins.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"

namespace ts {
    //!
    //! Metadata of an MPEG-2 transport packet for tsp plugins.
    //! @ingroup plugin
    //!
    //! An instance of this class is passed with each TS packet to packet processor plugins.
    //!
    class TSDUCKDLL TSPacketMetadata final
    {
    public:
        //!
        //! Maximum numbers of labels per TS packet.
        //! A plugin can set label numbers, from 0 to 31, to any packet.
        //! Other plugins, downward in the processing chain, can check the labels of the packet.
        //!
        static constexpr size_t LABEL_COUNT = 32;

        //!
        //! A bit mask of labels.
        //!
        typedef uint32_t LabelMask;

        //!
        //! A bitmask of labels where all labels are set.
        //!
        static constexpr LabelMask ALL_LABELS = ~(LabelMask(0));

        //!
        //! Constructor.
        //!
        TSPacketMetadata();

        //!
        //! Reset the content of this instance.
        //! Return to initial empty state.
        //!
        void reset();

        //!
        //! Specify if the packet was artificially inserted as input stuffing.
        //! @param [in] on When true, the packet was artificially inserted as input stuffing.
        //!
        void setInputStuffing(bool on) { _input_stuffing = on; }

        //!
        //! Check if the packet was artificially inserted as input stuffing.
        //! @return True when the packet was artificially inserted as input stuffing.
        //!
        bool getInputStuffing() const { return _input_stuffing; }

        //!
        //! Specify if the packet chain shall be flushed by tsp as soon as possible.
        //! @param [in] on When set to true by a packet processing plugin, the packet and all previously
        //! processed and buffered packets should be passed to the next processor as soon as possible.
        //!
        void setFlush(bool on) { _flush = on; }

        //!
        //! Check if the packet chain shall be flushed by tsp as soon as possible.
        //! @return True when the packet and all previously processed and buffered packets should be
        //! passed to the next processor as soon as possible
        //!
        bool getFlush() const { return _flush; }

        //!
        //! Specify if the plugin has changed the transport stream bitrate.
        //! @param [in] on When set to true by a packet processing plugin, tsp should call its
        //! getBitrate() callback as soon as possible.
        //!
        void setBitrateChanged(bool on) { _bitrate_changed = on; }

        //!
        //! Check if the plugin has changed the transport stream bitrate.
        //! @return True when tsp should call the getBitrate() callback of the plugin as soon as possible.
        //!
        bool getBitrateChanged() const { return _bitrate_changed; }

        //!
        //! Check if the TS packet has a specific label set.
        //! @param [in] label The label to check.
        //! @return True if the TS packet has @a label set.
        //!
        bool hasLabel(size_t label) const { return (_labels & (1 << label)) != 0; }

        //!
        //! Check if the TS packet has any label set from a set of labels.
        //! @param [in] mask The mask of labels to check.
        //! @return True if the TS packet has any label from @a mask.
        //!
        bool hasAnyLabel(LabelMask mask = ALL_LABELS) const { return (_labels & mask) != 0; }

        //!
        //! Set a specific label for the TS packet.
        //! @param [in] label The label to set.
        //!
        void setLabel(size_t label) { _labels |= (1 << label); }

        //!
        //! Set a specific set of labels for the TS packet.
        //! @param [in] mask The mask of labels to set.
        //!
        void setAllLabels(LabelMask mask = ALL_LABELS) { _labels |= mask; }

        //!
        //! Clear a specific label for the TS packet.
        //! @param [in] label The label to clear.
        //!
        void clearLabel(size_t label) { _labels &= ~(1 << label); }

        //!
        //! Clear a specific set of labels for the TS packet.
        //! @param [in] mask The mask of labels to clear.
        //!
        void clearAllLabels(LabelMask mask = ALL_LABELS) { _labels &= ~mask; }

    private:
        LabelMask _labels;           // Bit mask of labels.
        bool      _flush;            // Flush the packet buffer asap.
        bool      _bitrate_changed;  // Call getBitrate() callback as soon as possible.
        bool      _input_stuffing;   // Packet was artificially inserted as input stuffing.
    };
}
