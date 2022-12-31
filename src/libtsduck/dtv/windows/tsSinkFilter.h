//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!
//!  @file
//!  DirectShow filter for DVB tuners capture (Windows-specific).
//!
//!  With many ideas taken from VLC and Microsoft Windows SDK samples.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsMutex.h"
#include "tsCondition.h"
#include "tsReport.h"
#include "tsDirectShow.h"

namespace ts {

    class SinkFilter;
    class SinkPin;
    class SinkEnumMediaTypes;
    class SinkEnumPins;

    //!
    //! The DirectShow sink filter (Windows-specific).
    //! @ingroup windows
    //!
    //! This class implements a DirectShow filter.
    //!
    //! DirectShow is a very complicated infrastructure on Windows to support
    //! various media processing. BDA (Broadcast Device Architecture) is the
    //! generic device driver interface which links "broadcast devices" like
    //! DVB receivers to DirectShow. DirectShow is consequently the only generic
    //! way to interact with any type of DVB receiver hardware, provided that
    //! the hardware vendor supplies BDA-compatible drivers for the device.
    //!
    //! The "sink filter" is intended to be used after a DirectShow
    //! capture filter, as provided by the hardware vendor. We call it a "sink"
    //! filter because it has one input pin (for MPEG-2 TS) but no output pin.
    //! The TS "samples" are read asynchronously by the application. This filter
    //! acts as an adapter between the push model of DirectShow and the pull model
    //! of tsp, the transport stream processor.
    //!
    //! This module contains several classes:
    //!
    //! - SinkFilter         : The DirectShow filter
    //! - SinkPin            : Input pin for SinkFilter
    //! - SinkEnumMediaTypes : Enumerator returned by IPin::EnumMediaTypes
    //! - SinkEnumPins       : Enumerator returned by IBaseFilter::EnumPins
    //!
    //! The SinkPin accepts only MPEG-2 transport streams:
    //!
    //! - Major type : MEDIATYPE_Stream
    //! - Subtype    : MEDIASUBTYPE_MPEG2_TRANSPORT
    //!                MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE
    //!                KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT
    //!
    class SinkFilter : public ::IBaseFilter
    {
        TS_NOBUILD_NOCOPY(SinkFilter);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //!
        SinkFilter(Report& report);

        //!
        //! Destructor.
        //!
        virtual ~SinkFilter();

        //!
        //! Get the unique input pin.
        //! @return The unique input pin of the filter.
        //! The returned object has one reference for the caller.
        //! Use Release() when no longer needed.
        //!
        SinkPin* GetPin();

        //!
        //! Set the max number of media samples in the queue between the graph thread and the application thread.
        //! Must be called when the graph is stopped or paused.
        //! @param [in] maxMessages Max number of media samples in the queue between
        //! the graph thread and the application thread.
        //!
        void SetMaxMessages(size_t maxMessages);

        //!
        //! Discard and release all pending media samples.
        //!
        void Flush();

        //!
        //! Read data from transport stream.
        //! @param [out] buffer Address of returned TS packet buffer.
        //! @param [in] buffer_size Size in bytes of the @a buffer.
        //! @param [in] timeout Read timeout in milliseconds.
        //! If timeout is not infinite and no packet has been read
        //! within this timeout, return zero.
        //! @return The size in bytes of the data returned in buffer.
        //! Always return a multiple of 188, complete TS packets.
        //! Return zero on error or end of stream.
        //!
        size_t Read(void* buffer, size_t buffer_size, MilliSecond timeout = Infinite);

        //!
        //! Abort a blocked Read() operation.
        //! Can be called from any thread.
        //!
        void Abort();

        // Implementations of COM interfaces. Not documented in Doxygen.
        //! @cond nodoxygen

        // Implementation of ::IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
        STDMETHODIMP_(::ULONG) AddRef();
        STDMETHODIMP_(::ULONG) Release();

        // Implementation of ::IPersist
        STDMETHODIMP GetClassID(::CLSID* pClsID);

