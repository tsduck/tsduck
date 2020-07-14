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
#include "tsMemory.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! General-purpose memory buffer with bit access.
    //! @ingroup cpp
    //!
    //! A buffer has the following properties:
    //! - Internal memory space (freeed with the buffer object) or external memory area.
    //! - Access mode: read/write or read-only.
    //! - Maximum size (in bytes).
    //! - Read pointer (in bits).
    //! - Write pointer (in bits).
    //! - Error state (read error, write error, user-generated error).
    //! - Endianness: byte and bit order, used when reading or writing integer data.
    //!
    //! In a read/write buffer, both read and write pointers initially point to the start
    //! of the buffer. Then, the read pointer always remains behind the write pointer. In
    //! other words, we can read only what was previously written. The application cannot
    //! write beyond the current maximum buffer size and cannot read beyond the current
    //! write pointer.
    //!
    //! In a read-only buffer, the write pointer always points to the end of the buffer
    //! and cannot be changed.
    //!
    //! Read and write pointers are composed of a byte offset from the beginning of the
    //! buffer and a bit offset (0 to 7) in this byte. In big endian mode (the default),
    //! bit 0 is the most significant bit (msb) and bit 7 is the least significant bit
    //! (lsb). In little endian mode, bit 0 is the lsb and bit 7 is the msb.
    //!
    //! It is possible to read and write integer values of any number of bits, starting
    //! at any bit offset. Best performances are, of course, achieved on 8, 16, 32 and
    //! 64-bit integers starting at a byte boundary (bit offset 0).
    //!
    //! The two read-error and write-error states are independent. They are most commonly
    //! set when trying to read or write past the write pointer or end of buffer, respectively.
    //! When some multi-byte data cannot be read or written, the corresponding error is set
    //! and the read or write pointer is left unmodified (no partial read or write).
    //!
    //! Once the read error is set, all subsequent read operations will fail until the
    //! read error state is explicitly cleared. The same principle applies to write error
    //! state and write operations.
    //!
    //! For example, consider that the remaining number of bytes to read is 2. Trying to
    //! read a 32-bit data fails and the read error is set. Attempting to read an 8-bit
    //! or 16-bit data will then fail because the read error is set, even though there
    //! are enough bytes to read.
    //!
    //! Note: The principles of the C++ class ts::Buffer were freely inspired by the Java
    //! class @c java.nio.ByteBuffer. There are differences between the two but the main
    //! principles are similar.
    //!
    class TSDUCKDLL Buffer
    {
    public:
        //!
        //! Default internal size in bytes of a buffer.
        //!
        static constexpr size_t DEFAULT_SIZE = 1024;

        //!
        //! Minimal internal allocation size (capacity) in bytes of an internal private buffer.
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
        bool isValid() const;

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
        //! Get the current base address of the buffer.
        //! @return A constant pointer the base of the buffer.
        //!
        const uint8_t* data() const { return _buffer; }

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
        //! Check if there was a user-generated error.
        //! @return True if there was a user-generated error.
        //! @see setUserError()
        //!
        bool userError() const { return _user_error; }

        //!
        //! Check if there was any kind of error.
        //! @return True if there was any kind of error.
        //!
        bool error() const { return _read_error || _write_error || _user_error; }

        //!
        //! Clear the read error state.
        //!
        void clearReadError() { _read_error = false; }

        //!
        //! Clear the write error state.
        //!
        void clearWriteError() { _write_error = false; }

        //!
        //! Clear the user-generated error state.
        //!
        void clearUserError() { _user_error = false; }

        //!
        //! Clear all error states.
        //!
        void clearError() { _read_error = _write_error = _user_error = false; }

        //!
        //! Set the user-generated error state.
        //! This can be used to indicate an application error such as invalid data format for instance.
        //!
        void setUserError() { _user_error = true; }

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
        //! @return True on success, false on error.
        //!
        bool writeSeek(size_t byte, size_t bit = 0);

        //!
        //! Reset writing at the specified offset in the buffer and trash forward memory.
        //! Seeking backward beyon the read pointer moves the write pointer to the read pointer
        //! and generates a write error. Seeking forward past the end of buffer moves the write
        //! pointer to the end of the buffer and generates a write error.
        //! @param [in] byte Index of next byte to write.
        //! @param [in] bit Offset of next bit to write in this byte.
        //! @param [in] stuffing When seeking forward, byte value to write in skipped bytes.
        //! @return True on success, false on error.
        //!
        bool writeSeek(size_t byte, size_t bit, uint8_t stuffing);

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
        //! @return True on success, false if would got beyond write pointer (and set read error flag).
        //!
        bool readRealignByte();

        //!
        //! Align the write pointer to the next byte boundary if not already aligned.
        //! Fill bits in a partially written byte with a know value.
        //! @param [in] stuffing Bit value (must be 0 or 1) to write in skipped bits.
        //! @return Always true.
        //!
        bool writeRealignByte(int stuffing = 0);

        //!
        //! Skip read bytes forward (ignoring bit offset inside bytes).
        //! @param [in] bytes Number of bytes to skip.
        //! @return True on success, false if would got beyond write pointer (and set read error flag).
        //!
        bool skipBytes(size_t bytes);

        //!
        //! Skip read bits forward.
        //! @param [in] bits Number of bits to skip.
        //! @return True on success, false if would got beyond write pointer (and set read error flag).
        //!
        bool skipBits(size_t bits);

        //!
        //! Skip read bytes backward.
        //! @param [in] bytes Number of bytes to skip back.
        //! @return True on success, false if would got beyond start of buffer (and set read error flag).
        //!
        bool backBytes(size_t bytes);

        //!
        //! Skip read bits backward.
        //! @param [in] bits Number of bits to skip back.
        //! @return True on success, false if would got beyond start of buffer (and set read error flag).
        //!
        bool backBits(size_t bits);

        //!
        //! Get starting address of current read data (ignoring bit offset inside first byte to read).
        //! @return The starting address of current read data.
        //!
        const uint8_t* currentReadAddress() const { return _buffer + _state.rbyte; }

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
        //! Swap the current state of the read/write streams with the one on top of the stack of saved states.
        //!
        //! As a result, the previously saved state is restore and the current state (just before
        //! restoring the saved state) is pushed. If there was no saved state, the current state
        //! is unchanged but still saved. So it is always safe to assume that the current state
        //! was savded.
        //!
        //! @return The level of pushed state (0 for the first push, then 1, etc.)
        //! The returned level can be used by popReadWriteState() and dropReadWriteState().
        //! @see pushReadWriteState()
        //!
        size_t swapReadWriteState();

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
        //! Get the current number of pushed states of the read/write streams.
        //! @return The current number of pushed states of the read/write streams.
        //!
        size_t pushedReadWriteStateLevels() const { return _saved_states.size(); }

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

        //!
        //! Read the next bit and advance the read pointer.
        //! @param [in] def Default value to return if already at end of stream.
        //! @return The value of the next bit.
        //!
        uint8_t getBit(uint8_t def = 0);

        //!
        //! Write the next bit and advance the write pointer.
        //! @param [in] bit The bit value (0 or 1).
        //! @return True on success, false on error (read only or no more space to write).
        //!
        bool putBit(uint8_t bit);

        //!
        //! Read the next n bits as an integer value and advance the read pointer.
        //! @tparam INT An integer type for the result.
        //! @param [in] bits Number of bits to read.
        //! @param [in] def Default value to return if less than @a n bits before end of stream.
        //! @return The value of the next @a bits.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        INT getBits(size_t bits, INT def = 0);

        //!
        //! Put the next n bits from an integer value and advance the write pointer.
        //! @tparam INT An integer type.
        //! @param [in] value Integer value to write.
        //! @param [in] bits Number of bits to write.
        //! @return True on success, false on error (read only or no more space to write).
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool putBits(INT value, size_t bits);

        //!
        //! Get bulk bytes from the buffer.
        //! The bit aligment is ignored, reading starts at the current read byte pointer,
        //! even if a few bits were already read from that byte.
        //! @param [out] buffer Address of the buffer receiving the read bytes.
        //! @param [in] bytes Number of bytes to read.
        //! @return Actual number of returned bytes. If the requested number of bytes is not
        //! available, return as much as possible and set the read error.
        //!
        size_t getBytes(uint8_t* buffer, size_t bytes);

        //!
        //! Get bulk bytes from the buffer.
        //! The bit aligment is ignored, reading starts at the current read byte pointer,
        //! even if a few bits were already read from that byte.
        //! @param [in] bytes Number of bytes to read.
        //! @return Read data as a byte block. If the requested number of bytes is not
        //! available, return as much as possible and set the read error.
        //!
        ByteBlock getByteBlock(size_t bytes);

        //!
        //! Get bulk bytes from the buffer.
        //! The bit aligment is ignored, reading starts at the current read byte pointer,
        //! even if a few bits were already read from that byte.
        //! @param [in,out] bb Byte block receiving the read bytes. The read data are appended to @a bb.
        //! @param [in] bytes Number of bytes to read.
        //! @return Actual number of appended bytes. If the requested number of bytes is not
        //! available, return as much as possible and set the read error.
        //!
        size_t getByteBlockAppend(ByteBlock& bb, size_t bytes);

        //!
        //! Put bytes in the buffer.
        //! @param [in] buffer Address of the data to write.
        //! @param [in] bytes Number of bytes to write.
        //! @return Actual number of written bytes. If the requested number of bytes is not
        //! available, write as much as possible and set the write error.
        //!
        size_t putBytes(const uint8_t* buffer, size_t bytes);

        //!
        //! Put bulk bytes in the buffer.
        //! The bit aligment is ignored, writing starts at the current write byte pointer,
        //! even if a few bits were already written in that byte.
        //! @param [in] bb Byte block containing the data to write.
        //! @param [in] start Start in index in @a bb.
        //! @param [in] count Number of bytes to write.
        //! @return Actual number of written bytes. If the requested number of bytes is not
        //! available, write as much as possible and set the write error.
        //!
        size_t putBytes(const ByteBlock& bb, size_t start = 0, size_t count = NPOS);

        //!
        //! Read the next 8 bits as an unsigned integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value.
        //!
        uint8_t getUInt8() { return *rdb(1); }

        //!
        //! Read the next 16 bits as an unsigned integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        uint16_t getUInt16() { return _big_endian ? GetUInt16BE(rdb(2)) : GetUInt16LE(rdb(2)); }

        //!
        //! Read the next 24 bits as an unsigned integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        uint32_t getUInt24() { return _big_endian ? GetUInt24BE(rdb(3)) : GetUInt24LE(rdb(3)); }

        //!
        //! Read the next 32 bits as an unsigned integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        uint32_t getUInt32() { return _big_endian ? GetUInt32BE(rdb(4)) : GetUInt32LE(rdb(4)); }

        //!
        //! Read the next 40 bits as an unsigned integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        uint64_t getUInt40() { return _big_endian ? GetUInt40BE(rdb(5)) : GetUInt40LE(rdb(5)); }

        //!
        //! Read the next 48 bits as an unsigned integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        uint64_t getUInt48() { return _big_endian ? GetUInt48BE(rdb(6)) : GetUInt48LE(rdb(6)); }

        //!
        //! Read the next 64 bits as an unsigned integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        uint64_t getUInt64() { return _big_endian ? GetUInt64BE(rdb(8)) : GetUInt64LE(rdb(8)); }

        //!
        //! Read the next 8 bits as a signed integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value.
        //!
        int8_t getInt8() { return static_cast<int8_t>(*rdb(1)); }

        //!
        //! Read the next 16 bits as a signed integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        int16_t getInt16() { return _big_endian ? GetInt16BE(rdb(2)) : GetInt16LE(rdb(2)); }

        //!
        //! Read the next 24 bits as a signed integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        int32_t getInt24() { return _big_endian ? GetInt24BE(rdb(3)) : GetInt24LE(rdb(3)); }

        //!
        //! Read the next 32 bits as a signed integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        int32_t getInt32() { return _big_endian ? GetInt32BE(rdb(4)) : GetInt32LE(rdb(4)); }

        //!
        //! Read the next 40 bits as a signed integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        int64_t getInt40() { return _big_endian ? GetInt40BE(rdb(5)) : GetInt40LE(rdb(5)); }

        //!
        //! Read the next 48 bits as a signed integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        int64_t getInt48() { return _big_endian ? GetInt48BE(rdb(6)) : GetInt48LE(rdb(6)); }

        //!
        //! Read the next 64 bits as a signed integer value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded integer value, according to the current endianness of the buffer.
        //!
        int64_t getInt64() { return _big_endian ? GetInt64BE(rdb(8)) : GetInt64LE(rdb(8)); }

        //!
        //! Write an 8-bit unsigned integer value and advance the write pointer.
        //! @param [in] i 8-bit unsigned integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUInt8(uint8_t i) { return putint(i, 1, PutUInt8, PutUInt8); }

        //!
        //! Write a 16-bit unsigned integer value and advance the write pointer.
        //! @param [in] i 16-bit unsigned integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUInt16(uint16_t i) { return putint(i, 2, PutUInt16BE, PutUInt16LE); }

        //!
        //! Write a 24-bit unsigned integer value and advance the write pointer.
        //! @param [in] i 24-bit unsigned integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUInt24(uint32_t i) { return putint(i, 3, PutUInt24BE, PutUInt24LE); }

        //!
        //! Write a 32-bit unsigned integer value and advance the write pointer.
        //! @param [in] i 32-bit unsigned integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUInt32(uint32_t i) { return putint(i, 4, PutUInt32BE, PutUInt32LE); }

        //!
        //! Write a 40-bit unsigned integer value and advance the write pointer.
        //! @param [in] i 40-bit unsigned integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUInt40(uint64_t i) { return putint(i, 5, PutUInt40BE, PutUInt40LE); }

        //!
        //! Write a 48-bit unsigned integer value and advance the write pointer.
        //! @param [in] i 48-bit unsigned integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUInt48(uint64_t i) { return putint(i, 6, PutUInt48BE, PutUInt48LE); }

        //!
        //! Write a 64-bit unsigned integer value and advance the write pointer.
        //! @param [in] i 64-bit unsigned integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUInt64(uint64_t i) { return putint(i, 8, PutUInt64BE, PutUInt64LE); }

        //!
        //! Write an 8-bit signed integer value and advance the write pointer.
        //! @param [in] i 8-bit signed integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putInt8(int8_t i) { return putint(i, 1, PutInt8, PutInt8); }

        //!
        //! Write a 16-bit signed integer value and advance the write pointer.
        //! @param [in] i 16-bit signed integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putInt16(int16_t i) { return putint(i, 2, PutInt16BE, PutInt16LE); }

        //!
        //! Write a 24-bit signed integer value and advance the write pointer.
        //! @param [in] i 24-bit signed integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putInt24(int32_t i) { return putint(i, 3, PutInt24BE, PutInt24LE); }

        //!
        //! Write a 32-bit signed integer value and advance the write pointer.
        //! @param [in] i 32-bit signed integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putInt32(int32_t i) { return putint(i, 4, PutInt32BE, PutInt32LE); }

        //!
        //! Write a 40-bit signed integer value and advance the write pointer.
        //! @param [in] i 40-bit signed integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putInt40(int64_t i) { return putint(i, 5, PutInt40BE, PutInt40LE); }

        //!
        //! Write a 48-bit signed integer value and advance the write pointer.
        //! @param [in] i 48-bit signed integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putInt48(int64_t i) { return putint(i, 6, PutInt48BE, PutInt48LE); }

        //!
        //! Write a 64-bit signed integer value and advance the write pointer.
        //! @param [in] i 64-bit signed integer value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putInt64(int64_t i) { return putint(i, 8, PutInt64BE, PutInt64LE); }

    protected:
        //!
        //! Set the read error state (reserved to subclasses).
        //!
        void setReadError() { _read_error = true; }

        //!
        //! Set the write error state (reserved to subclasses).
        //!
        void setWriteError() { _write_error = true; }

        //!
        //! Get starting address of current write area (ignoring bit offset inside first byte to read).
        //! This operation is reserved to subclasses. Applications can only get a read-only pointer to
        //! the current read area (see currentReadAddress()).
        //! @return The starting address of current write area.
        //!
        uint8_t* currentWriteAddress() const { return _buffer + _state.wbyte; }

    private:
        // Internal "read bytes" method (1 to 8 bytes).
        // - Check that the specified number of bytes is available for reading.
        // - If not available, set read error and return the address of an 8-byte area containing FF.
        // - Otherwise, if current read pointer is at a byte boundary, return the read address in the buffer.
        // - If not byte aligned, read the bytes content into an internal 8-byte buffer, byte aligned, and return its address.
        // - Advance read pointer.
        const uint8_t* rdb(size_t bytes);

        // Internal put integer method.
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool putint(INT value, size_t bytes, void (*putBE)(void*,INT), void (*putLE)(void*,INT));

        // Request some read size. Return actually possible read size. Set read error if lower than requested.
        size_t requestReadBytes(size_t bytes);

        // Get bulk bytes, either aligned or not. Update read pointer.
        void readBytesInternal(uint8_t* data, size_t bytes);

        // Set range of bits [start_bit..end_bit[ in a byte.
        void setBits(size_t byte, size_t start_bit, size_t end_bit, uint8_t value);

        // Read/write state in the buffer.
        struct RWState {
            RWState();     // Constructor.
            size_t rbyte;  // Next byte to read, offset from beginning of buffer.
            size_t wbyte;  // Next byte to write, offset from beginning of buffer.
            size_t rbit;   // Next bit to read at offset rbyte (0 = MSB in big endian, LSB in little endian).
            size_t wbit;   // Next bit to write at offset wbyte (0 = MSB in big endian, LSB in little endian).
        };

        uint8_t*             _buffer;        // Base address of memory buffer.
        size_t               _buffer_size;   // Size of addressable area in _buffer.
        size_t               _buffer_max;    // Size of usable area in _buffer.
        bool                 _read_only;     // The buffer is in read-only mode.
        bool                 _allocated;     // If true, _buffer was internally allocated and must be freed later.
        bool                 _big_endian;    // Read/write integers in big endian mode (false means little endian).
        bool                 _read_error;    // Read error encountered (passed end of stream for instance).
        bool                 _write_error;   // Write error encountered (passed end of stream for instance).
        bool                 _user_error;    // User-generated error.
        RWState              _state;         // Read/write indexes.
        std::vector<size_t>  _saved_max;     // Stack of saved _buffer_max.
        std::vector<RWState> _saved_states;  // Stack of saved states.
        uint8_t              _realigned[8];  // 64-bit intermediate buffer to read realigned integer.
    };
}

#include "tsBufferTemplate.h"
