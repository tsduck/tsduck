//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  BIOP (Broadcast Inter-ORB Protocol) Message structures.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDSMCC.h"
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"
#include "tsByteBlock.h"
#include "tsDSMCCIOR.h"
#include "tsxmlElement.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace ts {
    //!
    //! BIOP Message Header.
    //! This is the common header for all BIOP messages in DSM-CC Object Carousel.
    //! @see ISO/IEC 13818-6, Section 8
    //! @see ETSI TR 101 202, Section 4.4
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL BIOPMessageHeader
    {
        TS_DEFAULT_COPY_MOVE(BIOPMessageHeader);

    public:
        //!
        //! BIOP magic number ("BIOP" in ASCII).
        //!
        static constexpr uint32_t BIOP_MAGIC = 0x42494F50;

        //!
        //! Standard BIOP version (major).
        //!
        static constexpr uint8_t BIOP_VERSION_MAJOR = 0x01;

        //!
        //! Standard BIOP version (minor).
        //!
        static constexpr uint8_t BIOP_VERSION_MINOR = 0x00;

        //!
        //! Big-endian byte order (always 0x00 per specification).
        //!
        static constexpr uint8_t BIOP_BYTE_ORDER_BIG_ENDIAN = 0x00;

        //!
        //! Standard message type (always 0x00 per specification).
        //!
        static constexpr uint8_t BIOP_MESSAGE_TYPE_STANDARD = 0x00;

        //!
        //! Fixed size of BIOP message header in bytes.
        //!
        static constexpr size_t HEADER_SIZE = 8;

        uint32_t magic = BIOP_MAGIC;                        //!< Magic number, must be 0x42494F50 ("BIOP").
        uint8_t version_major = BIOP_VERSION_MAJOR;         //!< BIOP version major, typically 0x01.
        uint8_t version_minor = BIOP_VERSION_MINOR;         //!< BIOP version minor, typically 0x00.
        uint8_t byte_order = BIOP_BYTE_ORDER_BIG_ENDIAN;    //!< Byte order: 0x00 = big-endian.
        uint8_t message_type = BIOP_MESSAGE_TYPE_STANDARD;  //!< Message type, typically 0x00 for standard message.

        //!
        //! Default constructor.
        //!
        BIOPMessageHeader() = default;

        //!
        //! Check if the header is valid.
        //! @return True if the header has valid magic number and supported version.
        //!
        bool isValid() const;

        //!
        //! Clear the content of the header.
        //!
        void clear();

        //!
        //! Serialize the BIOP message header.
        //! @param [in,out] buf Serialization buffer.
        //! @return True on success, false on error.
        //!
        bool serialize(PSIBuffer& buf) const;

        //!
        //! Deserialize the BIOP message header.
        //! @param [in,out] buf Deserialization buffer. Side effect: the buffer is
        //! switched to big-endian to match the BIOP byte_order field and is NOT
        //! restored afterwards — callers that share the buffer with little-endian
        //! readers must reset the byte order themselves.
        //! @return True on success, false on error.
        //!
        bool deserialize(PSIBuffer& buf);

        //!
        //! Display the BIOP message header.
        //! @param [in,out] display Display engine.
        //! @param [in] margin Left margin content.
        //!
        void display(TablesDisplay& display, const UString& margin) const;

        //!
        //! A static method to display a BIOP message header.
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the BIOP message header.
        //! @param [in] margin Left margin content.
        //! @return True on success, false on error.
        //!
        static bool Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin);

        //!
        //! Write the header fields as attributes on the given element.
        //! The header has no dedicated XML container; its fields live as attributes
        //! directly on the enclosing `<BIOP_message>` element.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] element The element to attach the header attributes to.
        //!
        void toXML(DuckContext& duck, xml::Element* element) const;

        //!
        //! Read the header fields from attributes of the given element.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The element whose attributes carry the header fields.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* element);
    };

    //!
    //! BIOP Object Kind constants.
    //!
    namespace BIOPObjectKind {
        constexpr char DIRECTORY[] = "dir";        //!< Directory object kind (4 bytes with null terminator).
        constexpr char FILE[] = "fil";             //!< File object kind (4 bytes with null terminator).
        constexpr char SERVICE_GATEWAY[] = "srg";  //!< Service Gateway object kind (4 bytes with null terminator).
        constexpr char STREAM[] = "str";           //!< Stream object kind (4 bytes with null terminator).
        constexpr char STREAM_EVENT[] = "ste";     //!< Stream Event object kind (4 bytes with null terminator).
        constexpr size_t SIZE = 4;                 //!< Fixed size of object kind field.
    }  // namespace BIOPObjectKind


    //!
    //! One entry of a BIOP ServiceContextList.
    //! @ingroup libtsduck mpeg
    //!
    struct TSDUCKDLL BIOPServiceContext {
        uint32_t  context_id = 0;  //!< Context identifier.
        ByteBlock context_data {}; //!< Context data.

        //!
        //! This method converts a BIOPServiceContext to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML element.
        //!
        void toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! This method decodes an XML BIOPServiceContext.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The XML element containing the service context.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* element);
    };

    //!
    //! One component of a BIOP Name (typically a path element).
    //! @ingroup libtsduck mpeg
    //!
    struct TSDUCKDLL BIOPNameComponent {
        ByteBlock id {};    //!< Raw id bytes (usually a NUL-terminated ASCII/UTF-8 string).
        ByteBlock kind {};  //!< Raw kind bytes (usually "fil\0", "dir\0", "srg\0" ...).

        //!
        //! Return the id as a UString, stripping any trailing NUL.
        //! @return The id as a UString.
        //!
        UString idString() const;

        //!
        //! Return the kind as a std::string, stripping any trailing NUL.
        //! @return The kind tag as a std::string.
        //!
        std::string kindTag() const;

        //!
        //! This method converts a BIOPNameComponent to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML element.
        //!
        void toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! This method decodes an XML BIOPNameComponent.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The XML element containing the name component.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* element);
    };

    //!
    //! A single BIOP Binding inside a Directory / ServiceGateway body.
    //! @see ISO/IEC 13818-6 section 8.6.3
    //! @ingroup libtsduck mpeg
    //!
    struct TSDUCKDLL BIOPBinding {
        static constexpr uint8_t BINDING_TYPE_NOBJECT  = 0x01;  //!< nobject (leaf: file / stream).
        static constexpr uint8_t BINDING_TYPE_NCONTEXT = 0x02;  //!< ncontext (directory).

        std::vector<BIOPNameComponent> name {};        //!< Path components (almost always a single entry).
        uint8_t                        binding_type = 0; //!< 0x01 = nobject, 0x02 = ncontext.
        DSMCCIOR                       ior {};         //!< Object reference to the target.
        ByteBlock                      object_info {}; //!< Kind-specific info (e.g. content_size for files).

        //!
        //! Deserialize one Binding from a buffer.
        //! @param [in,out] buf Deserialization buffer.
        //! @return True on success.
        //!
        bool deserialize(PSIBuffer& buf);

        //!
        //! Get the (module_id, object_key) the binding's IOR points to, if it
        //! carries a BIOP Profile Body containing a BIOP::ObjectLocation.
        //! @return The target location, or std::nullopt if absent.
        //!
        std::optional<std::pair<uint16_t, ByteBlock>> targetLocation() const;

        //!
        //! Get the binding's path joined from its NameComponent ids.
        //! @return Joined path (components separated by '/').
        //!
        UString pathString() const;

        //!
        //! This method converts a BIOPBinding to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML element.
        //!
        void toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! This method decodes an XML BIOPBinding.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The XML element containing the binding.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* element);
    };

    //!
    //! BIOP Message (polymorphic base).
    //!
    //! Parses the 8-byte BIOP header plus the common fields shared by all in-carousel
    //! BIOP messages (message_size, objectKey, objectKind, objectInfo, serviceContextList,
    //! messageBody_length). The body is deserialized by concrete subclasses.
    //!
    //! @see ISO/IEC 13818-6 section 8
    //! @see ETSI TR 101 202 section 4.4
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL BIOPMessage
    {
        TS_NOCOPY(BIOPMessage);

    public:
        BIOPMessageHeader              header {};           //!< BIOP message header.
        ByteBlock                      object_key {};       //!< Object key bytes.
        ByteBlock                      object_kind {};      //!< Object kind bytes on the wire (e.g. 'f','i','l',0). Use kindTag() for the trimmed tag.
        ByteBlock                      object_info {};      //!< Object info bytes (kind-specific; opaque at this level).
        std::vector<BIOPServiceContext> service_contexts {}; //!< Service context list.

        //!
        //! Default constructor.
        //!
        BIOPMessage() = default;

        //!
        //! Virtual destructor.
        //!
        virtual ~BIOPMessage() = default;

        //!
        //! Get the object kind as a trimmed string (without the trailing null, if any).
        //! @return Trimmed kind string.
        //!
        std::string kindTag() const;

        //!
        //! Bindings carried by this message, if any.
        //! @return Pointer to the bindings vector for Directory / ServiceGateway messages,
        //! nullptr for kinds that don't carry bindings.
        //!
        virtual const std::vector<BIOPBinding>* bindingList() const { return nullptr; }

        //!
        //! Parse one BIOP message from a buffer.
        //! Dispatches to the concrete subclass based on object_kind.
        //! @param [in,out] buf Deserialization buffer.
        //! @return Parsed message, or nullptr on error or unsupported kind.
        //!
        static std::unique_ptr<BIOPMessage> Parse(PSIBuffer& buf);

        //!
        //! This method converts a BIOPMessage to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML element.
        //!
        virtual void toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! This method decodes an XML BIOPMessage.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The XML element containing the message.
        //! @return True on success, false on error.
        //!
        virtual bool fromXML(DuckContext& duck, const xml::Element* element);

        //!
        //! Parse a BIOP message from an XML element.
        //! Dispatches to the concrete subclass based on the object_kind attribute.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The XML element to parse.
        //! @return Parsed message, or nullptr on error or unsupported kind.
        //!
        static std::unique_ptr<BIOPMessage> FromXML(DuckContext& duck, const xml::Element* element);

    protected:
        //!
        //! Construct a concrete BIOPMessage subclass for a given object_kind tag.
        //! @param [in] tag Trimmed object_kind string (e.g. "fil", "dir", "srg").
        //! @return A new, empty subclass instance, or nullptr if the kind is unsupported.
        //!
        static std::unique_ptr<BIOPMessage> CreateForKind(const std::string& tag);

        //!
        //! Deserialize the fields common to all BIOP messages, up to (and excluding) the body.
        //! After this call the buffer read position is at the first byte of the body.
        //! @param [in,out] buf Deserialization buffer.
        //! @param [out] body_length Size in bytes of the upcoming message body.
        //! @return True on success.
        //!
        bool deserializeCommon(PSIBuffer& buf, uint32_t& body_length);

        //!
        //! Deserialize the subclass-specific message body.
        //! @param [in,out] buf Deserialization buffer. Exactly the body bytes are available.
        //! @return True on success.
        //!
        virtual bool deserializeBody(PSIBuffer& buf) = 0;

        //!
        //! Serialize the subclass-specific body to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] msg_element The `<BIOP_message>` XML element to append body children to.
        //!
        virtual void toXMLBody(DuckContext& duck, xml::Element* msg_element) const = 0;

        //!
        //! Deserialize the subclass-specific body from XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] msg_element The `<BIOP_message>` XML element containing the body.
        //! @return True on success.
        //!
        virtual bool fromXMLBody(DuckContext& duck, const xml::Element* msg_element) = 0;
    };

    //!
    //! BIOP File Message.
    //!
    //! @see ISO/IEC 13818-6 section 8
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL BIOPFileMessage : public BIOPMessage
    {
    public:
        ByteBlock content {}; //!< File content bytes.

        BIOPFileMessage() = default;

    protected:
        virtual bool deserializeBody(PSIBuffer& buf) override;
        virtual void toXMLBody(DuckContext& duck, xml::Element* msg_element) const override;
        virtual bool fromXMLBody(DuckContext& duck, const xml::Element* msg_element) override;
    };


    //!
    //! Common base for BIOP messages whose body is a list of Bindings.
    //!
    //! Both Directory and ServiceGateway use the identical body layout.
    //! The subclasses below exist only so callers can distinguish them
    //! by type; all serialization logic lives here.
    //!
    //! @see ISO/IEC 13818-6 sections 8.6.3.1 and 8.6.3.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL BIOPBindingListMessage : public BIOPMessage
    {
    public:
        std::vector<BIOPBinding> bindings {}; //!< Child bindings.

        BIOPBindingListMessage() = default;

        virtual const std::vector<BIOPBinding>* bindingList() const override { return &bindings; }

    protected:
        virtual bool deserializeBody(PSIBuffer& buf) override;
        virtual void toXMLBody(DuckContext& duck, xml::Element* msg_element) const override;
        virtual bool fromXMLBody(DuckContext& duck, const xml::Element* msg_element) override;
    };

    //!
    //! BIOP ServiceGateway Message.
    //! The ServiceGateway is the root of the carousel's object hierarchy.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL BIOPServiceGatewayMessage : public BIOPBindingListMessage
    {
    public:
        BIOPServiceGatewayMessage() = default;
        virtual ~BIOPServiceGatewayMessage() override;
    };

    //!
    //! BIOP Directory Message.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL BIOPDirectoryMessage : public BIOPBindingListMessage
    {
    public:
        BIOPDirectoryMessage() = default;
        virtual ~BIOPDirectoryMessage() override;
    };
}  // namespace ts
