//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsSinkFilter.h"
#include "tsDirectShowUtils.h"
#include "tsMediaTypeUtils.h"
#include "tsWinUtils.h"
#include "tsTime.h"
#include "tsIntegerUtils.h"
#include "tsTS.h"

// Trace every low-level operation when COM instrumentation is enabled.
#if defined(TS_COM_INSTRUMENTATION)
#define TRACE(...) _report.log(__VA_ARGS__)
#define IF_TRACE(x) x
#else
#define TRACE(...)
#define IF_TRACE(x)
#endif

namespace {
    const ::WCHAR FILTER_NAME[] = L"TSDuck Sink Filter";
    const ::WCHAR PIN_NAME[]    = L"Capture";
    const ::WCHAR PIN_ID[]      = L"TSDuck Capture Pin";
}

//-----------------------------------------------------------------------------
// SinkFilter, the DirectShow filter
//-----------------------------------------------------------------------------

ts::SinkFilter::SinkFilter(Report& report) :
    _report(report),
    _pin(new SinkPin(report, this))
{
    TRACE(1, u"SinkFilter constructor, ref=%d", {_ref_count});
    // Initialize packet format to default
    _stride.dwOffset = 0;
    _stride.dwPacketLength = PKT_SIZE;
    _stride.dwStride = PKT_SIZE;
}

ts::SinkFilter::~SinkFilter()
{
    TRACE(1, u"SinkFilter destructor");
    Flush();
    _pin->Release();
}

// Implementation of ::IUnknown

