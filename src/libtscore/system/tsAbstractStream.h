//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class to read/write raw data from/to a stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterInterface.h"
#include "tsAbortInterface.h"
#include "tsNonBlockingDevice.h"

namespace ts {
    //!
    //! Abstract base class to read/write raw data from/to a stream.
    //! @ingroup libtscore system
    //!
    class TSCOREDLL AbstractStream
    {
        TS_NOBUILD_NOCOPY(AbstractStream);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reporter Reference to an object providing a Report.
        //! This reference is kept in the constructed object instance.
        //!
        AbstractStream(ReporterInterface& reporter) : _reporter(reporter) {}

        //!
        //! Virtual destructor.
        //!
        virtual ~AbstractStream();

        //!
        //! Write data to the stream.
        //! All bytes are written to the stream, blocking or retrying when necessary.
        //! @param [in] addr Address of the data to write.
        //! @param [in] size Size in bytes of the data to write.
        //! @param [out] written_size Actually written size in bytes.
        //! Can be less than @a size in case of error in the middle of the write.
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, the stream must be in non-blocking mode.
        //! When null, the stream must be in blocking mode (the default). See the description of ts::NonBlockingDevice::IOSB.
        //! Important: The parameter @a iosb should not be used by applications. It should be used only by
        //! "reactive classes", which work in combination with a Reactor.
        //! @return True on success, false on error.
        //!
        virtual bool writeStream(const void* addr, size_t size, size_t& written_size, NonBlockingDevice::IOSB* iosb = nullptr) = 0;

        //!
        //! Read data from the stream.
        //! Wait and read at least one byte. Don't try to read exactly @a max_size bytes.
        //! If @a ret_size is less than @a max_bytes, it is possible to read more.
        //! @param [out] addr Address of the buffer for the incoming data.
        //! @param [in] max_size Maximum size in bytes of the buffer.
        //! @param [out] ret_size Returned input size in bytes.
        //! If zero, end of file has been reached or an error occurred.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, the stream must be in non-blocking mode.
        //! When null, the stream must be in blocking mode (the default). See the description of ts::NonBlockingDevice::IOSB.
        //! Important: The parameter @a iosb should not be used by applications. It should be used only by
        //! "reactive classes", which work in combination with a Reactor.
        //! @return True on success, false on error.
        //!
        virtual bool readStream(void* addr, size_t max_size, size_t& ret_size, const AbortInterface* abort = nullptr, NonBlockingDevice::IOSB* iosb = nullptr) = 0;

        //!
        //! Read complete data from the stream.
        //! Wait and read exactly @a max_size bytes. If @a ret_size is less than @a max_bytes,
        //! it is not possible to read more. End of file has probably been reached.
        //! There is no @a iosb parameter because this method uses blocking I/O by design.
        //! @param [out] addr Address of the buffer for the incoming data.
        //! @param [in] max_size Maximum size in bytes of the buffer.
        //! @param [out] ret_size Returned input size in bytes.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //!
        virtual bool readStreamComplete(void* addr, size_t max_size, size_t& ret_size, const AbortInterface* abort = nullptr);

        //!
        //! Read chunks of data from the stream.
        //! There is no @a iosb parameter because this method uses blocking I/O by design.
        //! @param [out] addr Address of the buffer for the incoming data.
        //! @param [in] max_size Maximum size in bytes of the buffer.
        //! @param [in] chunk_size If not zero, make sure that the input size is always
        //! a multiple of @a chunk_size. If the initial read ends in the middle of a @e chunk,
        //! read again and again, up to the end of the current chunk or end of file.
        //! @param [out] ret_size Returned input size in bytes.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //!
        virtual bool readStreamChunks(void* addr, size_t max_size, size_t chunk_size, size_t& ret_size, const AbortInterface* abort = nullptr);

        //!
        //! Check if the end of stream was reached while reading.
        //! @return True on end of stream, false otherwise.
        //!
        virtual bool endOfStream() = 0;

    private:
        ReporterInterface& _reporter;
    };
}
