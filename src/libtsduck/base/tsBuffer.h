//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  General-purpose memory buffer with bit access.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsMemory.h"

namespace ts {
    //!
    //! General-purpose memory buffer with bit access.
    //! @ingroup cpp
    //!
    class TSDUCKDLL Buffer final
    {
    public:
        //!
        //! Default internal size in bytes of a buffer.
        //!
        static constexpr size_t DEFAULT_SIZE = 1024;

        //!
        //! Minimal internal allocation size in bytes of an internal private buffer.
        //!
        static constexpr size_t MINIMUM_SIZE = 16;

        //!
        //! Default constructor.
        //!
        //! The read and write index are at the beginning of the buffer.
        //! So, initially, there is nothing to read and the entire buffer to write.
        //!
        //! @param [in] size Initial internal size in bytes of the buffer.
        //!
        Buffer(size_t size = DEFAULT_SIZE);

        //!
        //! Constructor using an external memory area which must remain valid as long as the Buffer object is used and not reset.
        //!
        //! When @a read_only is true, the read index is at the beginning of the buffer and
        //! the write index is at the end of the buffer. When @a read_only is false,
        //! the read and write index are both at the beginning of the buffer.
        //!
        //! @param [in] data Address of data area to use as memory buffer.
        //! @param [in] size Size in bytes of the data area.
        //! @param [in] read_only The buffer is read-only.
        //!
        Buffer(void* data, size_t size, bool read_only = false);

        //!
        //! Constructor using a read-only external memory area which must remain valid as long as the Buffer object is used and not reset.
        //!
        //! The read index is at the beginning of the buffer and the write index is at the end of the buffer.
        //!
        //! @param [in] data Address of data area to use as memory buffer.
        //! @param [in] size Size in bytes of the data area.
        //!
        Buffer(const void* data, size_t size);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        Buffer(const Buffer& other);

