//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  Forward declarations for MPEG PSI/SI types.
//!  Useful to avoid interdependencies of header files.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSafePtr.h"
#include "tsMPEG.h"

namespace ts {

    class AbstractDescriptor;
    class AbstractTable;
    class BinaryTable;
    class Descriptor;
    class DescriptorList;
    class Section;
    class TablesDisplay;

    //!
    //! Safe pointer for AbstractDescriptor (not thread-safe).
    //!
    typedef SafePtr<AbstractDescriptor,NullMutex> AbstractDescriptorPtr;

    //!
    //! Vector of AbstractDescriptor pointers
    //!
    typedef std::vector<AbstractDescriptorPtr> AbstractDescriptorPtrVector;

    //!
    //! Safe pointer for AbstractTable (not thread-safe)
    //!
    typedef SafePtr<AbstractTable,NullMutex> AbstractTablePtr;

    //!
    //! Safe pointer for Section (not thread-safe).
    //!
    typedef SafePtr<Section, NullMutex> SectionPtr;

    //!
    //! Vector of Section pointers.
    //!
    typedef std::vector<SectionPtr> SectionPtrVector;

    //!
    //! Vector of BinaryTable pointers
    //!
    typedef std::vector<AbstractTablePtr> AbstractTablePtrVector;

    //!
    //! Safe pointer for BinaryTable (not thread-safe)
    //!
    typedef SafePtr<BinaryTable,NullMutex> BinaryTablePtr;

    //!
    //! Vector of BinaryTable pointers
    //!
    typedef std::vector<BinaryTablePtr> BinaryTablePtrVector;

    //!
    //! Safe pointer for Descriptor (not thread-safe)
    //!
    typedef SafePtr<Descriptor, NullMutex> DescriptorPtr;

    //!
    //! Vector of Descriptor pointers
    //! Use class DescriptorList for advanced features.
    //! @see DescriptorList
    //!
    typedef std::vector<DescriptorPtr> DescriptorPtrVector;


    //!
    //! Profile of a function to display a section.
    //! Each subclass should provide a static function named @e DisplaySection
    //! which displays a section of its table-id.
    //!
    //! @param [in,out] display Display engine.
    //! @param [in] section The section to display.
    //! @param [in] indent Indentation width.
    //!
    typedef void (*DisplaySectionFunction)(TablesDisplay& display, const Section& section, int indent);

    //!
    //! Profile of a function to display a descriptor.
    //! Each subclass should provide a static function named @e DisplayDescriptor
    //! which displays a descriptor of its type.
    //!
    //! @param [in,out] display Display engine.
    //! @param [in] did Descriptor id.
    //! @param [in] payload Address of the descriptor payload.
    //! @param [in] size Size in bytes of the descriptor payload.
    //! @param [in] indent Indentation width.
    //! @param [in] tid Table id of table containing the descriptors.
    //! This is optional. Used by some descriptors the interpretation of which may
    //! vary depending on the table that they are in.
    //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
    //!
    typedef void (*DisplayDescriptorFunction)(TablesDisplay& display,
                                              DID did,
                                              const uint8_t* payload,
                                              size_t size,
                                              int indent,
                                              TID tid,
                                              PDS pds);

    //!
    //! @hideinitializer
    //! Define a DisplaySection static function.
    //!
#define DeclareDisplaySection()                            \
        /** A static method to display a section.       */ \
        /** @param [in,out] display Display engine.     */ \
        /** @param [in] section The section to display. */ \
        /** @param [in] indent Indentation width.       */ \
        static void DisplaySection(TablesDisplay& display, const Section& section, int indent)

    //!
    //! @hideinitializer
    //! Define a DisplayDescriptor static function.
    //!
#define DeclareDisplayDescriptor()                                           \
        /** Static method to display a descriptor.                        */ \
        /** @param [in,out] display Display engine.                       */ \
        /** @param [in] did Descriptor id.                                */ \
        /** @param [in] payload Address of the descriptor payload.        */ \
        /** @param [in] size Size in bytes of the descriptor payload.     */ \
        /** @param [in] indent Indentation width.                         */ \
        /** @param [in] tid Table id of table containing the descriptors. */ \
        /** @param [in] pds Private Data Specifier.                       */ \
        static void DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds)
}
