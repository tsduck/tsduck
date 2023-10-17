//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for HTTP-based input plugins.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"
#include "tsTSFile.h"
#include "tsWebRequest.h"
#include "tsWebRequestArgs.h"

namespace ts {
    //!
    //! Abstract base class for HTTP-based input plugins.
    //! @ingroup plugin
    //!
    class TSDUCKDLL AbstractHTTPInputPlugin: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(AbstractHTTPInputPlugin);
    public:
        // Implementation of Plugin interface.
        // If overridden by subclass, superclass must be explicitly invoked.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool abortInput() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool setReceiveTimeout(MilliSecond timeout) override;

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] tsp Object to communicate with the Transport Stream Processor main executable.
        //! @param [in] description A short one-line description, eg. "Descrambler for 'xyz' CAS".
        //! @param [in] syntax A short one-line syntax summary, default: u"[options] [service]".
        //!
        AbstractHTTPInputPlugin(TSP* tsp, const UString& description, const UString& syntax);

        //!
        //! Open an URL.
        //! This abstract method must be implemented by subclasses.
        //! It is invoked repeatedly by the superclass at the end of each download.
        //! @param [in,out] request The request object to open.
        //! @return True on success, false on error or when no more download shall be performed.
        //!
        virtual bool openURL(WebRequest& request) = 0;

        //!
        //! Set a directory name where all loaded files are automatically saved.
        //! @param [in] dir A directory name.
        //!
        void setAutoSaveDirectory(const UString dir) { _autoSaveDir = dir; }

        //!
        //! Delete the cookies file, if there is one.
        //! @return True on success, false on error.
        //!
        bool deleteCookiesFile() { return _request.deleteCookiesFile(); }

        //!
        //! Web command line options can be accessed by subclasses for additional web operations.
        //!
        WebRequestArgs webArgs {};

    private:
        WebRequest _request;          // Current Web transfer in progress.
        TSPacket   _partial {};       // Buffer for incomplete packets.
        size_t     _partialSize = 0;  // Number of bytes in partial.
        UString    _autoSaveDir {};   // If not empty, automatically save loaded files to this directory.
        TSFile     _outSave {};       // TS file where to store the loaded file.

        // Start/receive/stop on one single transfer.
        bool startTransfer();
        size_t receiveTransfer(TSPacket*, size_t);
        bool stopTransfer();
    };
}
