//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//
//  DirectShow filter for DVB tuners capture, Windows-specific.
//  With many ideas taken from VLC and Microsoft Windows SDK samples.
//
//-----------------------------------------------------------------------------
//
//  This module implements a DirectShow filter.
//
//  DirectShow is a very complicated infrastructure on Windows to support
//  various media processing. BDA (Broadcast Device Architecture) is the
//  generic device driver interface which links "broadcast devices" like
//  DVB receivers to DirectShow. DirectShow is consequently the only generic
//  way to interact with any type of DVB receiver hardware, provided that
//  the hardware vendor supplies BDA-compatible drivers for the device.
//
//  The "sink filter" in this module is intended to be used after a DirectShow
//  capture filter, as provided by the hardware vendor. We call it a "sink"
//  filter because it has one input pin (for MPEG-2 TS) but no output pin.
//  The TS "samples" are read asynchronously by the application. This filter
//  acts as an adapter between the push model of DirectShow and the pull model
//  of tsp, the transport stream processor.
//
//  This module contains several classes:
//
//  - SinkFilter         : The DirectShow filter
//  - SinkPin            : Input pin for SinkFilter
//  - SinkEnumMediaTypes : Enumerator returned by ::IPin::EnumMediaTypes
//  - SinkEnumPins       : Enumerator returned by ::IBaseFilter::EnumPins
//
//  The SinkPin accepts only MPEG-2 transport streams:
//
//  - Major type : MEDIATYPE_Stream
//  - Subtype    : MEDIASUBTYPE_MPEG2_TRANSPORT
//                 MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE
//                 KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT

#pragma once
#include "tsMutex.h"
#include "tsCondition.h"
#include "tsReportInterface.h"

namespace ts {

    class SinkFilter;
    class SinkPin;
    class SinkEnumMediaTypes;
    class SinkEnumPins;

    //-------------------------------------------------------------------------
    // SinkFilter, the DirectShow filter
    //-------------------------------------------------------------------------

    class SinkFilter : public ::IBaseFilter
    {
    public:
        SinkFilter (ReportInterface&);
        virtual ~SinkFilter();

        // Implementation of ::IUnknown
        STDMETHODIMP QueryInterface (REFIID riid, void** ppv);
        STDMETHODIMP_(::ULONG) AddRef();
        STDMETHODIMP_(::ULONG) Release();

        // Implementation of ::IPersist
        STDMETHODIMP GetClassID (::CLSID* pClsID);

        // Implementation of ::IMediaFilter
        STDMETHODIMP GetState (::DWORD dwMSecs, ::FILTER_STATE* State);
        STDMETHODIMP SetSyncSource (::IReferenceClock* pClock);
        STDMETHODIMP GetSyncSource (::IReferenceClock** pClock);
        STDMETHODIMP Stop();
        STDMETHODIMP Pause();
        STDMETHODIMP Run (::REFERENCE_TIME tStart);

        // Implementation of ::IBaseFilter
        STDMETHODIMP EnumPins (::IEnumPins** ppEnum);
        STDMETHODIMP FindPin (::LPCWSTR Id, ::IPin** ppPin);
        STDMETHODIMP QueryFilterInfo (::FILTER_INFO* pInfo);
        STDMETHODIMP JoinFilterGraph (::IFilterGraph* pGraph, ::LPCWSTR pName);
        STDMETHODIMP QueryVendorInfo (::LPWSTR* pVendorInfo);

        // Return input pin (with one reference => use Release)
        SinkPin* GetPin();

        // Set the max number of media samples in the queue between
        // the graph thread and the application thread. Must be called
        // when the graph is stopped or paused.
        void SetMaxMessages (size_t maxMessages);

        // Discard and release all pending media samples
        void Flush();

        // Read data from transport stream.
        // If timeout is not infinite and no packet has been read
        // within this timeout, return zero.
        // Return size in bytes, zero on error or end of stream.
        size_t Read (void* buffer, size_t buffer_size, MilliSecond timeout = Infinite);

    private:
        friend class SinkPin;
        Mutex            _mutex;             // Protect access to all private members
        Condition        _not_empty;         // Signaled when some message is inserted
        std::deque <::IMediaSample*> _queue; // Queue of input
        size_t           _max_messages;
        ::IMediaSample*  _current_sample;    // Unfinished media sample
        size_t           _current_offset;    // Next offset in _current_sample
        ReportInterface& _report;
        ::LONG volatile  _ref_count;
        ::FILTER_STATE   _state;
        ::IFilterGraph*  _graph;
        SinkPin*         _pin;
        ::MPEG2_TRANSPORT_STRIDE _stride;    // Description of packet structure

        // Fill buffer/buffer_size with data from media sample in _current_sample/offset.
        // Update buffer and buffer_size.
        // If media sample completely copied, release it and nullify pointer.
        void FillBuffer (char*& buffer, size_t& buffer_size);
    };

    //-------------------------------------------------------------------------
    // SinkPin, input pin for our SinkFilter
    //-------------------------------------------------------------------------

