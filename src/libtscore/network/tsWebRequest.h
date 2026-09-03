//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Perform a simple Web request (HTTP, HTTPS, FTP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDevice.h"
#include "tsWebRequestArgs.h"
#include "tsWebRequestStatus.h"
#include "tsByteBlock.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Perform a simple Web request (HTTP, HTTPS, FTP).
    //! @ingroup libtscore net
    //!
    //! On UNIX systems, the implementation uses libcurl.
    //! On Windows systems, the implementation uses Microsoft Wininet.
    //! We could have used libcurl on Windows but building it was a pain...
    //!
    //! The proxy and transfer settings must be set before starting any
    //! download operation. The HTTP status and the response headers are
    //! available after a successful download start.
    //!
    //! By default, no proxy is used. If no proxy is set, the default proxy
    //! is used (system configuration on Windows, http_proxy environment on
    //! Unix systems).
    //!
    class TSCOREDLL WebRequest: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(WebRequest);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit WebRequest(Report* report);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //!
        explicit WebRequest(ReporterBase* delegate);

        //!
        //! Destructor.
        //!
        virtual ~WebRequest() override;

        //!
        //! Access the Web request arguments to get or set them.
        //! @return A non-const reference to the Web request arguments.
        //!
        WebRequestArgs& args();

        //!
        //! Access the Web request arguments to get them.
        //! @return A const reference to the Web request arguments.
        //!
        const WebRequestArgs& args() const;

        //!
        //! Access the Web request status.
        //! @return A const reference to the Web request status.
        //!
        const WebRequestStatus& status() const;

        //!
        //! Open an URL and start the transfer.
        //! For HTTP request, perform all redirections and get response headers.
        //! @param [in] url The complete URL to fetch.
        //! @param [in] buffer_size Size of input buffers to receive data.
        //! @return True on success, false on error.
        //!
        bool open(const UString& url, size_t buffer_size = Device::DEFAULT_RECEIVE_BUFFER_SIZE);

        //!
        //! Check if a transfer is open.
        //! @return True if a transfer is open, false otherwise.
        //!
        bool isOpen() const;

        //!
        //! Receive data.
        //!
        //! @param [out] buffer Address of the buffer for the received data.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received data. Will never be larger than @a max_size.
        //! When @a ret_size is zero, this is the end of the transfer.
        //! @return True on success, false on error. A successful end of transfer is reported when
        //! @a ret_size is zero and the returned value is true.
        //!
        bool receive(void* buffer, size_t max_size, size_t& ret_size);

        //!
        //! Close the transfer.
        //! @return True on success, false on error.
        //!
        bool close();

        //!
        //! Abort a transfer in progress.
        //! Can be called from another thread.
        //!
        void abort();

        //!
        //! Download the content of the URL as binary data in one operation.
        //! The open/read/close session is embedded in this method.
        //! The request must be in blocking mode (the default).
        //! @param [in] url The complete URL to fetch.
        //! @param [out] data The content of the URL in a shred pointer. Null in case of error.
        //! @param [in] chunk_size Individual download chunk size.
        //! @return True on success, false on error.
        //!
        bool downloadBinaryContent(const UString& url, ByteBlockPtr& data, size_t chunk_size = Device::DEFAULT_RECEIVE_BUFFER_SIZE);

        //!
        //! Download the content of the URL as text in one operation.
        //! The open/read/close session is embedded in this method..
        //! The downloaded text is converted from UTF-8.
        //! End of lines are normalized as LF.
        //! The request must be in blocking mode (the default).
        //! @param [in] url The complete URL to fetch.
        //! @param [out] text The content of the URL.
        //! @param [in] chunk_size Individual download chunk size.
        //! @return True on success, false on error.
        //!
        bool downloadTextContent(const UString& url, UString& text, size_t chunk_size = Device::DEFAULT_RECEIVE_BUFFER_SIZE);

        //!
        //! Download the content of the URL in a file in one operation.
        //! The open/read/close session is embedded in this method..
        //! No transformation is applied to the data.
        //! The request must be in blocking mode (the default).
        //! @param [in] url The complete URL to fetch.
        //! @param [in] file_name Name of the file to create.
        //! @param [in] chunk_size Individual download chunk size.
        //! @return True on success, false on error.
        //!
        bool downloadFile(const UString& url, const fs::path& file_name, size_t chunk_size = Device::DEFAULT_RECEIVE_BUFFER_SIZE);

    private:
        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class Guts;
        Guts* _guts = nullptr;
    };
}
