//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsFloatUtils.h"
#include "tsUString.h"
#include "tsIntegerUtils.h"

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
        bool readOnly() const { return _state.read_only; }

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
        size_t size() const { return _state.end; }

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
        //! Specify that read/write operations of integers should use the native endian representation.
        //! The endianness of the buffer is not changed by the various reset() operations.
        //!
        void setNativeEndian() { _big_endian = TS_BIG_ENDIAN_BOOL; }

        //!
        //! Switch the endianness of read/write operations of integers.
        //! The endianness of the buffer is not changed by the various reset() operations.
        //!
        void switchEndian() { _big_endian = !_big_endian; }

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
        //! Check if read/write operations of integers use the native endian representation.
        //! @return True if the native endian is used, false otherwise.
        //!
        bool isNativeEndian() const { return _big_endian == TS_BIG_ENDIAN_BOOL; }

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
        //! Skip read reserved bits forward.
        //! @param [in] bits Number of reserved bits to skip.
        //! @param [in] expected Expected value of each reserved bit. Must be 0 or 1, default is 1.
        //! For each reserved bit which does not have the expected value, a "reserved bit error" is logged.
        //! @return True on success, false if would got beyond write pointer (and set read error flag).
        //! @see reservedBitsError()
        //!
        bool skipReservedBits(size_t bits, int expected = 1);

        //!
        //! Check if there were "reserved bits errors".
        //! @return True if there were "reserved bits errors".
        //! @see skipReservedBits()
        //!
        bool reservedBitsError() const { return !_reserved_bits_errors.empty(); }

        //!
        //! Return a string describing the "reserved bits errors".
        //! @param [in] base_offset Artificial base offset in bytes which is used to describe the placement of each bit error.
        //! When the Buffer instance describes the payload of a structure, specify the structure header size as base_offset
        //! in order to display each bit error relatively to the complete structure.
        //! @param [in] margin Prepend that string to each line.
        //! @return A string describing the "reserved bits errors". This is a mul
        //! @see skipReservedBits()
        //!
        UString reservedBitsErrorString(size_t base_offset = 0, const UString& margin = UString())
        {
            return ReservedBitsErrorString(_reserved_bits_errors, base_offset, margin);
        }

        //!
        //! This static method returns a string describing "reserved bits errors".
        //! @param [in,out] errors A vector of error description. Each value is made of
        //! byte offset || bit offset (3 bits) || expected bit value (1 bit).
        //! The vector is sorted first.
        //! @param [in] base_offset Artificial base offset in bytes which is used to describe the placement of each bit error.
        //! When the Buffer instance describes the payload of a structure, specify the structure header size as base_offset
        //! in order to display each bit error relatively to the complete structure.
        //! @param [in] margin Prepend that string to each line.
        //! @return A string describing the "reserved bits errors". This is a mul
        //! @see skipReservedBits()
        //!
        static UString ReservedBitsErrorString(std::vector<size_t>& errors, size_t base_offset = 0, const UString& margin = UString());

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
        bool endOfWrite() const { return _state.wbyte >= _state.end; }

        //!
        //! Check if we can still read from the buffer.
        //! @return True if we can still read from the buffer.
        //!
        bool canRead() const { return !error() && !endOfRead(); }

        //!
        //! Check if we can read at least the specified number of bytes from the buffer.
        //! @param [in] bytes Number of bytes.
        //! @return True if we can read at least @a bytes.
        //!
        bool canReadBytes(size_t bytes) const { return !error() && remainingReadBytes() >= bytes; }

        //!
        //! Check if we can read at least the specified number of bits from the buffer.
        //! @param [in] bits Number of bits.
        //! @return True if we can read at least @a bits.
        //!
        bool canReadBits(size_t bits) const { return !error() && remainingReadBits() >= bits; }

        //!
        //! Check if we can still write in the buffer.
        //! @return True if we can still write in the buffer.
        //!
        bool canWrite() const { return !error() && !endOfWrite(); }

        //!
        //! Check if we can write at least the specified number of bytes in the buffer.
        //! @param [in] bytes Number of bytes.
        //! @return True if we can write at least @a bytes.
        //!
        bool canWriteBytes(size_t bytes) const { return !error() && remainingWriteBytes() >= bytes; }

        //!
        //! Check if we can write at least the specified number of bits in the buffer.
        //! @param [in] bits Number of bits.
        //! @return True if we can write at least @a bits.
        //!
        bool canWriteBits(size_t bits) const { return !error() && remainingWriteBits() >= bits; }

        //!
        //! Push the current state of the read/write streams on a stack of saved states.
        //!
        //! There is an internal stack of read/write states. It is possible to save the current
        //! state of the buffer, try to do some operations and then either restore (pop) the
        //! previous state if the attempted operations failed or drop the saved state and
        //! continue with the new state.
        //!
        //! @return The level of pushed state (0 for the first push, then 1, etc.)
        //! The returned level can be used by popState() and dropState().
        //! @see popState()
        //!
        size_t pushState();

        //!
        //! Temporary reduce the readable size of the buffer.
        //!
        //! The previous state is pushed to the internal stack of state and can be restored later.
        //! Saving the readable size temporarily changes the write pointer and sets the buffer as read only.
        //! When the state is restored using popState(), the previous readable size (write pointer)
        //! and read-only indicator are restored. The read pointer is set to the end of previous
        //! readable size.
        //!
        //! @param [in] size New readable size in bytes of the buffer. In some cases, the final granted
        //! size can be different. The final value is bounded by the current read and write pointers.
        //! @return The level of pushed state (0 for the first push, then 1, etc.) Return NPOS on error.
        //! @see popState()
        //!
        size_t pushReadSize(size_t size);

        //!
        //! Temporary reduce the readable size of the buffer using a length field from the stream.
        //!
        //! An integer value is read from the stream (given the value size in bits). The read pointer
        //! must then be byte-aligned. Finally, pushReadSize() is called so that the remaining number
        //! of bytes to read is the length value that was just read.
        //!
        //! @param [in] length_bits Size in bits of the length field to read.
        //! @return The level of pushed state (0 for the first push, then 1, etc.) Return NPOS on error.
        //! @see pushReadSize()
        //!
        size_t pushReadSizeFromLength(size_t length_bits);

        //!
        //! Temporary reduce the writable size of the buffer.
        //!
        //! The previous state is pushed to the internal stack of state and can be restored later.
        //! Saving the writable size temporarily changes the end of buffer.
        //! When the state is restored using popState(), the previous end of buffer is restored.
        //! The read and write pointers are not restored (everything that was read or written in
        //! the meantime remain valid).
        //!
        //! @param [in] size New writable size in bytes of the buffer. In some cases, the final granted
        //! size can be different. The final value is bounded by the current write pointer and end of buffer.
        //! @return The level of pushed state (0 for the first push, then 1, etc.) Return NPOS on error.
        //! @see popState()
        //!
        size_t pushWriteSize(size_t size);

        //!
        //! Start a write sequence with a leading length field.
        //!
        //! The current state is pushed and the specified number of bits are skipped in the write field.
        //! The write stream must then be byte-aligned or an error is generated.
        //! Writing data can be continued by the application. When popState() is called, the size in
        //! bytes starting after the length field is then written in the length field.
        //!
        //! @param [in] length_bits Size in bits of the length field to write. Must be in the range 1 to 64.
        //! @return The level of pushed state (0 for the first push, then 1, etc.) Return NPOS on error.
        //! @see popState()
        //!
        size_t pushWriteSequenceWithLeadingLength(size_t length_bits);

        //!
        //! Swap the current state of the read/write streams with the one on top of the stack of saved states.
        //!
        //! The previous state must have been fully saved using pushState() only, not any other
        //! push method such as pushReadSize() or pushWriteSize(). Otherwise, the buffer is set
        //! in read and write error state.
        //!
        //! As a result, the previously saved state is restored and the current state (just before
        //! restoring the saved state) is pushed. If there was no saved state, the current state
        //! is unchanged but still saved. So it is always safe to assume that the current state
        //! was saved.
        //!
        //! @return The level of pushed state (0 for the first push, then 1, etc.) Return NPOS on error.
        //!
        size_t swapState();

        //!
        //! Pop the current state of the read/write streams from the stack of saved states and perform appropriate actions.
        //! The new state depends on which method was used to push the previous state.
        //! @param [in] level Saved level to restore. The default is NPOS, meaning the last
        //! saved state. Another inner level can be specified, in which case all outer levels
        //! are also popped.
        //! @return True on success. False if there is no saved state or @a level does not exist.
        //! @see pushState()
        //! @see pushReadSize()
        //! @see pushWriteSize()
        //!
        bool popState(size_t level = NPOS);

        //!
        //! Drop the last saved state of the read/write streams from the stack of saved states.
        //! @param [in] level Saved level to drop. The default is NPOS, meaning the last
        //! saved state. Another inner level can be specified, in which case the specified level
        //! and all outer levels are dropped.
        //! @return True on success. False if there is no saved state or @a level does not exist.
        //! @see pushReadWriteState()
        //!
        bool dropState(size_t level = NPOS);

        //!
        //! Get the current number of pushed states of the read/write streams.
        //! @return The current number of pushed states of the read/write streams.
        //!
        size_t pushedLevels() const { return _saved_states.size(); }

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
        //! Read the next bit and advance the read pointer.
        //! @return The value of the next bit.
        //!
        uint8_t getBit();

        //!
        //! Read the next bit as a boolean and advance the read pointer.
        //! @return The value of the next bit.
        //!
        bool getBool() { return getBit() != 0; }

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
        //! @return The value of the next @a bits.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
        INT getBits(size_t bits); // unsigned version

        //! @cond nodoxygen
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
        INT getBits(size_t bits); // signed version
        //! @endcond

        //!
        //! Read the next n bits as an integer value and advance the read pointer.
        //! @tparam INT An integer type for the result.
        //! @param [out] value The value of the next @a bits.
        //! @param [in] bits Number of bits to read.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void getBits(INT& value, size_t bits) { value = getBits<INT>(bits); }

        //!
        //! Read the next n bits as an integer value and advance the read pointer.
        //! @tparam INT An integer type for the result.
        //! @param [out] value The value of the next @a bits as a std::optional instance. If no integer can be read, the std::optional is unset.
        //! @param [in] bits Number of bits to read.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void getBits(std::optional<INT>& value, size_t bits);

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
        //! Serialize the number of reserved '1' bits
        //! @param [in] bits Number of reserved '1' bits to write.
        //! @return True on success, false on error (read only or no more space to write).
        //!
        bool putReserved(size_t bits);

        //!
        //! Serialize the number of reserved '0' bits
        //! @param [in] bits Number of reserved '0' bits to write.
        //! @return True on success, false on error (read only or no more space to write).
        //!
        bool putReservedZero(size_t bits) { return putBits<int>(0, bits); }

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
        //! @param [in] bytes Number of bytes to read. If specified as NPOS, return all remaining bytes.
        //! @return Read data as a byte block. If the requested number of bytes is not
        //! available, return as much as possible and set the read error.
        //!
        ByteBlock getBytes(size_t bytes = NPOS);

        //!
        //! Get bulk bytes from the buffer.
        //! The bit aligment is ignored, reading starts at the current read byte pointer,
        //! even if a few bits were already read from that byte.
        //! @param [out] bb Byte block receiving the read bytes.
        //! @param [in] bytes Number of bytes to read. If specified as NPOS, return all remaining bytes.
        //!
        void getBytes(ByteBlock& bb, size_t bytes = NPOS);

        //!
        //! Get bulk bytes from the buffer.
        //! The bit aligment is ignored, reading starts at the current read byte pointer,
        //! even if a few bits were already read from that byte.
        //! @param [in,out] bb Byte block receiving the read bytes. The read data are appended to @a bb.
        //! @param [in] bytes Number of bytes to read. If specified as NPOS, return all remaining bytes.
        //! @return Actual number of appended bytes. If the requested number of bytes is not
        //! available, return as much as possible and set the read error.
        //!
        size_t getBytesAppend(ByteBlock& bb, size_t bytes = NPOS);

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
        //! Read the next 32 bits as an IEEE float value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded IEEE float value, according to the current endianness of the buffer.
        //!
        ieee_float32_t getFloat32() { return _big_endian ? GetFloat32BE(rdb(4)) : GetFloat32LE(rdb(4)); }

        //!
        //! Read the next 64 bits as an IEEE float value and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read.
        //! @return The decoded IEEE float value, according to the current endianness of the buffer.
        //!
        ieee_float64_t getFloat64() { return _big_endian ? GetFloat64BE(rdb(8)) : GetFloat64LE(rdb(8)); }

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

        //!
        //! Write a 32-bit IEEE float value and advance the write pointer.
        //! @param [in] f 32-bit IEEE float value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putFloat32(ieee_float32_t f) { return putint(f, 4, PutFloat32BE, PutFloat32LE); }

        //!
        //! Write a 64-bit IEEE float value and advance the write pointer.
        //! @param [in] f 32-bit IEEE float value to write.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putFloat64(ieee_float64_t f) { return putint(f, 8, PutFloat64BE, PutFloat64LE); }

        //!
        //! Read the next 4*n bits as a Binary Coded Decimal (BCD) value and advance the read pointer.
        //! If an invalid BCD digit is found, the read error state of the buffer is set after reading all BCD digits.
        //! @tparam INT An integer type.
        //! @param [in] bcd_count Number of BCD digits (@a bcd_count * 4 bits).
        //! @return The decoded BCD value or zero in case of error.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        INT getBCD(size_t bcd_count);

        //!
        //! Read the next 4*n bits as a Binary Coded Decimal (BCD) value and advance the read pointer.
        //! If an invalid BCD digit is found, the read error state of the buffer is set after reading all BCD digits.
        //! @tparam INT An integer type.
        //! @param [out] value The decoded BCD value or zero in case of error.
        //! @param [in] bcd_count Number of BCD digits (@a bcd_count * 4 bits).
        //! @return True on success, false on error.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool getBCD(INT& value, size_t bcd_count);

        //!
        //! Put the next 4*n bits as a Binary Coded Decimal (BCD) value and advance the write pointer.
        //! @tparam INT An integer type.
        //! @param [in] value Integer value to write.
        //! @param [in] bcd_count Number of BCD digits (@a bcd_count * 4 bits).
        //! @return True on success, false on error (read only or no more space to write).
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool putBCD(INT value, size_t bcd_count);

        //!
        //! Try to get an ASCII string.
        //! The read-pointer must be byte-aligned.
        //! If all bytes are valid ASCII characters (optionally zero-padded), the corresponding string is
        //! returned and the read pointer is moved. If the corresponding area is not a valid ASCII string
        //! (optionally zero-padded), do not move the read pointer and return an empty string.
        //! @param [out] result Returned ASCII string. If the binary area is zero-padded, the trailing
        //! zeroes are not included in the string. This means that @a result can be shorter than @a bytes.
        //! @param [in] bytes Size in bytes of the string. If specified as @a NPOS (the default), read up to
        //! the end of the buffer. If different from @a NPOS, the exact number of bytes must be available or a read
        //! error is generated.
        //! @return True on success, false on error. Also return false when the binary area is not a valid
        //! ASCII string (optionally zero-padded), but do not generate a read error (it was just a try).
        //!
        bool tryGetASCII(UString& result, size_t bytes = NPOS);

        //!
        //! Try to get an ASCII string.
        //! The read-pointer must be byte-aligned.
        //! If all bytes are valid ASCII characters (optionally zero-padded), the corresponding string is
        //! returned and the read pointer is moved. If the corresponding area is not a valid ASCII string
        //! (optionally zero-padded), do not move the read pointer and return an empty string.
        //! @param [in] bytes Size in bytes of the string. If specified as @a NPOS (the default), read up to
        //! the end of the buffer. If different from @a NPOS, the exact number of bytes must be available or a read
        //! error is generated.
        //! @return The ASCII string. If the binary area is zero-padded, the trailing
        //! zeroes are not included in the string. This means that @a result can be shorter than @a bytes.
        //! Return an empty string when the entire binary area is not a valid ASCII string (optionally zero-padded),
        //! but do not generate a read error (it was just a try).
        //!
        UString tryGetASCII(size_t bytes = NPOS);

        //!
        //! Get a UTF-8 string.
        //! The read-pointer must be byte-aligned.
        //! @param [out] result Returned decoded string.
        //! @param [in] bytes Size in bytes of the encoded UTF-8 string. If specified as @a NPOS (the default), read up to
        //! the end of the buffer. If different from @a NPOS, the exact number of bytes must be available or a read
        //! error is generated.
        //! @return True on success, false on error.
        //!
        bool getUTF8(UString& result, size_t bytes = NPOS)
        {
            return getUTFInternal(result, bytes, true); // true = UTF-8
        }

        //!
        //! Get a UTF-8 string.
        //! The read-pointer must be byte-aligned.
        //! @param [in] bytes Size in bytes of the encoded UTF-8 string. If specified as @a NPOS (the default), read up to
        //! the end of the buffer. If different from @a NPOS, the exact number of bytes must be available or a read
        //! error is generated.
        //! @return The decoded string.
        //!
        UString getUTF8(size_t bytes = NPOS)
        {
            return outStringToResult(bytes, &Buffer::getUTF8);
        }

        //!
        //! Get a UTF-8 string (preceded by its length).
        //! The read-pointer must be byte-aligned after reading the length-field.
        //! The specified number of bytes must be available or a read error is generated.
        //! @param [out] result Returned decoded string.
        //! @param [in] length_bits Size in bits in the length field.
        //! @return True on success, false on error (truncated, unsupported format, etc.)
        //!
        bool getUTF8WithLength(UString& result, size_t length_bits = 8)
        {
            return getUTFWithLengthInternal(result, length_bits, true); // true = UTF-8
        }

        //!
        //! Get a UTF-8 string (preceded by its length).
        //! The read-pointer must be byte-aligned after reading the length-field.
        //! The specified number of bytes must be available or a read error is generated.
        //! @param [in] length_bits Size in bits in the length field.
        //! @return The decoded string.
        //!
        UString getUTF8WithLength(size_t length_bits = 8)
        {
            return outStringToResult(length_bits, &Buffer::getUTF8WithLength);
        }

        //!
        //! Get a UTF-16 string.
        //! The read-pointer must be byte-aligned.
        //! @param [out] result Returned decoded string.
        //! @param [in] bytes Size in bytes of the encoded UTF-16 string. If specified as @a NPOS (the default), read up to
        //! the end of the buffer. If different from @a NPOS, the exact number of bytes must be available or a read
        //! error is generated. If @a bytes is an odd value, the last byte is skipped and ignored.
        //! @return True on success, false on error.
        //!
        bool getUTF16(UString& result, size_t bytes = NPOS)
        {
            return getUTFInternal(result, bytes, false); // false = UTF-16
        }

        //!
        //! Get a UTF-16 string.
        //! The read-pointer must be byte-aligned.
        //! @param [in] bytes Size in bytes of the encoded UTF-16 string. If specified as @a NPOS (the default), read up to
        //! the end of the buffer. If different from @a NPOS, the exact number of bytes must be available or a read
        //! error is generated. If @a bytes is an odd value, the last byte is skipped and ignored.
        //! @return The decoded string.
        //!
        UString getUTF16(size_t bytes = NPOS)
        {
            return outStringToResult(bytes, &Buffer::getUTF16);
        }

        //!
        //! Get a UTF-16 string (preceded by its length).
        //! The read-pointer must be byte-aligned after reading the length-field.
        //! The specified number of bytes must be available or a read error is generated.
        //! If the extracted length is an odd value, the last byte is skipped and ignored.
        //! @param [out] result Returned decoded string.
        //! @param [in] length_bits Size in bits in the length field.
        //! @return True on success, false on error (truncated, unsupported format, etc.)
        //!
        bool getUTF16WithLength(UString& result, size_t length_bits = 8)
        {
            return getUTFWithLengthInternal(result, length_bits, false); // false = UTF-16
        }

        //!
        //! Get a UTF-16 string (preceded by its length).
        //! The read-pointer must be byte-aligned after reading the length-field.
        //! The specified number of bytes must be available or a read error is generated.
        //! If the extracted length is an odd value, the last byte is skipped and ignored.
        //! @param [in] length_bits Size in bits in the length field.
        //! @return The decoded string.
        //!
        UString getUTF16WithLength(size_t length_bits = 8)
        {
            return outStringToResult(length_bits, &Buffer::getUTF16WithLength);
        }

        //!
        //! Put a string using UTF-8 format.
        //! The write-pointer must be byte-aligned.
        //! Generate a write error when the buffer is full before writing the complete string.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUTF8(const UString& str, size_t start = 0, size_t count = NPOS)
        {
            return putUTFInternal(str, start, count, false, NPOS, 0, true) != 0; // true = UTF-8
        }

        //!
        //! Put a string using UTF-8 format with a fixed binary size (truncate or pad).
        //! The write-pointer must be byte-aligned.
        //! Generate a write error when the buffer is full before writing the complete string.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] size Fixed size in bytes to fill. If @a str cannot be fully serialized, it is truncated.
        //! @param [in] pad It @a str does not fill @a size bytes, pad the remaining bytes with this value.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putFixedUTF8(const UString& str, size_t size, uint8_t pad = 0, size_t start = 0, size_t count = NPOS)
        {
            return putUTFInternal(str, start, count, false, size, pad, true) != 0; // true = UTF-8
        }

        //!
        //! Put a partial string using UTF-8 format.
        //! The write-pointer must be byte-aligned.
        //! Stop either when this string is serialized or when the buffer is full, whichever comes first.
        //! Do not generate a write error when the buffer is full.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_t putPartialUTF8(const UString& str, size_t start = 0, size_t count = NPOS)
        {
            return putUTFInternal(str, start, count, true, NPOS, 0, true); // true = UTF-8
        }

        //!
        //! Put a string (preceded by its length) using UTF-8 format.
        //! The write-pointer must be byte-aligned after writing the length-field.
        //! Generate a write error when the buffer is full before writing the complete string.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @param [in] length_bits Size in bits in the length field.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUTF8WithLength(const UString& str, size_t start = 0, size_t count = NPOS, size_t length_bits = 8)
        {
            return putUTFWithLengthInternal(str, start, count, length_bits, false, true) != 0;
        }

        //!
        //! Put a partial string (preceded by its length) using UTF-8 format.
        //! The write-pointer must be byte-aligned after writing the length-field.
        //! Do not generate a write error when the buffer is full.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @param [in] length_bits Size in bits in the length field.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_t putPartialUTF8WithLength(const UString& str, size_t start = 0, size_t count = NPOS, size_t length_bits = 8)
        {
            return putUTFWithLengthInternal(str, start, count, length_bits, true, true);
        }

        //!
        //! Put a string using UTF-16 format.
        //! The write-pointer must be byte-aligned.
        //! Generate a write error when the buffer is full before writing the complete string.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUTF16(const UString& str, size_t start = 0, size_t count = NPOS)
        {
            return putUTFInternal(str, start, count, false, NPOS, 0, false) != 0; // false = UTF-16
        }

        //!
        //! Put a string using UTF-16 format with a fixed binary size (truncate or pad).
        //! The write-pointer must be byte-aligned.
        //! Generate a write error when the buffer is full before writing the complete string.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] size Fixed size in bytes to fill. If @a str cannot be fully serialized, it is truncated.
        //! @param [in] pad It @a str does not fill @a size bytes, serialize the remaining UTF-16 characters with this value.
        //! If @a size has an odd value, the last byte is padded with the least significant byte of @a pad.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putFixedUTF16(const UString& str, size_t size, uint16_t pad = 0, size_t start = 0, size_t count = NPOS)
        {
            return putUTFInternal(str, start, count, false, size, pad, false) != 0; // false = UTF-16
        }

        //!
        //! Put a partial string using UTF-8 format.
        //! The write-pointer must be byte-aligned.
        //! Stop either when this string is serialized or when the buffer is full, whichever comes first.
        //! Do not generate a write error when the buffer is full.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_t putPartialUTF16(const UString& str, size_t start = 0, size_t count = NPOS)
        {
            return putUTFInternal(str, start, count, true, NPOS, 0, false); // false = UTF-16
        }

        //!
        //! Put a string (preceded by its length) using UTF-16 format.
        //! The write-pointer must be byte-aligned after writing the length-field.
        //! Generate a write error when the buffer is full before writing the complete string.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @param [in] length_bits Size in bits in the length field.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putUTF16WithLength(const UString& str, size_t start = 0, size_t count = NPOS, size_t length_bits = 8)
        {
            return putUTFWithLengthInternal(str, start, count, length_bits, false, false) != 0;
        }

        //!
        //! Put a partial string (preceded by its length) using UTF-16 format.
        //! The write-pointer must be byte-aligned after writing the length-field.
        //! Do not generate a write error when the buffer is full.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @param [in] length_bits Size in bits in the length field.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_t putPartialUTF16WithLength(const UString& str, size_t start = 0, size_t count = NPOS, size_t length_bits = 8)
        {
            return putUTFWithLengthInternal(str, start, count, length_bits, true, false);
        }

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
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_floating_point<INT>::value, int>::type = 0>
        bool putint(INT value, size_t bytes, void (*putBE)(void*,INT), void (*putLE)(void*,INT));

        // Request some read size. Return actually possible read size. Set read error if lower than requested.
        // If equal to NPOS, return maximum size.
        size_t requestReadBytes(size_t bytes);

        // Get bulk bytes, either aligned or not. Update read pointer.
        void readBytesInternal(uint8_t* data, size_t bytes);

        // Set range of bits [start_bit..end_bit[ in a byte.
        void setBits(size_t byte, size_t start_bit, size_t end_bit, uint8_t value);

        // Common code for UTF strings.
        UString outStringToResult(size_t param, bool (Buffer::*method)(UString&, size_t));
        bool getUTFInternal(UString& result, size_t bytes, bool utf8);
        bool getUTFWithLengthInternal(UString& result, size_t length_bits, bool utf8);
        size_t putUTFInternal(const UString& str, size_t start, size_t count, bool partial, size_t fixed_size, int pad, bool utf8);
        size_t putUTFWithLengthInternal(const UString& str, size_t start, size_t count, size_t length_bits, bool partial, bool utf8);

        // Reason for the creation of a buffer state.
        enum class Reason {
            FULL,           // Full state was saved.
            READ_SIZE,      // A new read size (write pointer) was specified.
            WRITE_SIZE,     // A new write size (end of buffer) was specified.
            WRITE_LEN_SEQ,  // A write sequence with a leading length field was started.
        };

        // Read/write state in the buffer.
        struct State {
            Reason reason {Reason::FULL}; // Reason for the creation of this state.
            bool   read_only;             // The buffer is in read-only mode.
            size_t end;                   // Size of usable area in buffer.
            size_t rbyte = 0;             // Next byte to read, offset from beginning of buffer.
            size_t wbyte = 0;             // Next byte to write, offset from beginning of buffer.
            size_t rbit = 0;              // Next bit to read at offset rbyte (0 = MSB in big endian, LSB in little endian).
            size_t wbit = 0;              // Next bit to write at offset wbyte (0 = MSB in big endian, LSB in little endian).
            size_t len_bits = 0;          // Size in bits of the length field (when reason is WRITE_LEN_SEQ).

            // Constructor.
            State(bool rdonly = true, size_t size = 0);

            // Reset all values to zero.
            void clear();
        };

        uint8_t*            _buffer;              // Base address of memory buffer.
        size_t              _buffer_size;         // Size of addressable area in _buffer.
        bool                _allocated;           // If true, _buffer was internally allocated and must be freed later.
        bool                _big_endian = true;   // Read/write integers in big endian mode (false means little endian).
        bool                _read_error = false;  // Read error encountered (passed end of stream for instance).
        bool                _write_error = false; // Write error encountered (passed end of stream for instance).
        bool                _user_error = false;  // User-generated error.
        State               _state;               // Read/write indexes.
        std::vector<State>  _saved_states {};     // Stack of saved states.
        uint8_t             _realigned[8] {};     // 64-bit intermediate buffer to read realigned integer.
        std::vector<size_t> _reserved_bits_errors {};  // Errors in reserved bits (byte offset || bit offset (3 bits) || expected bit (1 bit)).
    };

    //! @cond nodoxygen
    // Template specialization for boolean.
    template<> TSDUCKDLL inline bool Buffer::putBits(bool value, size_t bits) { return putBits<int>(value ? 1 : 0, bits); }
    //! @endcond
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Read the next n bits as an integer value and advance the read pointer.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::Buffer::getBits(size_t bits)
{
    typedef typename std::make_unsigned<INT>::type UNSINT;
    const INT value = static_cast<INT>(getBits<UNSINT>(bits));
    return SignExtend(value, bits);
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Buffer::getBits(std::optional<INT>& value, size_t bits)
{
    if (_read_error || currentReadBitOffset() + bits > currentWriteBitOffset()) {
        _read_error = true;
        value.reset();
    }
    else {
        value = getBits<INT>(bits);
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
INT ts::Buffer::getBits(size_t bits)
{
    // No read if read error is already set or not enough bits to read.
    if (_read_error || currentReadBitOffset() + bits > currentWriteBitOffset()) {
        _read_error = true;
        return 0;
    }

    INT val = 0;

    if (_big_endian) {
        // Read leading bits up to byte boundary
        while (bits > 0 && _state.rbit != 0) {
            val = INT(val << 1) | INT(getBit());
            --bits;
        }

        // Read complete bytes
        while (bits > 7) {
            val = INT(val << 8) | INT(_buffer[_state.rbyte++]);
            bits -= 8;
        }

        // Read trailing bits
        while (bits > 0) {
            val = INT(val << 1) | INT(getBit());
            --bits;
        }
    }
    else {
        // Little endian decoding
        int shift = 0;

        // Read leading bits up to byte boundary
        while (bits > 0 && _state.rbit != 0) {
            val |= INT(getBit()) << shift;
            --bits;
            shift++;
        }

        // Read complete bytes
        while (bits > 7) {
            val |= INT(_buffer[_state.rbyte++]) << shift;
            bits -= 8;
            shift += 8;
        }

        // Read trailing bits
        while (bits > 0) {
            val |= INT(getBit()) << shift;
            --bits;
            shift++;
        }
    }

    return val;
}

// Put the next n bits from an integer value and advance the write pointer.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::Buffer::putBits(INT value, size_t bits)
{
    // No write if write error is already set or read-only or not enough bits to write.
    if (_write_error || _state.read_only || remainingWriteBits() < bits) {
        _write_error = true;
        return false;
    }

    if (_big_endian) {
        // Write leading bits up to byte boundary
        while (bits > 0 && _state.wbit != 0) {
            putBit(uint8_t((value >> --bits) & 1));
        }

        // Write complete bytes.
        while (bits > 7) {
            bits -= 8;
            _buffer[_state.wbyte++] = uint8_t(value >> bits);
        }

        // Write trailing bits
        while (bits > 0) {
            putBit(uint8_t((value >> --bits) & 1));
        }
    }
    else {
        // Little endian encoding. Write leading bits up to byte boundary.
        while (bits > 0 && _state.wbit != 0) {
            putBit(uint8_t(value & 1));
            value >>= 1;
            --bits;
        }

        // Write complete bytes.
        while (bits > 7) {
            _buffer[_state.wbyte++] = uint8_t(value);
            TS_PUSH_WARNING()
            TS_LLVM_NOWARNING(shift-count-overflow) // "shift count >= width of type" for 8-bit ints
            value >>= 8;
            TS_POP_WARNING()
            bits -= 8;
        }

        // Write trailing bits
        while (bits > 0) {
            putBit(uint8_t(value & 1));
            value >>= 1;
            --bits;
        }
    }

    return true;
}

// Internal put integer method.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_floating_point<INT>::value, int>::type>
bool ts::Buffer::putint(INT value, size_t bytes, void (*putBE)(void*,INT), void (*putLE)(void*,INT))
{
    // Internally used to write up to 8 bytes (64-bit integers).
    assert(bytes <= 8);

    // No write if write error is already set or read-only.
    if (_write_error || _state.read_only) {
        _write_error = true;
        return false;
    }

    // Hypothetical new write pointer (bit pointer won't change).
    const size_t new_wbyte = _state.wbyte + bytes;

    if (new_wbyte > _state.end || (new_wbyte == _state.end && _state.wbit > 0)) {
        // Not enough bytes to write.
        _write_error = true;
        return false;
    }
    else if (_state.wbit == 0) {
        // Write pointer is byte aligned. Most common case.
        if (_big_endian) {
            putBE(_buffer + _state.wbyte, value);
        }
        else {
            putLE(_buffer + _state.wbyte, value);
        }
        _state.wbyte = new_wbyte;
        return true;
    }
    else {
        // Write pointer is not byte aligned. Use an intermediate buffer.
        uint8_t buf[8];
        if (_big_endian) {
            putBE(buf, value);
        }
        else {
            putLE(buf, value);
        }
        putBytes(buf, bytes);
        assert(_state.wbyte == new_wbyte);
        return true;
    }
}

// Read the next 4*n bits as a BCD value.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::Buffer::getBCD(size_t bcd_count)
{
    INT value = 0;
    getBCD(value, bcd_count);
    return value;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::Buffer::getBCD(INT& value, size_t bcd_count)
{
    typedef typename std::make_unsigned<INT>::type UNSINT;
    UNSINT uvalue = 0;

    if (_read_error || currentReadBitOffset() + 4 * bcd_count > currentWriteBitOffset()) {
        _read_error = true;
        value = 0;
        return false;
    }
    else {
        while (bcd_count-- > 0) {
            UNSINT nibble = getBits<UNSINT>(4);
            if (nibble > 9) {
                _read_error = true;
                nibble = 0;
            }
            uvalue = 10 * uvalue + nibble;
        }
        value = static_cast<INT>(uvalue);
        return true;
    }
}

// Put the next 4*n bits as a BCD value.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::Buffer::putBCD(INT value, size_t bcd_count)
{
    // No write if write error is already set or read-only or not enough bits to write.
    if (_write_error || _state.read_only || remainingWriteBits() < 4 * bcd_count) {
        _write_error = true;
        return false;
    }

    if (bcd_count > 0) {
        typedef typename std::make_unsigned<INT>::type UNSINT;
        UNSINT uvalue = static_cast<UNSINT>(value);
        UNSINT factor = static_cast<UNSINT>(Power10(bcd_count));
        while (bcd_count-- > 0) {
            uvalue %= factor;
            factor /= 10;
            putBits(uvalue / factor, 4);
        }
    }
    return true;
}