        // Implementation of ::IMediaFilter
        STDMETHODIMP GetState(::DWORD dwMSecs, ::FILTER_STATE* State);
        STDMETHODIMP SetSyncSource(::IReferenceClock* pClock);
        STDMETHODIMP GetSyncSource(::IReferenceClock** pClock);
        STDMETHODIMP Stop();
        STDMETHODIMP Pause();
        STDMETHODIMP Run(::REFERENCE_TIME tStart);

        // Implementation of ::IBaseFilter
        STDMETHODIMP EnumPins(::IEnumPins** ppEnum);
        STDMETHODIMP FindPin(::LPCWSTR Id, ::IPin** ppPin);
        STDMETHODIMP QueryFilterInfo(::FILTER_INFO* pInfo);
        STDMETHODIMP JoinFilterGraph(::IFilterGraph* pGraph, ::LPCWSTR pName);
        STDMETHODIMP QueryVendorInfo(::LPWSTR* pVendorInfo);

        //! @endcond

    private:
        friend class SinkPin;
        Mutex            _mutex;             // Protect access to all private members
        Condition        _not_empty;         // Signaled when some message is inserted
        std::deque <::IMediaSample*> _queue; // Queue of input
        size_t           _max_messages;
        ByteBlock        _sample_buffer;     // Collected media samples
        size_t           _sample_offset;     // Next offset in _sample_buffer
        Report&          _report;
        ::LONG volatile  _ref_count;
        ::FILTER_STATE   _state;
        ::IFilterGraph*  _graph;
        SinkPin*         _pin;
        ::MPEG2_TRANSPORT_STRIDE _stride;    // Description of packet structure

        //!
        //! Fill the user's buffer with data from media samples in _sample_buffer.
        //! @param [in,out] buffer Address of user's buffer. Updated after last read packet.
        //! @param [in,out] size Size in bytes of user's buffer. Updated after last read packet.
        //!
        void FillBuffer(char*& buffer, size_t& buffer_size);
    };

    //!
    //! SinkPin, input pin for SinkFilter (Windows-specific).
    //! @ingroup windows
    //!
    class SinkPin: public ::IPin, public ::IMemInputPin
    {
        TS_NOBUILD_NOCOPY(SinkPin);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in,out] filter The associated SinkFilter.
        //!
        SinkPin(Report& report, SinkFilter* filter);

        //!
        //! Destructor.
        //!
        virtual ~SinkPin();

        // Implementations of COM interfaces. Not documented in Doxygen.
        //! @cond nodoxygen

        // Implementation of ::IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
        STDMETHODIMP_(::ULONG) AddRef();
        STDMETHODIMP_(::ULONG) Release();

        // Implementation of ::IPin
        STDMETHODIMP Connect(::IPin* pReceivePin, const ::AM_MEDIA_TYPE* pmt);
        STDMETHODIMP ReceiveConnection(::IPin* pConnector, const ::AM_MEDIA_TYPE* pmt);
        STDMETHODIMP Disconnect();
        STDMETHODIMP ConnectedTo(::IPin** pPin);
        STDMETHODIMP ConnectionMediaType(::AM_MEDIA_TYPE* pmt);
        STDMETHODIMP QueryPinInfo(::PIN_INFO* pInfo);
        STDMETHODIMP QueryDirection(::PIN_DIRECTION* pPinDir);
        STDMETHODIMP QueryId(::LPWSTR* Id);
        STDMETHODIMP QueryAccept(const ::AM_MEDIA_TYPE* pmt);
        STDMETHODIMP EnumMediaTypes(::IEnumMediaTypes** ppEnum);
        STDMETHODIMP QueryInternalConnections(::IPin** apPin, ::ULONG *nPin);
        STDMETHODIMP EndOfStream();
        STDMETHODIMP BeginFlush();
        STDMETHODIMP EndFlush();
        STDMETHODIMP NewSegment(::REFERENCE_TIME tStart, ::REFERENCE_TIME tStop, double dRate);