STDMETHODIMP ts::SinkFilter::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == ::IID_IUnknown || riid == ::IID_IPersist || riid == ::IID_IMediaFilter || riid == ::IID_IBaseFilter) {
        TRACE(1, u"SinkFilter::QueryInterface: OK");
        AddRef();
        *ppv = static_cast<::IBaseFilter*>(this);
        return S_OK;
    }
    else {
        TRACE(1, u"SinkFilter::QueryInterface: no interface %s", {NameGUID(riid)});
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(::ULONG) ts::SinkFilter::AddRef()
{
    ::LONG c = ::InterlockedIncrement(&_ref_count);
    TRACE(2, u"SinkFilter::AddRef, ref=%d", {c});
    return c;
}

STDMETHODIMP_(::ULONG) ts::SinkFilter::Release()
{
    ::LONG c = ::InterlockedDecrement(&_ref_count);
    TRACE(2, u"SinkFilter::Release, ref=%d", {c});
    if (c == 0) {
        delete this;
    }
    return c;
}

// Implementation of ::IPersist

STDMETHODIMP ts::SinkFilter::GetClassID(::CLSID* pClsID)
{
    TRACE(1, u"SinkFilter::GetClassID");
    if (pClsID == nullptr) {
        return E_POINTER;
    }
    else {
        *pClsID = CLSID_SinkFilter;
        return S_OK;
    }
}

// Implementation of ::IMediaFilter

STDMETHODIMP ts::SinkFilter::GetState(::DWORD dwMSecs, ::FILTER_STATE* State)
{
    TRACE(1, u"SinkFilter::GetState");
    if (State == nullptr) {
        return E_POINTER;
    }
    else {
        *State = _state;
        return S_OK;
    }
}

STDMETHODIMP ts::SinkFilter::SetSyncSource(::IReferenceClock* pClock)
{
    TRACE(1, u"SinkFilter::SetSyncSource");
    // Don't care about reference clock;
    return S_OK;
}

STDMETHODIMP ts::SinkFilter::GetSyncSource(::IReferenceClock** pClock)
{
    TRACE(1, u"SinkFilter::GetSyncSource");
    if (pClock == nullptr) {
        return E_POINTER;
    }
    else {
        *pClock = nullptr;
        return S_OK;
    }
}

STDMETHODIMP ts::SinkFilter::Stop()
{
    TRACE(1, u"SinkFilter::Stop");
    _pin->EndFlush();
    _state = ::State_Stopped;
    return S_OK;
}

STDMETHODIMP ts::SinkFilter::Pause()
{
    TRACE(1, u"SinkFilter::Pause");
    _state = ::State_Paused;
    return S_OK;
}

STDMETHODIMP ts::SinkFilter::Run(::REFERENCE_TIME tStart)
{
    TRACE(1, u"SinkFilter::Run");
    _state = ::State_Running;
    return S_OK;
}

// Implementation of ::IBaseFilter

STDMETHODIMP ts::SinkFilter::EnumPins(::IEnumPins** ppEnum)
{
    TRACE(1, u"SinkFilter::EnumPins");
    if (ppEnum == nullptr) {
        return E_POINTER;
    }
    else if ((*ppEnum = new SinkEnumPins(_report, this, nullptr)) == nullptr) {
        return E_OUTOFMEMORY;
    }
    else {
        return S_OK;
    }
}

STDMETHODIMP ts::SinkFilter::FindPin(::LPCWSTR Id, ::IPin** ppPin)
{
    TRACE(1, u"SinkFilter::FindPin");
    if (ppPin == nullptr) {
        return E_POINTER;
    }
    else {
        // ignore Id, always return the single pin
        *ppPin = _pin;
        return S_OK;
    }
}

STDMETHODIMP ts::SinkFilter::QueryFilterInfo(::FILTER_INFO* pInfo)
{
    TRACE(1, u"SinkFilter::QueryFilterInfo");
    if (pInfo == nullptr) {
        return E_POINTER;
    }
    // Name should be the one specified by JoinFilterGraph
    assert(sizeof(pInfo->achName) >= sizeof(FILTER_NAME));
    std::memcpy(pInfo->achName, FILTER_NAME, sizeof(FILTER_NAME));  // Flawfinder: ignore: memcpy()
    pInfo->pGraph = _graph;
    if (_graph != nullptr) {
        _graph->AddRef();
    }
    return S_OK;
}

STDMETHODIMP ts::SinkFilter::JoinFilterGraph(::IFilterGraph* pGraph, ::LPCWSTR pName)
{
    TRACE(1, u"SinkFilter::JoinFilterGraph: %s graph", {pGraph != nullptr ? u"joining" : u"leaving"});
    _graph = pGraph;
    return S_OK;
}

STDMETHODIMP ts::SinkFilter::QueryVendorInfo(::LPWSTR* pVendorInfo)
{
    TRACE(1, u"SinkFilter::QueryVendorInfo");
    if (pVendorInfo != nullptr) {
        *pVendorInfo = nullptr;
    }
    return E_NOTIMPL;
}

// Return input pin (with one reference => use Release)

ts::SinkPin* ts::SinkFilter::GetPin()
{
    TRACE(1, u"SinkFilter::GetPin");
    _pin->AddRef();
    return _pin;
}

// Set the max number of media samples in the queue between
// the graph thread and the application thread.

void ts::SinkFilter::SetMaxMessages(size_t maxMessages)
{
    TRACE(1, u"SinkFilter::SetMaxMessages");
    _max_messages = maxMessages;
}

// Discard and release all pending media samples

void ts::SinkFilter::Flush()
{
    TRACE(1, u"SinkFilter::Flush");
    std::lock_guard<std::timed_mutex> lock(_mutex);
    _sample_buffer.clear();
    _sample_offset = 0;
    while (!_queue.empty()) {
        ::IMediaSample* ms = _queue.front();
        _queue.pop_front();
        if (ms != nullptr) {
            ms->Release();
        }
    }
}

// Read data from transport stream.
// Return size in bytes, zero on error or end of stream.

size_t ts::SinkFilter::Read(void* buffer, size_t buffer_size, cn::milliseconds timeout)
{
    TRACE(2, u"SinkFilter::Read");
    size_t remain = buffer_size;
    char* data = reinterpret_cast<char*>(buffer);
    const bool infinite = timeout.count() <= 0;
    const auto end = cn::steady_clock::now() + timeout;

    std::unique_lock<std::timed_mutex> lock(_mutex);

    // First, get data from buffered media samples.
    FillBuffer(data, remain);

    // Then, read from media queue if there is still some free space in the user's buffer.
    while (remain >= PKT_SIZE && (infinite || cn::steady_clock::now() < end)) {

        // Wait for the queue not being empty
        TRACE(5, u"SinkFilter::Read, waiting for packets, timeout = %d milliseconds", {timeout.count()});
        if (infinite) {
            _not_empty.wait(lock, [this]() { return !_queue.empty(); });
        }
        else {
            _not_empty.wait_until(lock, end, [this]() { return !_queue.empty(); });
        }
        TRACE(5, u"SinkFilter::Read, end of waiting for packets, queue size = %d", {_queue.size()});

        // If still nothing in the queue, there was an error, most likely a timeout in waiting for condition.
        if (_queue.empty()) {
            // Stop filling the buffer, returns what's in it (or zero)
            break;
        }

        // Dequeue one message
        ::IMediaSample* ms = _queue.front();
        _queue.pop_front();

        // Null pointer means end of stream
        if (ms == nullptr) {
            if (remain < buffer_size) {
                // Some data were already read for the user, this call will succeed.
                // Push eof back in queue so that eof will be reported next time.
                _queue.push_front(nullptr);
            }
            // Stop filling the buffer
            break;
        }

        // Locate data area in the media sample.
        ::LONG ms_size = ms->GetActualDataLength();
        ::BYTE* ms_buffer;
        if (!ComSuccess(ms->GetPointer(&ms_buffer), u"IMediaSample::GetPointer", _report)) {
            // Error getting media sample address
            ms_size = 0;
        }

        // Copy the media sample in the internal sample buffer.
        _sample_buffer.append(ms_buffer, ms_size);

        // We no longer need the media sample COM object, release it.
        ms->Release();

        // Copy data from media sample into user buffer
        FillBuffer(data, remain);
    }

    TRACE(2, u"SinkFilter::Read, returning %d bytes", {buffer_size - remain});
    return buffer_size - remain;
}

// Abort a blocked Read() operation.

void ts::SinkFilter::Abort()
{
    TRACE(2, u"SinkFilter::Abort");
    // Enqueue a NULL pointer. This will unblock any Read() operation and signal an end of stream.
    std::lock_guard<std::timed_mutex> lock(_mutex);
    _queue.push_back(nullptr);
    _not_empty.notify_all();
}

// Fill buffer/buffer_size with data from media samples in _sample_buffer.

void ts::SinkFilter::FillBuffer(char*& buffer, size_t& buffer_size)
{
    assert(_stride.dwPacketLength == PKT_SIZE);

    // It as been observed on Windows that some packets are truncated or corrupted
    // (not starting with 0x47). To avoid breaking the stream, we always try to resynchronize.
    // To do that, we consider that a packet is valid only when surrounded by two 0x47.
    // The first 0x47 ensures that the beginning of the packet is found.
    // The second 0x47 ensures that the packet was not truncated (next packet starts right after).
    // This means that we never read the last packet in the internal buffer. When only
    // one packet remains in the buffer, we wait for the next media sample to check the
    // next 0x47.

    size_t corrupted_chunks = 0;
    size_t corrupted_bytes = 0;

    assert(buffer != nullptr);
    assert(_sample_offset <= _sample_buffer.size());

    // Copy packet by packet, detecting and skipping corrupted chunks.
    while (buffer_size >= PKT_SIZE && _sample_offset + size_t(_stride.dwStride + _stride.dwOffset) < _sample_buffer.size()) {

        if (_sample_buffer[_sample_offset + _stride.dwOffset] == SYNC_BYTE &&
            _sample_buffer[_sample_offset + _stride.dwOffset + _stride.dwStride] == SYNC_BYTE)
        {
            // Current position contains a valid delimited packet.
            std::memcpy(buffer, _sample_buffer.data() + _sample_offset + _stride.dwOffset, PKT_SIZE);  // Flawfinder: ignore: memcpy()
            buffer += PKT_SIZE;
            buffer_size -= PKT_SIZE;
            _sample_offset += _stride.dwStride;
        }
        else {
            // Current position does not contain a valid packet.
            // Look for at least two 0x47 with a distance of 188 bytes to resynchronize.
            size_t dropped = 0;
            while (_sample_offset + size_t(_stride.dwStride + _stride.dwOffset) < _sample_buffer.size()) {
                if (_sample_buffer[_sample_offset + _stride.dwOffset] == SYNC_BYTE &&
                    _sample_buffer[_sample_offset + _stride.dwOffset + _stride.dwStride] == SYNC_BYTE)
                {
                    // Found a valid delimited packet.
                    break;
                }
                else {
                    _sample_offset++;
                    dropped++;
                }
            }

            TRACE(5, u"SinkFilter::FillBuffer, dropped %d corrupted bytes", {dropped});
            corrupted_bytes += dropped;
            corrupted_chunks++;
        }
    }

    // Remove returned packets or skipped corrupted data from the internal sample buffer.
    if (_sample_offset > 0) {
        _sample_buffer.erase(0, _sample_offset);
        _sample_offset = 0;
    }

    // Report corrupted packet count
    if (corrupted_chunks > 0) {
        _report.verbose(u"tuner packet synchronization lost, dropped %d bytes in %d chunks", {corrupted_bytes, corrupted_chunks});
    }
}


//-----------------------------------------------------------------------------
// SinkPin, input pin for our SinkFilter
//-----------------------------------------------------------------------------

ts::SinkPin::SinkPin(Report& report, SinkFilter* filter) :
    _flushing(false),
    _input_overflow(false),
    _report(report),
    _ref_count(1),
    _filter(filter),
    _partner(nullptr)
{
    TRACE(1, u"SinkPin constructor, ref=%d", {_ref_count});
    InitMediaType(_cur_media_type);
}

ts::SinkPin::~SinkPin()
{
    TRACE(1, u"SinkPin destructor");
    FreeMediaType(_cur_media_type);
}

// Implementation of ::IUnknown

STDMETHODIMP ts::SinkPin::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == ::IID_IUnknown || riid == ::IID_IPin) {
        TRACE(1, u"SinkPin::QueryInterface: IPin, OK");
        AddRef();
        *ppv = static_cast<::IPin*>(this);
        return S_OK;
    }
    else if (riid == ::IID_IMemInputPin) {
        TRACE(1, u"SinkPin::QueryInterface: IMemInputPin, OK");
        AddRef();
        *ppv = static_cast<::IMemInputPin*>(this);
        return S_OK;
    }
    else {
        TRACE(1, u"SinkPin::QueryInterface: no interface %s", {NameGUID(riid)});
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(::ULONG) ts::SinkPin::AddRef()
{
    ::LONG c = ::InterlockedIncrement(&_ref_count);
    TRACE(2, u"SinkPin::AddRef, ref=%d", {c});
    return c;
}

STDMETHODIMP_(::ULONG) ts::SinkPin::Release()
{
    ::LONG c = ::InterlockedDecrement(&_ref_count);
    TRACE(2, u"SinkPin::Release, ref=%d", {c});
    if (c == 0) {
        delete this;
    }
    return c;
}

// Implementation of ::IPin

STDMETHODIMP ts::SinkPin::Connect(::IPin* pReceivePin, const ::AM_MEDIA_TYPE* pmt)
{
    TRACE(1, u"SinkPin::Connect: checking");
    if (_filter->_state != ::State_Stopped) {
        return VFW_E_NOT_STOPPED; // dynamic reconnection not supported
    }
    if (_partner != nullptr) {
        return VFW_E_ALREADY_CONNECTED;
    }
    if (pmt != nullptr && QueryAccept(pmt) != S_OK) {
        return VFW_E_TYPE_NOT_ACCEPTED; // unsupported media type
    }
    TRACE(1, u"SinkPin::Connect: OK");
    return S_OK;
}

STDMETHODIMP ts::SinkPin::ReceiveConnection(::IPin* pConnector, const ::AM_MEDIA_TYPE* pmt)
{
    TRACE(1, u"SinkPin::ReceiveConnection: checking");
    if (_filter->_state == ::State_Running) {
        return VFW_E_NOT_STOPPED; // dynamic reconnection not supported
    }
    if (_partner != nullptr) {
        return VFW_E_ALREADY_CONNECTED;
    }
    if (pConnector == nullptr || pmt == nullptr) {
        return E_POINTER;
    }
    if (QueryAccept(pmt) != S_OK) {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    TRACE(1, u"SinkPin::ReceiveConnection: connected");
    _flushing = false;
    _input_overflow = false;

    // Get transport packet format
    if (pmt->subtype == ::MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE && pmt->formattype == ::FORMAT_None && pmt->pbFormat != nullptr) {
        assert(pmt->cbFormat >= sizeof(::MPEG2_TRANSPORT_STRIDE)); // already checked in QueryAccept
        // This is a transport stride with specific data
        std::memcpy(&_filter->_stride, pmt->pbFormat, sizeof(_filter->_stride));  // Flawfinder: ignore: memcpy()
        _report.debug(u"new connection transport stride: offset = %d, packet length = %d, stride = %d",
                      {_filter->_stride.dwOffset,
                       _filter->_stride.dwPacketLength = PKT_SIZE,
                       _filter->_stride.dwStride = PKT_SIZE});
        // Check consistency
        if (_filter->_stride.dwPacketLength != PKT_SIZE ||
            _filter->_stride.dwOffset + _filter->_stride.dwPacketLength > _filter->_stride.dwStride) {
            // Invalid stride values for TS
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
    else {
        // Default stride: contiguous packets.
        _filter->_stride.dwOffset = 0;
        _filter->_stride.dwPacketLength = PKT_SIZE;
        _filter->_stride.dwStride = PKT_SIZE;
    }
    // Keep a reference on partner pin
    _partner = pConnector;
    _partner->AddRef();
    // Copy media type into pin
    FreeMediaType(_cur_media_type);
    return CopyMediaType(_cur_media_type, *pmt);
}

STDMETHODIMP ts::SinkPin::Disconnect()
{
    TRACE(1, u"SinkPin::Disconnect: checking");
    if (_partner == nullptr) {
        return S_FALSE; // not connected
    }
    if (_filter->_state != ::State_Stopped) {
        return VFW_E_NOT_STOPPED;
    }
    TRACE(1, u"SinkPin::Disconnect: disconnected");
    _partner->Release();
    _partner = nullptr;
    return S_OK;
}

STDMETHODIMP ts::SinkPin::ConnectedTo(::IPin** pPin)
{
    TRACE(1, u"SinkPin::ConnectedTo");
    if (pPin == nullptr) {
        return E_POINTER;
    }
    else if (_partner == nullptr) {
        *pPin = nullptr;
        return VFW_E_NOT_CONNECTED;
    }
    else {
        _partner->AddRef();
        *pPin = _partner;
        return S_OK;
    }
}

STDMETHODIMP ts::SinkPin::ConnectionMediaType(::AM_MEDIA_TYPE* pmt)
{
    TRACE(1, u"SinkPin::ConnectionMediaType");
    if (pmt == nullptr) {
        return E_POINTER;
    }
    else if (_partner == nullptr) {
        return VFW_E_NOT_CONNECTED;
    }
    else {
        return CopyMediaType(*pmt, _cur_media_type);
    }
}

STDMETHODIMP ts::SinkPin::QueryPinInfo(::PIN_INFO* pInfo)
{
    TRACE(1, u"SinkPin::QueryPinInfo");
    if (pInfo == nullptr) {
        return E_POINTER;
    }
    pInfo->dir = ::PINDIR_INPUT;
    pInfo->pFilter = _filter;
    if (_filter != nullptr) {
        _filter->AddRef();
    }
    assert(sizeof(pInfo->achName) >= sizeof(PIN_NAME));
    std::memcpy(pInfo->achName, PIN_NAME, sizeof(PIN_NAME));  // Flawfinder: ignore: memcpy()
    return S_OK;
}

STDMETHODIMP ts::SinkPin::QueryDirection(::PIN_DIRECTION* pPinDir)
{
    TRACE(1, u"SinkPin::QueryDirection");
    if (pPinDir == nullptr) {
        return E_POINTER;
    }
    else {
        *pPinDir = ::PINDIR_INPUT;
        return S_OK;
    }
}

STDMETHODIMP ts::SinkPin::QueryId(::LPWSTR* Id)
{
    TRACE(1, u"SinkPin::QueryId");
    if (Id == nullptr) {
        return E_POINTER;
    }
    else if ((*Id = reinterpret_cast<::WCHAR*>(::CoTaskMemAlloc(sizeof(PIN_ID)))) == nullptr) {
        return E_OUTOFMEMORY;
    }
    else {
        std::memcpy(*Id, PIN_ID, sizeof(PIN_ID));  // Flawfinder: ignore: memcpy()
        return S_OK;
    }
}

STDMETHODIMP ts::SinkPin::QueryAccept(const ::AM_MEDIA_TYPE* pmt)
{
    TRACE(1, u"SinkPin::QueryAccept, type %s, subtype %s, format type %s, format size %d, content: %s",
          {NameGUID(pmt->majortype),
           NameGUID(pmt->subtype),
           NameGUID(pmt->formattype),
           pmt->cbFormat,
           pmt->pbFormat == nullptr ? u"none" : UString::Dump(pmt->pbFormat, pmt->cbFormat, UString::SINGLE_LINE)});

    if (pmt->majortype != ::MEDIATYPE_Stream) {
        return S_FALSE; // unsupported major type
    }
    bool found = false;
    for (int i = 0; !found && i < MAX_MEDIA_SUBTYPES; i++) {
        // MSVC++ bug: consider GUID::operator== as returning int
        found = (pmt->subtype == MEDIA_SUBTYPES[i]) != 0;
    }
    if (!found) {
        return S_FALSE; // unsupported subtype
    }
    if (pmt->subtype == ::MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE && pmt->formattype == ::FORMAT_None && pmt->pbFormat != nullptr) {
        if (pmt->cbFormat < sizeof(::MPEG2_TRANSPORT_STRIDE)) {
            return S_FALSE; // format structure too short
        }
        const ::MPEG2_TRANSPORT_STRIDE* ts = reinterpret_cast<const ::MPEG2_TRANSPORT_STRIDE*>(pmt->pbFormat);
        if (ts->dwPacketLength != ts::PKT_SIZE) {
            return S_FALSE; // invalid packet size
        }
    }
    return S_OK;
}

STDMETHODIMP ts::SinkPin::EnumMediaTypes(::IEnumMediaTypes** ppEnum)
{
    TRACE(1, u"SinkPin::EnumMediaTypes");
    if (ppEnum == nullptr) {
        return E_POINTER;
    }
    else if ((*ppEnum = new SinkEnumMediaTypes(_report, nullptr)) == nullptr) {
        return E_OUTOFMEMORY;
    }
    else {
        return S_OK;
    }
}

STDMETHODIMP ts::SinkPin::QueryInternalConnections(::IPin** apPin, ::ULONG *nPin)
{
    TRACE(1, u"SinkPin::QueryInternalConnections");
    return E_NOTIMPL; // not implemented
}

STDMETHODIMP ts::SinkPin::EndOfStream()
{
    TRACE(1, u"SinkPin::EndOfStream");
    // Enqueue a NULL pointer instead of an ::IMediaSample*
    std::lock_guard<std::timed_mutex> lock(_filter->_mutex);
    _filter->_queue.push_back(nullptr);
    _filter->_not_empty.notify_all();
    return S_OK;
}

STDMETHODIMP ts::SinkPin::BeginFlush()
{
    TRACE(1, u"SinkPin::BeginFlush");
    _flushing = true;
    _filter->Flush();
    return S_OK;
}

STDMETHODIMP ts::SinkPin::EndFlush()
{
    TRACE(1, u"SinkPin::EndFlush");
    _flushing = false;
    _input_overflow = false;
    _filter->Flush();
    return S_OK;
}

STDMETHODIMP ts::SinkPin::NewSegment(::REFERENCE_TIME tStart, ::REFERENCE_TIME tStop, double dRate)
{
    TRACE(1, u"SinkPin::NewSegment");
    // We don't care about time info
    return S_OK;
}

// Implementation of ::IMemInputPin

STDMETHODIMP ts::SinkPin::GetAllocator(::IMemAllocator** ppAllocator)
{
    TRACE(1, u"SinkPin::GetAllocator");
    return VFW_E_NO_ALLOCATOR;  // we don't provide allocators
}

STDMETHODIMP ts::SinkPin::NotifyAllocator(::IMemAllocator* pAllocator, ::BOOL bReadOnly)
{
    TRACE(1, u"SinkPin::NotifyAllocator");
    return S_OK; // we don't care
}

STDMETHODIMP ts::SinkPin::GetAllocatorRequirements(::ALLOCATOR_PROPERTIES* pProps)
{
    TRACE(1, u"SinkPin::GetAllocatorRequirements");
    return E_NOTIMPL; // we don't have any requirement
}

STDMETHODIMP ts::SinkPin::ReceiveCanBlock()
{
    TRACE(1, u"SinkPin::ReceiveCanBlock");
    return S_FALSE; // we never block
}

STDMETHODIMP ts::SinkPin::Receive(::IMediaSample* pSample)
{
    TRACE(2, u"SinkPin::Receive");
    long processed = 0;
    return ReceiveMultiple(&pSample, 1, &processed);
}

STDMETHODIMP ts::SinkPin::ReceiveMultiple(::IMediaSample** pSamples, long nSamples, long* nSamplesProcessed)
{
    TRACE(2, u"SinkPin::ReceiveMultiple: samples count: %d", {nSamples});

    *nSamplesProcessed = 0;
    bool report_overflow = false;

    if (_flushing) {
        // Reject samples during a flush
        return S_FALSE;
    }
    else if (nSamples <= 0) {
        // Avoid locking if no data is specified.
        return S_OK;
    }

    // Enqueue all media samples using one single lock section.
    {
        // Try to get the mutex within 1 second.
        std::unique_lock<std::timed_mutex> lock(_filter->_mutex, cn::seconds(1));
        if (!lock.owns_lock()) {
            _report.error(u"cannot enqueue media sample, lock timeout");
            return S_FALSE;
        }

        // Loop on all media samples.
        while (*nSamplesProcessed < nSamples) {
            ::IMediaSample* pSample = pSamples[*nSamplesProcessed];
            IF_TRACE(const long length = pSample->GetActualDataLength();)
            TRACE(2, u"SinkPin::ReceiveMultiple: actual data length: %d bytes, %d packets + %d bytes", {length, length / PKT_SIZE, length % PKT_SIZE});
            if (_filter->_max_messages != 0 && _filter->_queue.size() >= _filter->_max_messages) {
                // Cannot enqueue. Don't report consecutive overflow
                report_overflow = report_overflow || !_input_overflow;
                _input_overflow = true;
                break;
            }
            else {
                pSample->AddRef();
                _filter->_queue.push_back(pSample);
                _input_overflow = false;
            }
            (*nSamplesProcessed)++;
        }

        // Notify client application when all samples are enqueued.
        _filter->_not_empty.notify_all();
    }

    // Does reporting outside locked section.
    if (report_overflow) {
        _report.verbose(u"transport stream input overflow");
    }
    return S_OK;
}

// Supported media subtypes

const ::GUID ts::SinkPin::MEDIA_SUBTYPES [MAX_MEDIA_SUBTYPES] = {
    MEDIASUBTYPE_MPEG2_TRANSPORT,
    MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE,
    KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT,
};


//-----------------------------------------------------------------------------
// SinkEnumMediaTypes, enumerator returned by ::IPin::EnumMediaTypes
//-----------------------------------------------------------------------------

ts::SinkEnumMediaTypes::SinkEnumMediaTypes(Report& report, const SinkEnumMediaTypes* cloned) :
    _report(report),
    _ref_count(1),
    _next(cloned == nullptr ? 0 : cloned->_next)
{
    TRACE(2, u"SinkEnumMediaTypes constructor, ref=%d", {_ref_count});
}

ts::SinkEnumMediaTypes::~SinkEnumMediaTypes()
{
    TRACE(2, u"SinkEnumMediaTypes destructor");
}

// Implementation of ::IUnknown

STDMETHODIMP ts::SinkEnumMediaTypes::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == ::IID_IUnknown || riid == ::IID_IEnumMediaTypes) {
        TRACE(1, u"SinkEnumMediaTypes::QueryInterface: OK");
        AddRef();
        *ppv = static_cast<::IEnumMediaTypes*>(this);
        return S_OK;
    }
    else {
        TRACE(1, u"SinkEnumMediaTypes::QueryInterface: no interface");
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(::ULONG) ts::SinkEnumMediaTypes::AddRef()
{
    ::LONG c = ::InterlockedIncrement(&_ref_count);
    TRACE(2, u"SinkEnumMediaTypes::AddRef, ref=%d", {c});
    return c;
}

STDMETHODIMP_(::ULONG) ts::SinkEnumMediaTypes::Release()
{
    ::LONG c = ::InterlockedDecrement(&_ref_count);
    TRACE(2, u"SinkEnumMediaTypes::Release, ref=%d", {c});
    if (c == 0) {
        delete this;
    }
    return c;
}

// Implementation of ::IEnumMediaTypes

STDMETHODIMP ts::SinkEnumMediaTypes::Next(::ULONG cMediaTypes, ::AM_MEDIA_TYPE** ppMediaTypes, ::ULONG* pcFetched)
{
    TRACE(1, u"SinkEnumMediaTypes::Next");
    if (ppMediaTypes == nullptr || (pcFetched == nullptr && cMediaTypes > 1)) {
        return E_POINTER;
    }
    ::ULONG copied;
    for (copied = 0; copied < cMediaTypes && _next < SinkPin::MAX_MEDIA_SUBTYPES; copied++) {
        ppMediaTypes[copied] = reinterpret_cast<::AM_MEDIA_TYPE*>(::CoTaskMemAlloc(sizeof(::AM_MEDIA_TYPE)));
        if (ppMediaTypes[copied] == nullptr) {
            return E_OUTOFMEMORY;
        }
        InitMediaType(*ppMediaTypes[copied]);
        ppMediaTypes[copied]->majortype = ::MEDIATYPE_Stream;
        ppMediaTypes[copied]->subtype = SinkPin::MEDIA_SUBTYPES [_next];
        ppMediaTypes[copied]->formattype = ::FORMAT_None;
        _next++;
    }
    if (pcFetched != nullptr) {
        *pcFetched = copied;
    }
    return copied == cMediaTypes ? S_OK : S_FALSE; // both are success
}

STDMETHODIMP ts::SinkEnumMediaTypes::Skip(::ULONG cMediaTypes)
{
    TRACE(1, u"SinkEnumMediaTypes::Skip(%d)", {cMediaTypes});
    _next = std::max(0, std::min(SinkPin::MAX_MEDIA_SUBTYPES, _next + int(cMediaTypes)));
    return _next < SinkPin::MAX_MEDIA_SUBTYPES ? S_OK : S_FALSE;
}

STDMETHODIMP ts::SinkEnumMediaTypes::Reset()
{
    TRACE(1, u"SinkEnumMediaTypes::Reset");
    _next = 0;
    return S_OK;
}

STDMETHODIMP ts::SinkEnumMediaTypes::Clone(::IEnumMediaTypes** ppEnum)
{
    TRACE(1, u"SinkEnumMediaTypes::Clone");
    if (ppEnum == nullptr) {
        return E_POINTER;
    }
    else if ((*ppEnum = new SinkEnumMediaTypes(_report, this)) == nullptr) {
        return E_OUTOFMEMORY;
    }
    else {
        return S_OK;
    }
}


//-----------------------------------------------------------------------------
// SinkEnumPins, enumerator returned by ::IBaseFilter::EnumPins
//-----------------------------------------------------------------------------

ts::SinkEnumPins::SinkEnumPins(Report& report, SinkFilter* filter, const SinkEnumPins* cloned) :
    _report(report),
    _ref_count(1),
    _filter(filter),
    _done(cloned == nullptr ? false : cloned->_done)
{
    TRACE(2, u"SinkEnumPins constructor, ref=%d, done=%s", {_ref_count, UString::TrueFalse(_done)});
    _filter->AddRef();
}

ts::SinkEnumPins::~SinkEnumPins()
{
    TRACE(2, u"SinkEnumPins destructor");
    _filter->Release();
}

// Implementation of ::IUnknown

STDMETHODIMP ts::SinkEnumPins::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == ::IID_IUnknown || riid == ::IID_IEnumPins) {
        TRACE(1, u"SinkEnumPins::QueryInterface: OK");
        AddRef();
        *ppv = static_cast<::IEnumPins*>(this);
        return S_OK;
    }
    else {
        TRACE(1, u"SinkEnumPins::QueryInterface: no interface");
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(::ULONG) ts::SinkEnumPins::AddRef()
{
    ::LONG c = ::InterlockedIncrement(&_ref_count);
    TRACE(2, u"SinkEnumPins::AddRef, ref=%d", {c});
    return c;
}

STDMETHODIMP_(::ULONG) ts::SinkEnumPins::Release()
{
    ::LONG c = ::InterlockedDecrement(&_ref_count);
    TRACE(2, u"SinkEnumPins::Release, ref=%d", {c});
    if (c == 0) {
        delete this;
    }
    return c;
}

// Implementation of ::IEnumPins

STDMETHODIMP ts::SinkEnumPins::Next(::ULONG cPins, ::IPin** ppPins, ::ULONG* pcFetched)
{
    TRACE(1, u"SinkEnumPins::Next");
    if (ppPins == nullptr || (pcFetched == nullptr && cPins > 1)) {
        return E_POINTER;
    }
    ::ULONG copied = 0;
    if (cPins > 0 && !_done) {
        ppPins[0] = _filter->GetPin(); // AddRef() performed on pin
        _done = true;
        copied = 1;
    }
    if (pcFetched != nullptr) {
        *pcFetched = copied;
    }
    return copied == cPins ? S_OK : S_FALSE; // both are success
}

STDMETHODIMP ts::SinkEnumPins::Skip(::ULONG cPins)
{
    TRACE(1, u"SinkEnumPins::Skip(%d)", {cPins});
    if (cPins > 0) {
        _done = true;
    }
    return _done ? S_FALSE : S_OK;
}

STDMETHODIMP ts::SinkEnumPins::Reset()
{
    TRACE(1, u"SinkEnumPins::Reset");
    _done = false;
    return S_OK;
}

STDMETHODIMP ts::SinkEnumPins::Clone(::IEnumPins** ppEnum)
{
    TRACE(1, u"SinkEnumPins::Clone");
    if (ppEnum == nullptr) {
        return E_POINTER;
    }
    else if ((*ppEnum = new SinkEnumPins(_report, _filter, this)) == nullptr) {
        return E_OUTOFMEMORY;
    }
    else {
        return S_OK;
    }
}
