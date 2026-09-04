//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Perform a simple Web request. Common parts. See specific parts in
//  unix/tsWebRequestGuts.cpp and windows/tsWebRequestGuts.cpp.
//
//----------------------------------------------------------------------------

#include "tsWebRequest.h"
#include "tsReactiveWebRequest.h"


//----------------------------------------------------------------------------
// Guts implementation of WebRequest.
// We use the reactive implementation as portable back-end.
//----------------------------------------------------------------------------

class ts::WebRequest::Guts: public ReactiveWebHandlerInterface
{
    TS_NOBUILD_NOCOPY(Guts);
public:
    // Constructor with a reference to parent WebRequest.
    Guts(WebRequest& parent) : request(parent) {}
    virtual ~Guts() override;

    WebRequest&        request;
    Reactor            reactor {&request};
    ReactiveWebRequest reactive {reactor};
    bool               open_called = false;
    int                open_status = SYS_SUCCESS;
    int                in_status = SYS_SUCCESS;
    ByteBlockPtr       in_data {};
    size_t             in_start = 0;
    std::ofstream      out_file {};

    // Implementation of ReactiveWebHandlerInterface.
    virtual void handleWebOpen(ReactiveWebRequest&, int, const ObjectPtr&) override;
    virtual void handleWebReceive(ReactiveWebRequest&, const ByteBlockPtr&, int, const ObjectPtr&) override;
};


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::WebRequest::WebRequest(Report* report) :
    ReporterBase(report),
    _guts(new Guts(*this))
{
}

ts::WebRequest::WebRequest(ReporterBase* delegate) :
    ReporterBase(delegate),
    _guts(new Guts(*this))
{
}

ts::WebRequest::~WebRequest()
{
    if (_guts != nullptr) {
        delete _guts;
        _guts = nullptr;
    }
}

ts::WebRequest::Guts::~Guts()
{
}


//----------------------------------------------------------------------------
// Directly delegated methods.
//----------------------------------------------------------------------------

ts::WebRequestArgs& ts::WebRequest::args()
{
    return _guts->reactive.args();
}

const ts::WebRequestArgs& ts::WebRequest::args() const
{
    return _guts->reactive.args();
}

const ts::WebRequestStatus& ts::WebRequest::status() const
{
    return _guts->reactive.status();
}

bool ts::WebRequest::isOpen() const
{
    return _guts->reactive.isOpen();
}


//----------------------------------------------------------------------------
// Open an URL and start the transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::open(const UString& url, size_t buffer_size)
{
    if (isOpen() || _guts->reactor.isOpen()) {
        report().error(u"internal error, transfer already started, cannot download %s", url);
        return false;
    }

    // Initialize the internal guts. Don't reset out_file.
    _guts->open_called = false;
    _guts->open_status = _guts->in_status = SYS_SUCCESS;
    _guts->in_data.reset();
    _guts->in_start = 0;

    if (!_guts->reactor.open()) {
        return false;
    }

    // Start the transfer in the reactive environment.
    if (!_guts->reactive.start(_guts, url, buffer_size)) {
        _guts->reactor.close();
        return false;
    }

    // Loop on events until the open callback is invoked.
    while (!_guts->open_called) {
        _guts->reactor.processEventLoop();
    }

    // Report the open status.
    const bool status = SysSuccess(_guts->open_status);
    if (!status) {
        abort();
    }
    SetLastSysErrorCode(_guts->open_status);
    return status;
}


//----------------------------------------------------------------------------
// Invoked when the URL is open.
//----------------------------------------------------------------------------

void ts::WebRequest::Guts::handleWebOpen(ReactiveWebRequest& req, int error_code, const ObjectPtr& user_data)
{
    open_called = true;
    open_status = error_code;

    // Exit event loop at end of open, so that WebRequest::open() can get control back.
    reactor.exitEventLoop();
}


//----------------------------------------------------------------------------
// Invoked when data are received.
//----------------------------------------------------------------------------

