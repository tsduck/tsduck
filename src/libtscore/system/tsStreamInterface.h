//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class to read/write raw data from/to a stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbortInterface.h"
#include "tsNonBlockingDevice.h"

namespace ts {
    //!
    //! Interface class to read/write raw data from/to a stream.
    //! @ingroup libtscore system
    //!
    class TSCOREDLL StreamInterface
    {
        TS_INTERFACE(StreamInterface);
    public:
        //!
        //! Write some data to the stream.
        //! All bytes are written to the stream, blocking or retrying when necessary when the stream is in blocking mode.
        //! Return the number of actually written bytes if some error occurred before writing everything.
        //! @param [in] addr Address of the data to write.
        //! @param [in] size Size in bytes of the data to write.
        //! @param [out] written_size Actually written size in bytes.
        //! Can be less than @a size in case of error in the middle of the write.
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, the stream must be in non-blocking mode.
        //! When null, the stream must be in blocking mode (the default). See the description of ts::NonBlockingDevice::IOSB.
        //! Important: The parameter @a iosb should not be used by applications. It should be used only by
        //! "reactive classes", which work in combination with a Reactor.
        //! @return True on success, false on error. In case of non-blocking mode, if the I/O is successfully started
        //! but still pending, iosb->pending is set to true and the method returns true.
        //!
        virtual bool writeStream(const void* addr, size_t size, size_t& written_size, NonBlockingDevice::IOSB* iosb = nullptr) = 0;

        //!
        //! Write data to the stream.
        //! All bytes are written to the stream, blocking or retrying when necessary when the stream is in blocking mode.
        //! The base implementation of writeStream() uses the virtual version with a @a written_size output parameter.
        //! @param [in] addr Address of the data to write.
        //! @param [in] size Size in bytes of the data to write.
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, the stream must be in non-blocking mode.
        //! When null, the stream must be in blocking mode (the default). See the description of ts::NonBlockingDevice::IOSB.
        //! Important: The parameter @a iosb should not be used by applications. It should be used only by
        //! "reactive classes", which work in combination with a Reactor.
        //! @return True on success, false on error or it less than @a size bytes could be written. In case of non-blocking mode,
        //! if the I/O is successfully started but still pending, iosb->pending is set to true and the method returns true.
        //!
        virtual bool writeStream(const void* addr, size_t size, NonBlockingDevice::IOSB* iosb = nullptr) = 0;

        //!
        //! Read some data from the stream.
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
        //! @return True on success, false on error. In case of non-blocking mode, if the I/O is successfully started
        //! but still pending, iosb->pending is set to true and the method returns true.
        //!
        virtual bool readStream(void* addr, size_t max_size, size_t& ret_size, const AbortInterface* abort = nullptr, NonBlockingDevice::IOSB* iosb = nullptr) = 0;

        //!
        //! Read complete data from the stream.
        //! Read exactly @a size bytes, waiting if necessary.
        //!
        //! Synchronization: There is no @a iosb parameter because this method uses blocking I/O by design.
        //! An error is returned if the instance of the class which implements StreamInterface is in non-blocking mode.
        //!
        //! @param [out] addr Address of the buffer for the incoming data.
        //! @param [in] size Size in bytes of the buffer.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error or it less than @a size bytes could be read.
        //!
        virtual bool readStream(void* addr, size_t size, const AbortInterface* abort = nullptr) = 0;

        //!
        //! Update the status of an asynchronous readStream() or writeStream() after it completed.
        //! This method applies to asynchronous I/O only (Windows), not non-blocking I/O (UNIX).
        //! @param [in,out] iosb Address of the IOSB structure which was used when readStream() or writeStream() was called.
        //! @return True on success, false on error.
        //! 
        virtual bool asyncCompletedStream(NonBlockingDevice::IOSB* iosb) = 0;

        //!
        //! Check if the stream is open for read.
        //! @return True if readStream() is possible, false otherwise (write-only, closed, or disconnected stream).
        //!
        virtual bool isReadStream() = 0;

        //!
        //! Check if the stream is open for write.
        //! @return True if writeStream() is possible, false otherwise (read-only, closed, or disconnected stream).
        //!
        virtual bool isWriteStream() = 0;

        //!
        //! Check if the end of stream was reached while reading.
        //! @return True on end of stream, false otherwise.
        //!
        virtual bool endOfStream() = 0;

    protected:
        //!
        //! Implementation helper for fixed-size version of writeStream().
        //!
        //! Often, the fixed-size version can be implemented using the variable-size version. However, this works well
        //! at a given derivation stage only.
        //!
        //! Assume that:
        //! - We implement the fixed-size version inside StreamInterface, using the virtual variable-size version.
        //! - Class A implements StreamInterface and implements the variable-size version only. The base fixed-size version
        //!   StreamInterface::writeStream() uses the virtual variable-size version A::writeStream() and it works well.
        //! - Class B derives from A. It is an encrypted version of A (think A=TCP, B=TLS). If the implementation of B wants
        //!   to write raw data (after encryption) calling the fixed-size version A::writeStream(), then this will end up
        //!   calling the virtual variable-size version B::writeStream(), which will try to encrypt already encrypted data.
        //!   This is why it is important that each layer of inheritance calls its own version of writeStream().
        //!
        //! @tparam T A subclass of StreamInterface.
        //! Write data to the stream.
        //! All bytes are written to the stream, blocking or retrying when necessary when the stream is in blocking mode.
        //! The base implementation of writeStream() uses the virtual version with a @a written_size output parameter.
        //! @param [in,out] obj Object of class T.
        //! @param [in] addr Address of the data to write.
        //! @param [in] size Size in bytes of the data to write.
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, the stream must be in non-blocking mode.
        //! When null, the stream must be in blocking mode (the default). See the description of ts::NonBlockingDevice::IOSB.
        //! Important: The parameter @a iosb should not be used by applications. It should be used only by
        //! "reactive classes", which work in combination with a Reactor.
        //! @return True on success, false on error or it less than @a size bytes could be written. In case of non-blocking mode,
        //! if the I/O is successfully started but still pending, iosb->pending is set to true and the method returns true.
        //!
        template <class T> requires std::derived_from<T, StreamInterface>
        static bool WriteStreamHelper(T*obj, const void* addr, size_t size, NonBlockingDevice::IOSB* iosb)
        {
            size_t written_size = 0;
            return obj->T::writeStream(addr, size, written_size, iosb) && ((iosb != nullptr && iosb->pending) || written_size == size);
        }

        //!
        //! Implementation helper for fixed-size version of readStream().
        //! See writeStreamHelper() for a rationale.
        //!
        //! @tparam T A subclass of StreamInterface.
        //! @param [in,out] obj Object of class T.
        //! @param [out] addr Address of the buffer for the incoming data.
        //! @param [in] size Size in bytes of the buffer.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error or it less than @a size bytes could be read.
        //!
        template <class T> requires std::derived_from<T, StreamInterface>
        static bool ReadStreamHelper(T* obj, void* addr, size_t size, const AbortInterface* abort)
        {
            uint8_t* data = reinterpret_cast<uint8_t*>(addr);
            size_t insize = 0;

            while (size > 0) {
                if (!obj->T::readStream(data, size, insize, abort) || insize == 0) {
                    return false;
                }
                assert(insize <= size);
                data += insize;
                size -= insize;
            }
            return true;
        }
    };
}