    class SinkPin: public ::IPin, public ::IMemInputPin
    {
    public:
        SinkPin (ReportInterface&, SinkFilter*);
        virtual ~SinkPin();

        // Implementation of ::IUnknown
        STDMETHODIMP QueryInterface (REFIID riid, void** ppv);
        STDMETHODIMP_(::ULONG) AddRef();
        STDMETHODIMP_(::ULONG) Release();

        // Implementation of ::IPin
        STDMETHODIMP Connect (::IPin* pReceivePin, const ::AM_MEDIA_TYPE* pmt);
        STDMETHODIMP ReceiveConnection (::IPin* pConnector, const ::AM_MEDIA_TYPE* pmt);
        STDMETHODIMP Disconnect ();
        STDMETHODIMP ConnectedTo (::IPin** pPin);
        STDMETHODIMP ConnectionMediaType (::AM_MEDIA_TYPE* pmt);
        STDMETHODIMP QueryPinInfo (::PIN_INFO* pInfo);
        STDMETHODIMP QueryDirection (::PIN_DIRECTION* pPinDir);
        STDMETHODIMP QueryId (::LPWSTR* Id);
        STDMETHODIMP QueryAccept (const ::AM_MEDIA_TYPE* pmt);
        STDMETHODIMP EnumMediaTypes (::IEnumMediaTypes** ppEnum);
        STDMETHODIMP QueryInternalConnections (::IPin** apPin, ::ULONG *nPin);
        STDMETHODIMP EndOfStream ();
        STDMETHODIMP BeginFlush ();
        STDMETHODIMP EndFlush ();
        STDMETHODIMP NewSegment (::REFERENCE_TIME tStart, ::REFERENCE_TIME tStop, double dRate);

        // Implementation of ::IMemInputPin
        STDMETHODIMP GetAllocator (::IMemAllocator** ppAllocator);
        STDMETHODIMP NotifyAllocator (::IMemAllocator* pAllocator, ::BOOL bReadOnly);
        STDMETHODIMP GetAllocatorRequirements (::ALLOCATOR_PROPERTIES* pProps);
        STDMETHODIMP Receive (::IMediaSample* pSample);
        STDMETHODIMP ReceiveMultiple (::IMediaSample** pSamples, long nSamples, long* nSamplesProcessed);
        STDMETHODIMP ReceiveCanBlock ();

        // Supported media subtypes
        static const int MAX_MEDIA_SUBTYPES = 3;
        static const ::GUID MEDIA_SUBTYPES [MAX_MEDIA_SUBTYPES];

    private:
        bool             _flushing;
        bool             _input_overflow;
        ReportInterface& _report;
        ::LONG volatile  _ref_count;
        SinkFilter*      _filter;
        ::IPin*          _partner;
        ::AM_MEDIA_TYPE  _cur_media_type;
    };

    //-------------------------------------------------------------------------
    // SinkEnumMediaTypes, enumerator returned by ::IPin::EnumMediaTypes
    //-------------------------------------------------------------------------

    class SinkEnumMediaTypes : public ::IEnumMediaTypes
    {
    public:
        SinkEnumMediaTypes (ReportInterface&, const SinkEnumMediaTypes* cloned);
        virtual ~SinkEnumMediaTypes();

        // Implementation of ::IUnknown
        STDMETHODIMP QueryInterface (REFIID riid, void** ppv);
        STDMETHODIMP_(::ULONG) AddRef();
        STDMETHODIMP_(::ULONG) Release();

        // Implementation of ::IEnumMediaTypes
        STDMETHODIMP Next (::ULONG cMediaTypes, ::AM_MEDIA_TYPE** ppMediaTypes, ::ULONG* pcFetched);
        STDMETHODIMP Skip (::ULONG cMediaTypes);
        STDMETHODIMP Reset();
        STDMETHODIMP Clone (::IEnumMediaTypes** ppEnum);

    private:
        ReportInterface& _report;
        ::LONG volatile  _ref_count;
        int              _next; // Next media type to enumerate
    };

    //-------------------------------------------------------------------------
    // SinkEnumPins, enumerator returned by ::IBaseFilter::EnumPins
    //-------------------------------------------------------------------------

    class SinkEnumPins : public ::IEnumPins
    {
    public:
        SinkEnumPins (ReportInterface&, SinkFilter*, const SinkEnumPins* cloned);
        virtual ~SinkEnumPins();

        // Implementation of ::IUnknown
        STDMETHODIMP QueryInterface (REFIID riid, void** ppv);
        STDMETHODIMP_(::ULONG) AddRef();
        STDMETHODIMP_(::ULONG) Release();

        // Implementation of ::IEnumPins
        STDMETHODIMP Next (::ULONG cPins, ::IPin** ppPins, ::ULONG* pcFetched);
        STDMETHODIMP Skip (::ULONG cPins);
        STDMETHODIMP Reset ();
        STDMETHODIMP Clone (::IEnumPins** ppEnum);

    private:
        // There is only one pin to enumerate
        ReportInterface& _report;
        ::LONG volatile  _ref_count;
        SinkFilter*      _filter;
        bool             _done;
    };
}
