//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#pragma once


//----------------------------------------------------------------------------
// Display a container of services.
//----------------------------------------------------------------------------

template <class ITERATOR>
std::ostream& ts::Service::Display(std::ostream& strm,
                                   const UString& margin,
                                   const ITERATOR& begin,
                                   const ITERATOR& end,
                                   bool header)
{
    // Some header
    const UString h_name(u"Name");
    const UString h_provider(u"Provider");

    // List fields which are present
    uint32_t fields = 0;
    size_t name_width = h_name.width();
    size_t provider_width = h_provider.width();
    size_t count = 0;
    for (ITERATOR it = begin; it != end; ++it) {
        count++;
        fields |= it->getFields();
        if (it->_name.set()) {
            name_width = std::max(name_width, it->_name.value().width());
            fields |= NAME;
        }
        if (it->_provider.set()) {
            provider_width = std::max(provider_width, it->_provider.value().width());
            fields |= PROVIDER;
        }
    }

    // Empty container: nothing to display
    if (count == 0) {
        return strm;
    }

    // Display header: LCN NAME PROVIDER ID TSID ONID TYPE PMT_PID
    if (header) {
        strm << margin;
        if (fields & LCN) {
            strm << "LCN ";
        }
        if (fields & NAME) {
            strm << h_name.toJustifiedLeft(name_width + 1);
        }
        if (fields & PROVIDER) {
            strm << h_provider.toJustifiedLeft(provider_width + 1);
        }
        if (fields & ID) {
            strm << "ServId ";
        }
        if (fields & TSID) {
            strm << "TSId   ";
        }
        if (fields & ONID) {
            strm << "ONetId ";
        }
        if (fields & (TYPE_DVB | TYPE_ATSC)) {
            strm << "Type ";
        }
        if (fields & PMT_PID) {
            strm << "PMTPID";
        }
        strm << std::endl << margin;
        if (fields & LCN) {
            strm << "--- ";
        }
        if (fields & NAME) {
            strm << UString().toJustifiedLeft(name_width, '-') << " ";
        }
        if (fields & PROVIDER) {
            strm << UString().toJustifiedLeft(provider_width, '-') << " ";
        }
        if (fields & ID) {
            strm << "------ ";
        }
        if (fields & TSID) {
            strm << "------ ";
        }
        if (fields & ONID) {
            strm << "------ ";
        }
        if (fields & (TYPE_DVB | TYPE_ATSC)) {
            strm << "---- ";
        }
        if (fields & PMT_PID) {
            strm << "------";
        }
        strm << std::endl;
    }

    // Display all elements
    for (ITERATOR it = begin; it != end; ++it) {
        strm << margin;
        if (fields & LCN) {
            if (it->_lcn.set()) {
                strm << UString::Format(u"%3d ", {it->_lcn.value()});
            }
            else {
                strm << "    ";
            }
        }
        if (fields & NAME) {
            strm << it->getName().toJustifiedLeft(name_width + 1);
        }
        if (fields & PROVIDER) {
            strm << it->getProvider().toJustifiedLeft(provider_width + 1);
        }
        if (fields & ID) {
            if (it->_id.set()) {
                strm << UString::Format(u"0x%04X ", {it->_id.value()});
            }
            else {
                strm << "       ";
            }
        }
        if (fields & TSID) {
            if (it->_tsid.set()) {
                strm << UString::Format(u"0x%04X ", {it->_tsid.value()});
            }
            else {
                strm << "       ";
            }
        }
        if (fields & ONID) {
            if (it->_onid.set()) {
                strm << UString::Format(u"0x%04X ", {it->_onid.value()});
            }
            else {
                strm << "       ";
            }
        }
        if (fields & (TYPE_DVB | TYPE_ATSC)) {
            if (it->_type_dvb.set()) {
                strm << UString::Format(u"0x%02X ", {it->_type_dvb.value()});
            }
            else if (it->_type_atsc.set()) {
                strm << UString::Format(u"0x%02X ", {it->_type_atsc.value()});
            }
            else {
                strm << "     ";
            }
        }
        if (fields & PMT_PID) {
            if (it->_pmt_pid.set()) {
                strm << UString::Format(u"0x%04X ", {it->_pmt_pid.value()});
            }
            else {
                strm << "      ";
            }
        }
        strm << std::endl;
    }

    return strm;
}
