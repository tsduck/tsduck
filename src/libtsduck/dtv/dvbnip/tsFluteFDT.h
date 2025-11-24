//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of the File Delivery Table (FDT) in the FLUTE protocol.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsFluteFile.h"
#include "tsReport.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of the File Delivery Table (FDT) in the FLUTE protocol.
    //! @see IETF RFC 3926, section 3.4.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteFDT : public FluteFile
    {
        TS_RULE_OF_FIVE(FluteFDT, override);
    public:
        //!
        //! Default constructor.
        //!
        FluteFDT() = default;

        //!
        //! Constructor.
        //! @param [in] report Where to report errors.
        //! @param [in] sid Session id.
        //! @param [in] instance_id FDT Instance ID.
        //! @param [in] content File content (XML text in UTF-8 representation).
        //!
        FluteFDT(Report&               report,
                 const FluteSessionId& sid,
                 uint32_t              instance_id,
                 const ByteBlockPtr&   content);

        //!
        //! Description of one file of the FDT.
        //!
        class TSDUCKDLL File
        {
        public:
            File() = default;                       //!< Constructor.
            UString   content_location {};          //!< Content-Location attribute.
            uint64_t  toi = 0;                      //!< Transport object identifier.
            uint64_t  content_length = 0;           //!< The length of  the file in bytes.
            uint64_t  transfer_length = 0;          //!< The length of the transport object that carries the file in bytes.
            UString   content_type {};              //!< MIME type.
            UString   content_encoding {};          //!< Encoding type.
            ByteBlock content_md5 {};               //!< MD5 checksum of file content.
            uint32_t  fec_encoding_id = 0;          //!< FEC Encoding ID which was used to parse the structure (not part of the structore).
            uint32_t  fec_instance_id = 0;          //!< FEC Instance ID (FEC Encoding ID 128-255).
            uint32_t  max_source_block_length = 0;  //!< Max number of source symbols per source block (FEC Encoding ID 0, 128, 129, 130).
            uint32_t  encoding_symbol_length = 0;   //!< Length of Encoding Symbol in bytes (FEC Encoding ID 0, 128, 129, 130).
            uint32_t  max_encoding_symbols = 0;     //!< Max number of encoding symbols (FEC Encoding ID 129).
        };

        // The content of the FDT is in public fields:
        uint32_t  instance_id = 0;              //!< FDT instance id.
        Time      expires {};                   //!< FDT expiration date.
        bool      complete = false;             //!< FDT is complete, no new data in future FDT instances.
        UString   content_type {};              //!< MIME type.
        UString   content_encoding {};          //!< Encoding type.
        uint32_t  fec_encoding_id = 0;          //!< FEC Encoding ID which was used to parse the structure (not part of the structore).
        uint32_t  fec_instance_id = 0;          //!< FEC Instance ID (FEC Encoding ID 128-255).
        uint32_t  max_source_block_length = 0;  //!< Max number of source symbols per source block (FEC Encoding ID 0, 128, 129, 130).
        uint32_t  encoding_symbol_length = 0;   //!< Length of Encoding Symbol in bytes (FEC Encoding ID 0, 128, 129, 130).
        uint32_t  max_encoding_symbols = 0;     //!< Max number of encoding symbols (FEC Encoding ID 129).
        std::list<File> files {};               //!< List of files in this FDT.
    };
}
