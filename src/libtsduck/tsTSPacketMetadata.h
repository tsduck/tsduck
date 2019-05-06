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
        //! Maximum value for labels.
        //!
        static constexpr size_t LABEL_MAX = LABEL_COUNT - 1;

        //!
        //! A set of labels for TS packets.
        //!
        typedef std::bitset<LABEL_COUNT> LabelSet;

        //!
        //! A set of labels where all labels are cleared (no label).
        //!
        static const LabelSet NoLabel;

        //!
        //! A set of labels where all labels are set.
        //!
        static const LabelSet AllLabels;

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
        //! Specify if the packet was explicitly turned into a null packet by a plugin.
        //! @param [in] on When true, the packet was explicitly turned into a null packet by a plugin.
        //!
        void setNullified(bool on) { _nullified = on; }

        //!
        //! Check if the packet was explicitly turned into a null packet by a plugin.
        //! @return True when the packet was explicitly turned into a null packet by a plugin.
        //!
        bool getNullified() const { return _nullified; }

        //!
        //! Specify if the packet chain shall be flushed by tsp as soon as possible.
        //! This is typically called by a packet processing plugin.
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
        //! This is typically called by a packet processing plugin.
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
        bool hasLabel(size_t label) const { return _labels.test(label); }

        //!
        //! Check if the TS packet has any label set.
        //! @return True if the TS packet has any label.
        //!
        bool hasAnyLabel() const { return _labels.any(); }

        //!
        //! Check if the TS packet has any label set from a set of labels.
        //! @param [in] mask The mask of labels to check.
        //! @return True if the TS packet has any label from @a mask.
        //!
        bool hasAnyLabel(const LabelSet& mask) const;

        //!
        //! Check if the TS packet has all labels set from a set of labels.
        //! @param [in] mask The mask of labels to check.
        //! @return True if the TS packet has all labels from @a mask.
        //!
        bool hasAllLabels(const LabelSet& mask) const;

        //!
        //! Set a specific label for the TS packet.
        //! @param [in] label The label to set.
        //!
        void setLabel(size_t label) { _labels.set(label); }

        //!
        //! Set a specific set of labels for the TS packet.
        //! @param [in] mask The mask of labels to set.
        //!
        void setLabels(const LabelSet& mask);

        //!
        //! Clear a specific label for the TS packet.
        //! @param [in] label The label to clear.
        //!
        void clearLabel(size_t label) { _labels.reset(label); }

        //!
        //! Clear a specific set of labels for the TS packet.
        //! @param [in] mask The mask of labels to clear.
        //!
        void clearLabels(const LabelSet& mask);

        //!
        //! Clear all labels for the TS packet.
        //!
        void clearAllLabels() { _labels.reset(); }

    private:
        LabelSet _labels;           // Bit mask of labels.
        bool      _flush;            // Flush the packet buffer asap.
        bool      _bitrate_changed;  // Call getBitrate() callback as soon as possible.
        bool      _input_stuffing;   // Packet was artificially inserted as input stuffing.
        bool      _nullified;        // Packet was explicitly turned into a null packet by a plugin.
    };
}