        //!
        //! Move constructor.
        //! @param [in,out] other Other instance to move. Reset and invalidated on return.
        //!
        Buffer(Buffer&& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        Buffer& operator=(const Buffer& other);

        //!
        //! Move-assignment operator.
        //! @param [in,out] other Other instance to move. Reset and invalidated on return.
        //! @return A reference to this object.
        //!
        Buffer& operator=(Buffer&& other);

        //!
        //! Destructor.
        //!
        ~Buffer();

        //!
        //! Check if the buffer is valid and contains some memory.
        //! @return True if the buffer is valid and contains some memory.
        //!
        bool valid() const;

        //!
        //! Check if the buffer is read-only.
        //! @return True if the buffer is read-only.
        //!
        bool readOnly() const { return _read_only; }

        //!
        //! Check if the buffer uses some internal private memory buffer.
        //! @return True if the buffer uses some internal private memory buffer.
        //!
        bool internalMemory() const { return _allocated; }

        //!
        //! Check if the buffer is linked to some external memory area.
        //! @return True if the buffer is linked to some external memory area.
        //!
        bool externalMemory() const { return !_allocated; }

        //!
        //! Get the maximum buffer size in bytes.
        //! @return The maximum buffer size in bytes.
        //!
        size_t capacity() const { return _buffer_size; }

        //!
        //! Get the current buffer size in bytes.
        //! @return The current buffer size in bytes.
        //!
        size_t size() const { return _buffer_max; }

        //!
        //! Specify that read/write operations of integers should use big endian representation.
        //! The endianness of the buffer is not changed by the various reset() operations.
        //!
        void setBigEndian() { _big_endian = true; }

        //!
        //! Specify that read/write operations of integers should use little endian representation.
        //! The endianness of the buffer is not changed by the various reset() operations.
        //!
        void setLittleEndian() { _big_endian = false; }

        //!
        //! Check if read/write operations of integers use big endian representation.
        //! @return True if big endian is used, false if little endian.
        //!
        bool isBigEndian() const { return _big_endian; }

        //!
        //! Check if read/write operations of integers use little endian representation.
        //! @return True if little endian is used, false if big endian.
        //!
        bool isLittleEndian() const { return !_big_endian; }

        //!
        //! Reset the buffer, remove link to any external memory, reallocate an internal buffer if necessary.
        //!
        //! The read and write index are at the beginning of the buffer.
        //! So, initially, there is nothing to read and the entire buffer to write.
        //!
        //! @param [in] size Internal size in bytes of the buffer. If an internal buffer already exists and
        //! is larger than the requested @a size, it is not shrunk.
        //!
        void reset(size_t size);

        //!
        //! Reset the buffer using an external memory area which must remain valid as long as the Buffer object is used and not reset.
        //!
        //! When @a read_only is true, the read index is at the beginning of the buffer and
        //! the write index is at the end of the buffer. When @a read_only is false,
        //! the read and write index are both at the beginning of the buffer.
        //!
        //! @param [in] data Address of data area to use as memory buffer.
        //! @param [in] size Size in bytes of the data area.
        //! @param [in] read_only The buffer is read-only.
        //!
        void reset(void* data, size_t size, bool read_only = false);

        //!
        //! Reset the buffer using a read-only external memory area which must remain valid as long as the Buffer object is used and not reset.
        //!
        //! The read index is at the beginning of the buffer and the write index is at the end of the buffer.
        //!
        //! @param [in] data Address of data area to use as memory buffer.
        //! @param [in] size Size in bytes of the data area.
        //!
        void reset(const void* data, size_t size);

        //!
        //! Check if there was a read error.
        //! @return True if there was a read error.
        //!
        bool readError() const { return _read_error; }

        //!
        //! Check if there was a write error.
        //! @return True if there was a write error.
        //!
        bool writeError() const { return _write_error; }

        //!
        //! Check if there was any kind of error.
        //! @return True if there was any kind of error.
        //!
        bool error() const { return _read_error || _write_error; }

        //!
        //! Clear the read error state.
        //!
        void clearReadError() { _read_error = false; }

        //!
        //! Clear the write error state.
        //!
        void clearWriteError() { _write_error = false; }

        //!
        //! Clear all error states.
        //!
        void clearError() { _read_error = _write_error = false; }

        //!
        //! Reset reading at the specified offset in the buffer.
        //! Seeking past the write pointer moves the read pointer to the write pointer
        //! and generates a read error.
        //! @param [in] byte Index of next byte to read.
        //! @param [in] bit Offset of next bit to read in this byte.
        //! @return True on success, false on error.
        //!
        bool readSeek(size_t byte, size_t bit = 0);

        //!
        //! Reset writing at the specified offset in the buffer.
        //! Seeking backward beyon the read pointer moves the write pointer to the read pointer
        //! and generates a write error. Seeking forward past the end of buffer moves the write
        //! pointer to the end of the buffer and generates a write error.
        //! @param [in] byte Index of next byte to write.
        //! @param [in] bit Offset of next bit to write in this byte.
        //! @param [in] stuffing When seeking forward, byte value to write in skipped bytes.
        //! @return True on success, false on error.
        //!
        bool writeSeek(size_t byte, size_t bit = 0, uint8_t stuffing = 0);

        //!
        //! Check if the current read bit pointer is on a byte boundary.
        //! @return True if the next bit to read is at the beginning of a byte.
        //!
        bool readIsByteAligned() const { return _state.rbit == 0; }

        //!
        //! Check if the current write bit pointer is on a byte boundary.
        //! @return True if the next bit to write is at the beginning of a byte.
        //!
        bool writeIsByteAligned() const { return _state.wbit == 0; }

        //!
        //! Align the read pointer to the next byte boundary if not already aligned.
        //! Skip any bit in a partially read byte.
        //! A read error is generated if this would go beyond the write pointer.
        //! @return A reference to this object.
        //!
        Buffer& readAlignByte();

        //!
        //! Align the write pointer to the next byte boundary if not already aligned.
        //! Fill bits in a partially written byte with a know value.
        //! @param [in] suffing Bit value (must be 0 or 1) to write in skipped bits.
        //! @return A reference to this object.
        //!
        Buffer& writeAlignByte(int stuffing = 0);

        //!
        //! Skip read bytes forward (ignoring bit offset inside bytes).
        //! @param [in] bytes Number of bytes to skip.
        //! @return A reference to this object.
        //!
        Buffer& skipBytes(size_t bytes);

        //!
        //! Skip read bits forward.
        //! @param [in] bits Number of bits to skip.
        //! @return A reference to this object.
        //!
        Buffer& skipBits(size_t bits);

        //!
        //! Skip read bytes backward.
        //! @param [in] bytes Number of bytes to skip back.
        //! @return A reference to this object.
        //!
        Buffer& backBytes(size_t bytes);

        //!
        //! Skip read bits backward.
        //! @param [in] bits Number of bits to skip back.
        //! @return A reference to this object.
        //!
        Buffer& backBits(size_t bits);

        //!
        //! Get current read byte index (ignoring bit offset inside bytes).
        //! @return The offset of the byte to read from the beginning of the buffer.
        //!
        size_t currentReadByteOffset() const { return _state.rbyte; }

        //!
        //! Get current read bit offset from the beginning of the buffer.
        //! @return The offset of the current bit to read from the beginning of the buffer.
        //!
        size_t currentReadBitOffset() const { return 8 * _state.rbyte + _state.rbit; }

        //!
        //! Get current write byte index (ignoring bit offset inside bytes).
        //! @return The offset of the next byte to write from the beginning of the buffer.
        //!
        size_t currentWriteByteOffset() const { return _state.wbyte; }

        //!
        //! Get current write bit offset from the beginning of the buffer.
        //! @return The offset of the next bit to write from the beginning of the buffer.
        //!
        size_t currentWriteBitOffset() const { return 8 * _state.wbyte + _state.wbit; }

        //!
        //! Get number of remaining bytes to read (ignoring bit offset inside bytes).
        //! @return The number of remaining bytes to read.
        //!
        size_t remainingReadBytes() const;

        //!
        //! Get number of remaining bits to read.
        //! @return The number of remaining bits to read.
        //!
        size_t remainingReadBits() const;

        //!
        //! Get number of remaining bytes to write (ignoring bit offset inside bytes).
        //! @return The number of remaining bytes to write.
        //!
        size_t remainingWriteBytes() const;

        //!
        //! Get number of remaining bits to write.
        //! @return The number of remaining bits to write.
        //!
        size_t remainingWriteBits() const;

        //!
        //! Check end of read stream.
        //! @return True if the end of read stream is reached.
        //!
        bool endOfRead() const { return _state.rbyte == _state.wbyte && _state.rbit == _state.wbit; }

        //!
        //! Check end of write stream.
        //! @return True if the end of write stream is reached.
        //!
        bool endOfWrite() const { return _state.wbyte >= _buffer_max; }

        //!
        //! Read the next bit and advance the bitstream pointer.
        //! @param [in] def Default value to return if already at end of stream.
        //! @return The value of the next bit.
        //!
        uint8_t readBit(uint8_t def = 0);

        //!
        //! Read the next n bits as an integer value and advance the bitstream pointer.
        //! @tparam INT An integer type for the result.
        //! @param [in] bits Number of bits to read.
        //! @param [in] def Default value to return if less than @a n bits before end of stream.
        //! @return The value of the next @a bits.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        INT readBits(size_t bits, INT def = 0);

        //!
        //! Push the current state of the read/write streams on a stack of saved states.
        //!
        //! There is an internal stack of read/write states. It is possible to save the current
        //! state of the buffer, try to do some operations and then either restore (pop) the
        //! previous state if the attempted operations failed or drop the saved state and
        //! continue with the new state.
        //!
        //! @return The level of pushed state (0 for the first push, then 1, etc.)
        //! The returned level can be used by popReadWriteState() and dropReadWriteState().
        //! @see popReadWriteState()
        //! @see dropReadWriteState()
        //!
        size_t pushReadWriteState();

        //!
        //! Restore the current state of the read/write streams from the stack of saved states.
        //! @param [in] level Saved level to restore. The default is NPOS, meaning the last
        //! saved state. Another inner level can be specified, in which case all outer levels
        //! are dropped.
        //! @return True on success. False if there is no saved state or @a level does not exist.
        //! @see pushReadWriteState()
        //!
        bool popReadWriteState(size_t level = NPOS);

        //!
        //! Drop the last saved state of the read/write streams from the stack of saved states.
        //! @param [in] level Saved level to drop. The default is NPOS, meaning the last
        //! saved state. Another inner level can be specified, in which case the specified level
        //! and all outer levels are dropped.
        //! @return True on success. False if there is no saved state or @a level does not exist.
        //! @see pushReadWriteState()
        //!
        bool dropReadWriteState(size_t level = NPOS);

        //!
        //! Change the usable size of the buffer.
        //! @param [in] size New usable size in bytes of the buffer. In some cases, the final granted
        //! size can be different:
        //! - If @a size is lower than the current write pointer, the new usable size is set to the
        //!   current write pointer.
        //! - If @a size is greater than the current capacity() and the buffer is an external memory
        //!   or @a reallocate is false, the new usable size is set to the current capacity().
        //! @param [in] reallocate If true and the buffer is internally allocated, then reallocate
        //! the internal buffer to the final accepted size.
        //! @return True in case of success. False if the requested @a size could not be granted.
        //! When the result is false, call size() to get the new actual buffer size.
        //! @see size()
        //! @see capacity()
        //!
        bool resize(size_t size, bool reallocate);

        //!
        //! Temporary change the new usable size of the buffer.
        //! The current value is pushed to an internal stack and can be restored later.
        //! @param [in] size New usable size in bytes of the buffer. In some cases, the final granted
        //! size can be different. The final value is bounded by the current write pointer (lower bound)
        //! and the current capacity() (upper bound).
        //! @return The level of pushed state (0 for the first push, then 1, etc.)
        //! The returned level can be used by popSize() and dropSize().
        //! @see popSize()
        //! @see dropSize()
        //!
        size_t pushSize(size_t size);

        //!
        //! Restore the current buffer size from the stack of saved size.
        //! @param [in] level Saved level to restore. The default is NPOS, meaning the last
        //! saved size. Another inner level can be specified, in which case all outer levels
        //! are dropped.
        //! @return True on success. False if there is no saved size or @a level does not exist.
        //! @see pushSize()
        //!
        bool popSize(size_t level = NPOS);

        //!
        //! Drop the last saved buffer size from the stack of saved sizes.
        //! @param [in] level Saved level to drop. The default is NPOS, meaning the last
        //! saved size. Another inner level can be specified, in which case the specified level
        //! and all outer levels are dropped.
        //! @return True on success. False if there is no saved size or @a level does not exist.
        //! @see pushSize()
        //!
        bool dropSize(size_t level = NPOS);

    private:
        // Read/write state in the buffer.
        struct RWState {
            RWState();     // Constructor.
            size_t rbyte;  // Next byte to read, offset from beginning of buffer.
            size_t wbyte;  // Next byte to write, offset from beginning of buffer.
            size_t rbit;   // Next bit to read at offset rbyte (0 = MSB in big endian, LSB in little endian).
            size_t wbit;   // Next bit to write at offset wbyte (0 = MSB in big endian, LSB in little endian).
        };

        uint8_t* _buffer;       // Base address of memory buffer.
        size_t   _buffer_size;  // Size of addressable area in _buffer.
        size_t   _buffer_max;   // Size of usable area in _buffer.
        bool     _read_only;    // The buffer is in read-only mode.
        bool     _allocated;    // If true, _buffer was internally allocated and must be freed later.
        bool     _big_endian;   // Read/write integers in big endian mode (false means little endian).
        bool     _read_error;   // Read error encountered (passed end of stream for instance).
        bool     _write_error;  // Write error encountered (passed end of stream for instance).
        RWState  _state;        // Read/write indexes.
        std::vector<size_t>  _saved_max;     // Stack of saved _buffer_max.
        std::vector<RWState> _saved_states;  // Stack of saved states.
    };
}

#include "tsBufferTemplate.h"