        // Implementation of ::IMemInputPin
        STDMETHODIMP GetAllocator(::IMemAllocator** ppAllocator);
        STDMETHODIMP NotifyAllocator(::IMemAllocator* pAllocator, ::BOOL bReadOnly);
        STDMETHODIMP GetAllocatorRequirements(::ALLOCATOR_PROPERTIES* pProps);
        STDMETHODIMP Receive(::IMediaSample* pSample);
        STDMETHODIMP ReceiveMultiple(::IMediaSample** pSamples, long nSamples, long* nSamplesProcessed);
        STDMETHODIMP ReceiveCanBlock();

        // Supported media subtypes
        static const int MAX_MEDIA_SUBTYPES = 3;
        static const ::GUID MEDIA_SUBTYPES[MAX_MEDIA_SUBTYPES];

        //! @endcond

    private:
        bool             _flushing;
        bool             _input_overflow;
        Report&          _report;
        ::LONG volatile  _ref_count;
        SinkFilter*      _filter;
        ::IPin*          _partner;
        ::AM_MEDIA_TYPE  _cur_media_type;
    };

    //!
    //! SinkEnumMediaTypes, enumerator returned by \::IPin\::EnumMediaTypes (Windows-specific).
    //! @ingroup windows
    //!
    class SinkEnumMediaTypes : public ::IEnumMediaTypes
    {
        TS_NOBUILD_NOCOPY(SinkEnumMediaTypes);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] cloned Optional SinkEnumMediaTypes that we are cloning. Can be null.
        //!
        SinkEnumMediaTypes(Report& report, const SinkEnumMediaTypes* cloned);

        //!
        //! Destructor.
        //!
        virtual ~SinkEnumMediaTypes();

        // Implementations of COM interfaces. Not documented in Doxygen.
        //! @cond nodoxygen

        // Implementation of ::IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
        STDMETHODIMP_(::ULONG) AddRef();
        STDMETHODIMP_(::ULONG) Release();

        // Implementation of ::IEnumMediaTypes
        STDMETHODIMP Next(::ULONG cMediaTypes, ::AM_MEDIA_TYPE** ppMediaTypes, ::ULONG* pcFetched);
        STDMETHODIMP Skip(::ULONG cMediaTypes);
        STDMETHODIMP Reset();
        STDMETHODIMP Clone(::IEnumMediaTypes** ppEnum);

        //! @endcond

    private:
        Report&          _report;
        ::LONG volatile  _ref_count;
        int              _next; // Next media type to enumerate
    };

    //!
    //! SinkEnumPins, enumerator returned by \::IBaseFilter\::EnumPins (Windows-specific).
    //! @ingroup windows
    //!
    class SinkEnumPins : public ::IEnumPins
    {
        TS_NOBUILD_NOCOPY(SinkEnumPins);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in,out] filter The associated SinkFilter.
        //! @param [in] cloned Optional SinkEnumPins that we are cloning. Can be null.
        //!
        SinkEnumPins(Report& report, SinkFilter* filter, const SinkEnumPins* cloned);

        //!
        //! Destructor.
        //!
        virtual ~SinkEnumPins();

        // Implementations of COM interfaces. Not documented in Doxygen.
        //! @cond nodoxygen

        // Implementation of ::IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
        STDMETHODIMP_(::ULONG) AddRef();
        STDMETHODIMP_(::ULONG) Release();

        // Implementation of ::IEnumPins
        STDMETHODIMP Next(::ULONG cPins, ::IPin** ppPins, ::ULONG* pcFetched);
        STDMETHODIMP Skip(::ULONG cPins);
        STDMETHODIMP Reset();
        STDMETHODIMP Clone(::IEnumPins** ppEnum);

        //! @endcond

    private:
        // There is only one pin to enumerate
        Report&          _report;
        ::LONG volatile  _ref_count;
        SinkFilter*      _filter;
        bool             _done;
    };
}
