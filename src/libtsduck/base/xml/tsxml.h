//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup xml
//!  Forward declaration of XML classes.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Namespace for XML classes.
    //!
    //! The XML features of TSDuck are freely inspired from TinyXML-2, a simple
    //! and lightweight XML library originally developed by Lee Thomason.
    //!
    //! TSDuck used to embed TinyXML-2 in the past but no longer does to allow
    //! more specialized operations. This set of classes is probably less fast
    //! than TinyXML-2 but TSDuck does not manipulate huge XML files. So, this
    //! should be OK.
    //!
    //! Among the differences between TinyXML-2 and this set of classes:
    //! - Uses Unicode strings from the beginning.
    //! - Error reporting using ts::Report.
    //! - Case-insensitive search of names and attributes.
    //! - Getting values and attributes with cardinality and value bounds checks.
    //! - Print / format any subset of a document.
    //! - XML document validation using a template.
    //!
    namespace xml {

        // Forward declaration of XML classes.
        class Attribute;
        class Comment;
        class Declaration;
        class Element;
        class Node;
        class Text;
        class Unknown;
        class Document;
        class ModelDocument;
        class PatchDocument;

        //!
        //! Vector of constant elements.
        //!
        typedef std::vector<const Element*> ElementVector;

        //!
        //! Specify an unlimited number of elements.
        //!
        static constexpr size_t UNLIMITED = std::numeric_limits<size_t>::max();

        //!
        //! How to process attributes when merging XML elements.
        //! In the merge process, there a main element and a secondary element which is merged into the main.
        //! These declarations describe what to do when merging two elements with same tag.
        //! @see Element::merge()
        //!
        enum class MergeAttributes {
            NONE,      //!< Ignore attributes in the secondary element.
            ADD,       //!< Add attributes from the secondary element which are not already present in the main element.
            REPLACE,   //!< Unconditionally copy attributes from the secondary element into the main.
        };
    }
}
