//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCBIOPMessage.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// BIOPMessageHeader - Check if the header is valid
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::isValid() const
{
    if (magic != BIOP_MAGIC) {
        return false;
    }

    if (version_major != BIOP_VERSION_MAJOR) {
        return false;
    }

    if (byte_order != BIOP_BYTE_ORDER_BIG_ENDIAN) {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Clear the content
//----------------------------------------------------------------------------

void ts::BIOPMessageHeader::clear()
{
    magic = BIOP_MAGIC;
    version_major = BIOP_VERSION_MAJOR;
    version_minor = BIOP_VERSION_MINOR;
    byte_order = BIOP_BYTE_ORDER_BIG_ENDIAN;
    message_type = BIOP_MESSAGE_TYPE_STANDARD;
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Serialize
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::serialize(PSIBuffer& buf) const
{
    if (buf.remainingWriteBytes() < HEADER_SIZE) {
        buf.setUserError();
        return false;
    }

    // Write header fields
    buf.putUInt32(magic);
    buf.putUInt8(version_major);
    buf.putUInt8(version_minor);
    buf.putUInt8(byte_order);
    buf.putUInt8(message_type);

    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Deserialize
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::deserialize(PSIBuffer& buf)
{
    if (buf.remainingReadBytes() < HEADER_SIZE) {
        buf.setUserError();
        return false;
    }

    if (!buf.isBigEndian()) {
        buf.setBigEndian();
    }

    magic = buf.getUInt32();
    version_major = buf.getUInt8();
    version_minor = buf.getUInt8();
    byte_order = buf.getUInt8();
    message_type = buf.getUInt8();

    if (!isValid()) {
        buf.setUserError();
        return false;
    }

    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Display
//----------------------------------------------------------------------------

void ts::BIOPMessageHeader::display(TablesDisplay& disp, const UString& margin) const
{
    disp << margin << "BIOP Message Header:" << std::endl;
    disp << margin << UString::Format(u"  Magic: %n", magic);

    if (magic == BIOP_MAGIC) {
        disp << " (ASCII: \"BIOP\")";
    }
    else {
        disp << " (Invalid magic number)";
    }
    disp << std::endl;

    disp << margin << UString::Format(u"  BIOP version: %d.%d", version_major, version_minor) << std::endl;

    disp << margin << "  Byte order: ";
    if (byte_order == BIOP_BYTE_ORDER_BIG_ENDIAN) {
        disp << "Big-endian";
    }
    else {
        disp << UString::Format(u"Invalid (%n)", byte_order);
    }
    disp << std::endl;

    disp << margin << UString::Format(u"  Message type: %n", message_type) << std::endl;
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Static Display
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (buf.remainingReadBytes() < HEADER_SIZE) {
        disp.displayExtraData(buf, margin);
        return false;
    }

    BIOPMessageHeader header;
    if (header.deserialize(buf)) {
        header.display(disp, margin);
        return true;
    }
    else {
        disp << margin << "Invalid BIOP message header" << std::endl;
        disp.displayExtraData(buf, margin);
        return false;
    }
}
