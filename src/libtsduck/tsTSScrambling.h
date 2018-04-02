//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Transport stream scrambling using multiple algorithms.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsCerrReport.h"
#include "tsTSPacket.h"
#include "tsDVBCSA2.h"
#include "tsIDSA.h"

namespace ts {
    //!
    //! Transport stream scrambling using multiple algorithms.
    //! Include command line arguments processing.
    //!
    class TSDUCKDLL TSScrambling
    {
    public:
        //!
        //! Default constructor.
        //! @param [in,out] report Where to report error and information.
        //!
        TSScrambling(Report& report = CERR);

        //!
        //! Define command line options in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineOptions(Args& args) const;

        //!
        //! Add help about command line options in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void addHelp(Args& args) const;

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //!
        void loadArgs(Args& args);

        //!
        //! Check of fixed control words were loaded from the command line.
        //! @return True if this object uses fixed control words.
        //!
        bool hasFixedCW() const { return !_cw_list.empty(); }

        //!
        //! Get the scrambling algorithm name.
        //! @return The scrambling algorithm name.
        //!
        UString algoName() const { return _scrambler->name(); }

        //!
        //! Get the required control word size in bytes.
        //! @return The required control word size in bytes.
        //!
        size_t cwSize() const { return _scrambler->minKeySize(); }

        //!
        //! Set the control word for encrypt and decrypt.
        //! @param [in] cw The control word to use.
        //! @param [in] parity Use the parity of this integer value (odd or even).
        //! @return True on success, false on error.
        //!
        bool setCW(const ByteBlock& cw, int parity);

        //!
        //! Set the parity of all subsequent encryptions.
        //! @param [in] parity Use the parity of this integer value (odd or even).
        //!
        void setEncryptParity(int parity);

        //!
        //! Encrypt a TS packet with the current parity and corresponding CW.
        //! @param [in,out] the packet to encrypt.
        //! @return True on success, false on error. An already encrypted packet is an error.
        //!
        bool encrypt(TSPacket& pkt);

        //!
        //! Decrypt a TS packet with the CW corresponding to the parity in the packet.
        //! @param [in,out] the packet to decrypt.
        //! @return True on success, false on error. A clear packet is not an error.
        //!
        bool decrypt(TSPacket& pkt);

    private:
        // List of control words
        typedef std::list<ByteBlock> CWList;

        Report&          _report;
        DVBCSA2          _dvbcsa;
        IDSA             _idsa;
        CipherChaining*  _scrambler;
        CWList           _cw_list;  
        CWList::iterator _next_cw;
        int              _encrypt_parity;

        // Inaccessible operations.
        TSScrambling(const TSScrambling&) = delete;
        TSScrambling& operator=(const TSScrambling&) = delete;
    };
}