void ts::WebRequest::Guts::handleWebReceive(ReactiveWebRequest& req, const ByteBlockPtr& data, int error_code, const ObjectPtr& user_data)
{
    in_status = error_code;

    // Collect input data.
    if (data != nullptr && out_file.is_open()) {
        // Save data to a file.
        data->write(out_file);
        // Report file error.
        if (!out_file && SysSuccess(in_status)) {
            in_status = SYS_ERROR;
        }
    }
    else if (in_data == nullptr || in_start >= in_data->size()) {
        // Internal buffer is unused.
        in_data = data;
        in_start = 0;
    }
    else if (data != nullptr) {
        // Append to internal buffer.
        in_data->append(*data);
    }

    // Exit event loop error (including EOF), so that WebRequest::receive() can get control back.
    if (!SysSuccess(error_code)) {
        reactor.exitEventLoop(open_status);
    }
}


//----------------------------------------------------------------------------
// Receive data.
//----------------------------------------------------------------------------

bool ts::WebRequest::receive(void* buffer, size_t max_size, size_t& ret_size)
{
    ret_size = 0;

    if (!_guts->reactive.isOpen()) {
        report().error(u"web request is not open");
        return false;
    }

    // Receive data until error or until there are some data in the input buffer.
    while (SysSuccess(_guts->in_status) && (_guts->in_data == nullptr || _guts->in_start >= _guts->in_data->size())) {
        _guts->reactor.processEventLoop();
    }

    // If there are some data in the buffer, return them.
    if (_guts->in_data != nullptr && _guts->in_start < _guts->in_data->size()) {
        ret_size = std::min(max_size, _guts->in_data->size() - _guts->in_start);
        MemCopy(buffer, _guts->in_data->data() + _guts->in_start, ret_size);
        _guts->in_start += ret_size;
        if (_guts->in_start >= _guts->in_data->size()) {
            // All buffer is used, drop it.
            _guts->in_data.reset();
        }
        return true;
    }

    // If there is nothing to receive, EOF is not an error (just ret_size == 0).
    SetLastSysErrorCode(_guts->in_status);
    return _guts->in_status == SYS_EOF;
}


//----------------------------------------------------------------------------
// Close the transfer.
//----------------------------------------------------------------------------

bool ts::WebRequest::close()
{
    // Wait for completion of all I/O.
    abort();
    return true;
}


//----------------------------------------------------------------------------
// Abort a transfer in progress.
//----------------------------------------------------------------------------

void ts::WebRequest::abort()
{
    if (_guts->reactor.isOpen()) {
        // Abort all I/O.
        _guts->reactive.abort(true);
        // Wait for an error, which can be EOF if the transfer was already completed.
        while (SysSuccess(_guts->in_status)) {
            _guts->reactor.processEventLoop();
        }
        // Close the reactor.
        _guts->reactor.close(true);
    }
}


//----------------------------------------------------------------------------
// Download the content of the URL as binary data.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadBinaryContent(const UString& url, ByteBlockPtr& data, size_t chunk_size)
{
    // Abort current transfer, if any.
    abort();
    data = nullptr;

    // Transfer initialization.
    if (!open(url, chunk_size)) {
        return false;
    }

    // Receive data in the buffer.
    while (SysSuccess(_guts->in_status)) {
        _guts->reactor.processEventLoop();
    }

    // Return the data.
    data.swap(_guts->in_data);
    return _guts->in_status == SYS_EOF;
}


//----------------------------------------------------------------------------
// Download the content of the URL as text.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadTextContent(const UString& url, UString& text, size_t chunk_size)
{
    // Download the content as raw binary data.
    ByteBlockPtr data;
    if (downloadBinaryContent(url, data, chunk_size) && data != nullptr) {
        // Convert to UTF-8.
        text.assignFromUTF8(reinterpret_cast<const char*>(data->data()), data->size());
        // Remove all CR, just keep the LF.
        text.remove(u'\r');
        return true;
    }
    else {
        // Download error.
        text.clear();
        return false;
    }
}


//----------------------------------------------------------------------------
// Download the content of the URL in a file.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadFile(const UString& url, const fs::path& file_name, size_t chunk_size)
{
    // Abort current transfer, if any.
    abort();

    // Create the output file.
    _guts->out_file.open(file_name, std::ios::out | std::ios::binary);
    if (!_guts->out_file) {
        report().error(u"error creating file %s", file_name);
        return false;
    }

    // Transfer initialization.
    if (!open(url, chunk_size)) {
        _guts->out_file.close();
        return false;
    }

    // Receive data in the file.
    while (SysSuccess(_guts->in_status)) {
        _guts->reactor.processEventLoop();
    }

    // Close the reactor and file.
    _guts->reactor.close(true);
    _guts->out_file.close();
    return _guts->in_status == SYS_EOF;
}
