//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG-defined MPEGH_3D_audio_scene_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined MPEGH_3D_audio_scene_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.110.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MPEGH3DAudioSceneDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Interactivity group.
        //!
        class TSDUCKDLL MH3D_InteractivityInfo_type
        {
            TS_DEFAULT_COPY_MOVE(MH3D_InteractivityInfo_type);
        public:
            //!
            //! Position interactivity.
            //!
            class TSDUCKDLL PositionInteractivityType
            {
                TS_DEFAULT_COPY_MOVE(PositionInteractivityType);
            public:
                uint8_t mae_interactivityMinAzOffset = 0;    //!< 7 bits. Minimum azimuth offset for changing the position of the members of the metadata element group.
                uint8_t mae_interactivityMaxAzOffset = 0;    //!< 7 bits. Maximum azimuth offset for changing the position of the members of the metadata element group.
                uint8_t mae_interactivityMinElOffset = 0;    //!< 5 bits. Minimum elevation offset for changing the position of the members of the metadata element group.
                uint8_t mae_interactivityMaxElOffset = 0;    //!< 5 bits. Maximum elevation offset for changing the position of the members of the metadata element group.
                uint8_t mae_interactivityMinDistOffset = 0;  //!< 4 bits. Minimum distance factor change got interactivity changing the position of the members of the metadata element group.
                uint8_t mae_interactivityMaxDistOffset = 0;  //!< 4 bits. Maximum distance factor change got interactivity changing the position of the members of the metadata element group.

                //!
                //! Default constructor.
                //!
                PositionInteractivityType();
                //!
                //! Constructor from a binary descriptor
                //! @param [in] buf A binary descriptor to deserialize.
                //!
                PositionInteractivityType(PSIBuffer& buf) : PositionInteractivityType() { deserialize(buf); }

                //! @cond nodoxygen
                void serialize(PSIBuffer&) const;
                void deserialize(PSIBuffer&);
                void toXML(xml::Element*) const;
                bool fromXML(const xml::Element*);
                void display(TablesDisplay&, PSIBuffer&, const UString&);
                //! @endcond
            };
            //!
            //! Gain interactivity.
            //!
            class TSDUCKDLL GainInteractivityType
            {
                TS_DEFAULT_COPY_MOVE(GainInteractivityType);
            public:
                uint8_t mae_interactivityMinGain = 0;  //!< 6 bits. Minumum gain of the members of a metadata element group.
                uint8_t mae_interactivityMaxGain = 0;  //!< 5 bits. Maximum gain of the members of a metadata element group.

                //!
                //! Default constructor.
                //!
                GainInteractivityType();
                //!
                //! Constructor from a binary descriptor
                //! @param [in] buf A binary descriptor to deserialize.
                //!
                GainInteractivityType(PSIBuffer& buf) : GainInteractivityType() { deserialize(buf); }

                //! @cond nodoxygen
                void serialize(PSIBuffer&) const;
                void deserialize(PSIBuffer&);
                void toXML(xml::Element*) const;
                bool fromXML(const xml::Element*);
                void display(TablesDisplay&, PSIBuffer&, const UString&);
                //! @endcond
            };

        public:
            uint8_t                                  mae_groupID = 0;          //!< 7 bits. ID of the group of metadata elements.
            bool                                     mae_allowOnOff = false;   //!< Indicates if the audience is allowed to switch a metadata element group on and off.
            bool                                     mae_defaultOnOff = false; //!< Default status of a metadata element group.
            uint8_t                                  mae_contentKind = 0;      //!< 4 bits. The type of content of a metadata element group, see table 247 of ISO.IEC 23008-3.
            std::optional<PositionInteractivityType> positionInteractivity {}; //!< Position interactivity.
            std::optional<GainInteractivityType>     gainInteractivity {};     //!< Gain interactivity.
            std::optional<UString>                   mae_contentLanguage {};   //!< ISO-639 language code, 3 characters.

            //!
            //! Default constructor.
            //!
            MH3D_InteractivityInfo_type();
            //!
            //! Constructor from a binary descriptor
            //! @param [in] buf A binary descriptor to deserialize.
            //!
            MH3D_InteractivityInfo_type(PSIBuffer& buf) : MH3D_InteractivityInfo_type() { deserialize(buf); }

            //! @cond nodoxygen
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&, uint8_t);
            //! @endcond
        };

        //!
        //! Switch group.
        //!
        class TSDUCKDLL MH3D_SwitchGroup_type
        {
            TS_DEFAULT_COPY_MOVE(MH3D_SwitchGroup_type);
        public:
            uint8_t              mae_switchGroupID = 0;               //!< 5 bits. ID for a switch group of metadata elements
            bool                 mae_switchGroupAllowOnOff = false;   //!< Indicates if the audience is allowed to completely disable the playback of the switch group.
            bool                 mae_switchGroupDefaultOnOff = false; //!< Indicates if the switch grouo is enabled or disabled for playback by default.
            std::vector<uint8_t> mae_switchGroupMemberID {};          //!< 7 bits. Group IDs of the memers of the switch group.
            uint8_t              mae_switchGroupDefaultGroupID = 0;   //!< 7 bits. Signales the default member of the switch group.

            //!
            //! Default constructor.
            //!
            MH3D_SwitchGroup_type();
            //!
            //! Constructor from a binary descriptor
            //! @param [in] buf A binary descriptor to deserialize.
            //!
            MH3D_SwitchGroup_type(PSIBuffer& buf) : MH3D_SwitchGroup_type() { deserialize(buf); }

            //! @cond nodoxygen
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&, uint8_t);
            //! @endcond
        };

        //!
        //! Preset group
        //!
        class TSDUCKDLL MH3D_PresetGroup_type
        {
            TS_DEFAULT_COPY_MOVE(MH3D_PresetGroup_type);
        public:
            //!
            //! Group preset conditions.
            //!
            class TSDUCKDLL GroupPresetConditions_type
            {
                TS_DEFAULT_COPY_MOVE(GroupPresetConditions_type);
            public:
                uint8_t                mae_groupPresetGroupID = 0;                     //!< 7 bits. ID for a group preset.
                std::optional<bool>    mae_groupPresetDisableGainInteractivity {};     //!< Indicates if the gain interactivity of the group of the members shall be disabled.
                std::optional<bool>    mae_groupPresetDisablePositionInteractivity {}; //!< Indicates if the position interactivity of the group of the members shall be disabled.
                std::optional<uint8_t> mae_groupPresetGain {};                         //!< Initial gain of the members of the metadata element group.
                std::optional<uint8_t> mae_groupPresetAzOffset {};                     //!< Additional azimuth offset to be applied to the current group.
                std::optional<uint8_t> mae_groupPresetElOffset {};                     //!< 6 bits. Additional elevation offset to be applied to the current group.
                std::optional<uint8_t> mae_groupPresetDistFactor {};                   //!< 4 bits. Additional distance change factor to be applied to the current group.

                //!
                //! Default constructor.
                //!
                GroupPresetConditions_type();
                //!
                //! Constructor from a binary descriptor
                //! @param [in] buf A binary descriptor to deserialize.
                //!
                GroupPresetConditions_type(PSIBuffer& buf) : GroupPresetConditions_type() { deserialize(buf); }

                //! @cond nodoxygen
                void serialize(PSIBuffer&) const;
                void deserialize(PSIBuffer&);
                void toXML(xml::Element*) const;
                bool fromXML(const xml::Element*);
                void display(TablesDisplay&, PSIBuffer&, const UString&, uint8_t);
                //! @endcond
            };

        public:
            uint8_t                                 mae_groupPresetID = 0;    //!< 5 bits. ID for a group preset.
            uint8_t                                 mae_groupPresetKind = 0;  //!< 5 bits. Kind of content of a group preset, see tabe=le 248 of ISO.IEC 23008-3.
            std::vector<GroupPresetConditions_type> groupPresetConditions {}; //!< Group preset conditions.

            //!
            //! Default constructor.
            //!
            MH3D_PresetGroup_type();
            //!
            //! Constructor from a binary descriptor
            //! @param [in] buf A binary descriptor to deserialize.
            //!
            MH3D_PresetGroup_type(PSIBuffer& buf) : MH3D_PresetGroup_type() { deserialize(buf); }

            //! @cond nodoxygen
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&, uint8_t);
            //! @endcond
        };

    public:
        // MPEGH3DAudioSceneDescriptor public members:
        uint8_t                                  _3dAudioSceneID = 0;    //!< ID for the current audio scene, per mae_audio_SeceneInfoId in ISO/IEC 23008-03.
        std::vector<MH3D_InteractivityInfo_type> interactivityGroups {}; //!< Interactivity groups.
        std::vector<MH3D_SwitchGroup_type>       switchGroups {};        //!< Switch groups.
        std::vector<MH3D_PresetGroup_type>       presetGroups {};        //!< Preset groups.
        ByteBlock                                reserved {};            //!< Reserved data.

        //!
        //! Default constructor.
        //!
        MPEGH3DAudioSceneDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MPEGH3DAudioSceneDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
