# Tables and descriptors cross-reference   {#sigxref}
[TOC]

All signalization tables and descriptors which are supported by TSDuck are
documented in the TSDuck user's guide, appendix C "PSI/SI XML Reference Model".

The tables below summarize all available structures and the reference of
the standard which specifies them.

# Tables   {#sigxtables}

| XML name | C++ class | Defining document
| -------- | --------- | -----------------
| AIT | ts::AIT | ETSI TS 101 812, 10.4.6
| ATSC_EIT | ts::ATSCEIT | ATSC A/65, 6.5
| BAT | ts::BAT | ETSI EN 300 468, 5.2.2
| BIT | ts::BIT | ARIB STD-B10, Part 2, 5.2.13
| cable_emergency_alert_table | ts::CableEmergencyAlertTable | ANSI/SCTE 18, 5
| CAT | ts::CAT | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.6
| CIT | ts::CIT | ETSI TS 102 323, 12.2
| CVCT | ts::CVCT | ATSC A/65, 6.3.2
| DCCSCT | ts::DCCSCT | ATSC A/65, 6.8
| DCCT | ts::DCCT | ATSC A/65, 6.7
| discontinuity_information_table | ts::DiscontinuityInformationTable | ETSI EN 300 468, 7.1.1
| DSMCC_stream_descriptors_table | ts::DSMCCStreamDescriptorsTable | ISO/IEC 13818-6, 9.2.2 and 9.2.7
| EIT | ts::EIT | ETSI EN 300 468, 5.2.4
| ERT | ts::ERT | ARIB STD-B10, Part 3, 5.1.2
| ETT | ts::ETT | ATSC A/65, 6.6
| INT | ts::INT | ETSI EN 301 192, 8.4.3
| ITT | ts::ITT | ARIB STD-B10, Part 3, 5.1.3
| LDT | ts::LDT | ARIB STD-B10, Part 2, 5.2.15
| LIT | ts::LIT | ARIB STD-B10, Part 3, 5.1.1
| MGT | ts::MGT | ATSC A/65, 6.2
| NBIT | ts::NBIT | ARIB STD-B10, Part 2, 5.2.14
| NIT | ts::NIT | ETSI EN 300 468, 5.2.1
| PAT | ts::PAT | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.3
| PCAT | ts::PCAT | ARIB STD-B10, Part 2, 5.2.12
| PMT | ts::PMT | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.8
| RNT | ts::RNT | ETSI TS 102 323, 5.2.2
| RRT | ts::RRT | ATSC A/65, 6.4
| RST | ts::RST | ETSI EN 300 468, 5.2.7
| SDT | ts::SDT | ETSI EN 300 468, 5.2.3
| selection_information_table | ts::SelectionInformationTable | ETSI EN 300 468, 7.1.2
| splice_information_table | ts::SpliceInformationTable | ANSI/SCTE 35, 9.2
| STT | ts::STT | ATSC A/65, 6.1
| TDT | ts::TDT | ETSI EN 300 468, 5.2.5
| TOT | ts::TOT | ETSI EN 300 468, 5.2.6
| TSDT | ts::TSDT | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.12
| TVCT | ts::TVCT | ATSC A/65, 6.3.1
| UNT | ts::UNT | ETSI TS 102 006, 9.4.1

# Descriptors   {#sigxdescs}

| XML name | C++ class | Defining document
| -------- | --------- | -----------------
| AAC_descriptor | ts::AACDescriptor | ETSI EN 300 468, H.2.1
| adaptation_field_data_descriptor | ts::AdaptationFieldDataDescriptor | ETSI EN 300 468, 6.2.1
| af_extensions_descriptor | ts::AFExtensionsDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.99
| ancillary_data_descriptor | ts::AncillaryDataDescriptor | ETSI EN 300 468, 6.2.2
| announcement_support_descriptor | ts::AnnouncementSupportDescriptor | ETSI EN 300 468, 6.2.3
| application_descriptor | ts::ApplicationDescriptor | ETSI TS 102 809, 5.3.5.3
| application_icons_descriptor | ts::ApplicationIconsDescriptor | ETSI TS 102 809, 5.3.5.6.2
| application_name_descriptor | ts::ApplicationNameDescriptor | ETSI TS 101 812, 10.7.4.1
| application_recording_descriptor | ts::ApplicationRecordingDescriptor | ETSI TS 102 809, 5.3.5.4
| application_signalling_descriptor | ts::ApplicationSignallingDescriptor | ETSI TS 102 809, 5.3.5.1
| application_storage_descriptor | ts::ApplicationStorageDescriptor | ETSI TS 102 809, 5.3.10.1
| application_usage_descriptor | ts::ApplicationUsageDescriptor | ETSI TS 102 809, 5.3.5.5
| area_broadcasting_information_descriptor | ts::AreaBroadcastingInformationDescriptor | ARIB STD-B10, Part 2, 6.2.55
| association_tag_descriptor | ts::AssociationTagDescriptor | ISO/IEC 13818-6 (DSM-CC), 11.4.2
| ATSC_AC3_audio_stream_descriptor | ts::ATSCAC3AudioStreamDescriptor | ATSC A/52, A.4.3
| ATSC_EAC3_audio_descriptor | ts::ATSCEAC3AudioDescriptor | ATSC A/52, G.3.5
| ATSC_stuffing_descriptor | ts::ATSCStuffingDescriptor | ATSC A/65, 6.9.8
| ATSC_time_shifted_service_descriptor | ts::ATSCTimeShiftedServiceDescriptor | ATSC A/65, 6.9.6
| audio_component_descriptor | ts::AudioComponentDescriptor | ARIB STD-B10, Part 2, 6.2.26
| audio_preselection_descriptor | ts::AudioPreselectionDescriptor | ETSI EN 300 468, 6.4.1
| audio_stream_descriptor | ts::AudioStreamDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.4
| AVC_timing_and_HRD_descriptor | ts::AVCTimingAndHRDDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.66
| AVC_video_descriptor | ts::AVCVideoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.64
| basic_local_event_descriptor | ts::BasicLocalEventDescriptor | ARIB STD-B10, Part 3, 5.2.1
| board_information_descriptor | ts::BoardInformationDescriptor | ARIB STD-B10, Part 2, 6.2.39
| bouquet_name_descriptor | ts::BouquetNameDescriptor | ETSI EN 300 468, 6.2.4
| broadcaster_name_descriptor | ts::BroadcasterNameDescriptor | ARIB STD-B10, Part 2, 6.2.36
| C2_bundle_delivery_system_descriptor | ts::C2BundleDeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.4
| C2_delivery_system_descriptor | ts::C2DeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.1
| cable_delivery_system_descriptor | ts::CableDeliverySystemDescriptor | ETSI EN 300 468, 6.2.13.1
| CA_contract_info_descriptor | ts::CAContractInfoDescriptor | ARIB STD-B25, Part 1, 4.7.2
| CA_descriptor | ts::CADescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.16
| CA_EMM_TS_descriptor | ts::CAEMMTSDescriptor | ARIB STD-B25, Part 1, 4.7.1
| CA_identifier_descriptor | ts::CAIdentifierDescriptor | ETSI EN 300 468, 6.2.5
| caption_service_descriptor | ts::CaptionServiceDescriptor | ATSC A/65, 6.9.2
| carousel_identifier_descriptor | ts::CarouselIdentifierDescriptor | ISO/IEC 13818-6 (DSM-CC), 11.4.1
| CA_service_descriptor | ts::CAServiceDescriptor | ARIB STD-B25, Part 1, 4.7.3
| cell_frequency_link_descriptor | ts::CellFrequencyLinkDescriptor | ETSI EN 300 468, 6.2.6
| cell_list_descriptor | ts::CellListDescriptor | ETSI EN 300 468, 6.2.7
| CI_ancillary_data_descriptor | ts::CIAncillaryDataDescriptor | ETSI EN 300 468, 6.4.1
| component_descriptor | ts::ComponentDescriptor | ETSI EN 300 468, 6.2.8
| component_name_descriptor | ts::ComponentNameDescriptor | ATSC A/65, 6.9.7
| conditional_playback_descriptor | ts::ConditionalPlaybackDescriptor | ARIB STD-B25, Part 2, 2.3.2.6.4
| content_advisory_descriptor | ts::ContentAdvisoryDescriptor | ATSC A/65, 6.9.3
| content_availability_descriptor | ts::ContentAvailabilityDescriptor | ARIB STD-B10, Part 2, 6.2.45
| content_descriptor | ts::ContentDescriptor | ETSI EN 300 468, 6.2.9
| content_identifier_descriptor | ts::ContentIdentifierDescriptor | ETSI TS 102 323, 12.1
| content_labelling_descriptor | ts::ContentLabellingDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.56
| copyright_descriptor | ts::CopyrightDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.24
| country_availability_descriptor | ts::CountryAvailabilityDescriptor | ETSI EN 300 468, 6.2.10
| CP_descriptor | ts::CPDescriptor | ETSI EN 300 468, 6.4.2
| CP_identifier_descriptor | ts::CPIdentifierDescriptor | ETSI EN 300 468, 6.4.3
| cue_identifier_descriptor | ts::CueIdentifierDescriptor | ANSI/SCTE 35, 8.2
| data_broadcast_descriptor | ts::DataBroadcastDescriptor | ETSI EN 300 468, 6.2.11
| data_broadcast_id_descriptor | ts::DataBroadcastIdDescriptor | ETSI EN 300 468, 6.2.12
| data_component_descriptor | ts::DataComponentDescriptor | ARIB STD-B10, Part 2, 6.2.20
| data_content_descriptor | ts::DataContentDescriptor | ARIB STD-B10, Part 2, 6.2.28
| data_stream_alignment_descriptor | ts::DataStreamAlignmentDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.10
| dcc_arriving_request_descriptor | ts::DCCArrivingRequestDescriptor | ATSC A/65, 6.9.11
| dcc_departing_request_descriptor | ts::DCCDepartingRequestDescriptor | ATSC A/65, 6.9.10
| deferred_association_tags_descriptor | ts::DeferredAssociationTagsDescriptor | ISO/IEC 13818-6 (DSM-CC), 11.4.3
| digital_copy_control_descriptor | ts::DigitalCopyControlDescriptor | ARIB STD-B10, Part 2, 6.2.23
| DII_location_descriptor | ts::DIILocationDescriptor | ETSI TS 101 812, 10.8.3.3
| DSNG_descriptor | ts::DSNGDescriptor | ETSI EN 300 468, 6.2.14
| dtg_guidance_descriptor | ts::DTGGuidanceDescriptor | The D-Book 7 Part A (DTG), 8.5.3.20
| dtg_HD_simulcast_logical_channel_descriptor | ts::DTGHDSimulcastLogicalChannelDescriptor | The D-Book 7 Part A (DTG), 8.5.3.23
| dtg_logical_channel_descriptor | ts::DTGLogicalChannelDescriptor | The D-Book 7 Part A (DTG), 8.5.3.6
| dtg_preferred_name_identifier_descriptor | ts::DTGPreferredNameIdentifierDescriptor | The D-Book 7 Part A (DTG), 8.5.3.8
| dtg_preferred_name_list_descriptor | ts::DTGPreferredNameListDescriptor | The D-Book 7 Part A (DTG), 8.5.3.7
| dtg_service_attribute_descriptor | ts::DTGServiceAttributeDescriptor | The D-Book 7 Part A (DTG), 8.5.3.9
| dtg_short_service_name_descriptor | ts::DTGShortServiceNameDescriptor | The D-Book 7 Part A (DTG), 8.5.3.10
| DTS_descriptor | ts::DTSDescriptor | ETSI EN 300 468, G.2.1
| DTS_HD_descriptor | ts::DTSHDDescriptor | ETSI EN 300 468, G.3.1
| DTS_neural_descriptor | ts::DTSNeuralDescriptor | ETSI EN 300 468, L.1
| DVB_AC3_descriptor | ts::DVBAC3Descriptor | ETSI EN 300 468, D.3
| DVB_AC4_descriptor | ts::DVBAC4Descriptor | ETSI EN 300 468, D.7
| DVB_enhanced_AC3_descriptor | ts::DVBEnhancedAC3Descriptor | ETSI EN 300 468, D.5
| dvb_html_application_boundary_descriptor | ts::DVBHTMLApplicationBoundaryDescriptor | ETSI TS 101 812, 10.10.3
| dvb_html_application_descriptor | ts::DVBHTMLApplicationDescriptor | ETSI TS 101 812, 10.10.1
| dvb_html_application_location_descriptor | ts::DVBHTMLApplicationLocationDescriptor | ETSI TS 101 812, 10.10.2
| dvb_j_application_descriptor | ts::DVBJApplicationDescriptor | ETSI TS 101 812, 10.9.1
| dvb_j_application_location_descriptor | ts::DVBJApplicationLocationDescriptor | ETSI TS 101 812, 10.9.2
| DVB_stuffing_descriptor | ts::DVBStuffingDescriptor | ETSI EN 300 468, 6.2.40
| DVB_time_shifted_service_descriptor | ts::DVBTimeShiftedServiceDescriptor | ETSI EN 300 468, 6.2.45
| eacem_logical_channel_number_descriptor | ts::EacemLogicalChannelNumberDescriptor | EACEM Technical Report Number TR-030, 9.2.11.2
| eacem_preferred_name_identifier_descriptor | ts::EacemPreferredNameIdentifierDescriptor | EACEM Technical Report Number TR-030, 9.2.11.2
| eacem_preferred_name_list_descriptor | ts::EacemPreferredNameListDescriptor | EACEM Technical Report Number TR-030, 9.2.11.2
| eacem_stream_identifier_descriptor | ts::EacemStreamIdentifierDescriptor | EACEM Technical Report Number TR-030, 9.2.11.2
| EAS_audio_file_descriptor | ts::EASAudioFileDescriptor | ANSI/SCTE 18, 5.1.3
| EAS_inband_details_channel_descriptor | ts::EASInbandDetailsChannelDescriptor | ANSI/SCTE 18, 5.1.1
| EAS_inband_exception_channels_descriptor | ts::EASInbandExceptionChannelsDescriptor | ANSI/SCTE 18, 5.1.2
| EAS_metadata_descriptor | ts::EASMetadataDescriptor | ANSI/SCTE 164, 5.0
| ECM_repetition_rate_descriptor | ts::ECMRepetitionRateDescriptor | ETSI EN 301 192, 9.7
| emergency_information_descriptor | ts::EmergencyInformationDescriptor | ARIB STD-B10, Part 2, 6.2.24
| event_group_descriptor | ts::EventGroupDescriptor | ARIB STD-B10, Part 2, 6.2.34
| extended_broadcaster_descriptor | ts::ExtendedBroadcasterDescriptor | ARIB STD-B10, Part 2, 6.2.43
| extended_channel_name_descriptor | ts::ExtendedChannelNameDescriptor | ATSC A/65, 6.9.4
| extended_event_descriptor | ts::ExtendedEventDescriptor | ETSI EN 300 468, 6.2.15
| external_application_authorization_descriptor | ts::ExternalApplicationAuthorizationDescriptor | ETSI TS 102 809, 5.3.5.7
| external_ES_ID_descriptor | ts::ExternalESIdDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.46
| flexmux_timing_descriptor | ts::FlexMuxTimingDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.54
| FMC_descriptor | ts::FMCDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.44
| frequency_list_descriptor | ts::FrequencyListDescriptor | ETSI EN 300 468, 6.2.17
| FTA_content_management_descriptor | ts::FTAContentManagementDescriptor | ETSI EN 300 468, 6.2.18
| genre_descriptor | ts::GenreDescriptor | ATSC A/65, 6.9.13
| graphics_constraints_descriptor | ts::GraphicsConstraintsDescriptor | ETSI TS 102 809, 5.3.5.8
| green_extension_descriptor | ts::GreenExtensionDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.104
| HEVC_hierarchy_extension_descriptor | ts::HEVCHierarchyExtensionDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.102
| HEVC_timing_and_HRD_descriptor | ts::HEVCTimingAndHRDDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.97
| HEVC_video_descriptor | ts::HEVCVideoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.95
| hierarchical_transmission_descriptor | ts::HierarchicalTransmissionDescriptor | ARIB STD-B10, Part 2, 6.2.22
| hierarchy_descriptor | ts::HierarchyDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.6
| hybrid_information_descriptor | ts::HybridInformationDescriptor | ARIB STD-B10, Part 2, 6.2.58
| IBP_descriptor | ts::IBPDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.34
| image_icon_descriptor | ts::ImageIconDescriptor | ETSI EN 300 468, 6.4.7
| IPMAC_generic_stream_location_descriptor | ts::IPMACGenericStreamLocationDescriptor | ETSI EN 301 192, 8.4.5.15
| IPMAC_platform_name_descriptor | ts::IPMACPlatformNameDescriptor | ETSI EN 301 192, 8.4.5.2
| IPMAC_platform_provider_name_descriptor | ts::IPMACPlatformProviderNameDescriptor | ETSI EN 301 192, 8.4.5.3
| IPMAC_stream_location_descriptor | ts::IPMACStreamLocationDescriptor | ETSI EN 301 192, 8.4.5.14
| ip_signalling_descriptor | ts::IPSignallingDescriptor | ETSI TS 101 812, 10.8.2
| ISDB_access_control_descriptor | ts::ISDBAccessControlDescriptor | ARIB STD-B10, Part 2, 6.2.54
| ISDB_terrestrial_delivery_system_descriptor | ts::ISDBTerrestrialDeliverySystemDescriptor | ARIB STD-B10, Part 2, 6.2.31
| ISO_639_language_descriptor | ts::ISO639LanguageDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.18
| ISP_access_mode_descriptor | ts::ISPAccessModeDescriptor | ETSI EN 301 192, 8.4.5.16
| J2K_video_descriptor | ts::J2KVideoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.80
| linkage_descriptor | ts::LinkageDescriptor | ETSI EN 300 468, 6.2.19
| linkage_descriptor | ts::SSULinkageDescriptor | ETSI EN 300 468, 6.2.19
| local_time_offset_descriptor | ts::LocalTimeOffsetDescriptor | ETSI EN 300 468, 6.2.20
| logo_transmission_descriptor | ts::LogoTransmissionDescriptor | ARIB STD-B10, Part 2, 6.2.44
| maximum_bitrate_descriptor | ts::MaximumBitrateDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.26
| message_descriptor | ts::MessageDescriptor | ETSI EN 300 468, 6.4.7
| metadata_descriptor | ts::MetadataDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.60
| metadata_pointer_descriptor | ts::MetadataPointerDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.58
| metadata_STD_descriptor | ts::MetadataSTDDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.62
| mosaic_descriptor | ts::MosaicDescriptor | ETSI EN 300 468, 6.2.21
| MPEG2_AAC_audio_descriptor | ts::MPEG2AACAudioDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.68
| MPEG2_stereoscopic_video_format_descriptor | ts::MPEG2StereoscopicVideoFormatDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.84
| MPEG4_audio_descriptor | ts::MPEG4AudioDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.38
| MPEG4_video_descriptor | ts::MPEG4VideoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.36
| MPEGH_3D_audio_descriptor | ts::MPEGH3DAudioDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.106
| MPEGH_3D_audio_multi_stream_descriptor | ts::MPEGH3DAudioMultiStreamDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.114
| multilingual_bouquet_name_descriptor | ts::MultilingualBouquetNameDescriptor | ETSI EN 300 468, 6.2.22
| multilingual_component_descriptor | ts::MultilingualComponentDescriptor | ETSI EN 300 468, 6.2.23
| multilingual_network_name_descriptor | ts::MultilingualNetworkNameDescriptor | ETSI EN 300 468, 6.2.24
| multilingual_service_name_descriptor | ts::MultilingualServiceNameDescriptor | ETSI EN 300 468, 6.2.25
| multiplex_buffer_descriptor | ts::MultiplexBufferDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.52
| multiplex_buffer_utilization_descriptor | ts::MultiplexBufferUtilizationDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.22
| MVC_extension_descriptor | ts::MVCExtensionDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.78
| MVC_operation_point_descriptor | ts::MVCOperationPointDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.82
| network_change_notify_descriptor | ts::NetworkChangeNotifyDescriptor | ETSI EN 300 468, 6.4.9
| network_name_descriptor | ts::NetworkNameDescriptor | ETSI EN 300 468, 6.2.27
| node_relation_descriptor | ts::NodeRelationDescriptor | ARIB STD-B10, Part 3, 5.2.3
| nordig_logical_channel_descriptor_v1 | ts::NorDigLogicalChannelDescriptorV1 | NorDig Unified Requirements ver. 3.1.1, 12.2.9.2
| nordig_logical_channel_descriptor_v2 | ts::NorDigLogicalChannelDescriptorV2 | NorDig Unified Requirements ver. 3.1.1, 12.2.9.3
| NPT_endpoint_descriptor | ts::NPTEndpointDescriptor | ISO/IEC 13818-6, 8.1.5
| NPT_reference_descriptor | ts::NPTReferenceDescriptor | ISO/IEC 13818-6, 8.1.1
| NVOD_reference_descriptor | ts::NVODReferenceDescriptor | ETSI EN 300 468, 6.2.26
| parental_rating_descriptor | ts::ParentalRatingDescriptor | ETSI EN 300 468, 6.2.28
| partial_reception_descriptor | ts::PartialReceptionDescriptor | ARIB STD-B10, Part 2, 6.2.32
| partial_transport_stream_descriptor | ts::PartialTransportStreamDescriptor | ETSI EN 300 468, 7.2.1
| PDC_descriptor | ts::PDCDescriptor | ETSI EN 300 468, 6.2.30
| prefetch_descriptor | ts::PrefetchDescriptor | ETSI TS 101 812, 10.8.3.2
| private_data_indicator_descriptor | ts::PrivateDataIndicatorDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.28
| private_data_specifier_descriptor | ts::PrivateDataSpecifierDescriptor | ETSI EN 300 468, 6.2.31
| protection_message_descriptor | ts::ProtectionMessageDescriptor | ETSI TS 102 809, 9.3.3
| redistribution_control_descriptor | ts::RedistributionControlDescriptor | ATSC A/65, 6.9.12
| reference_descriptor | ts::ReferenceDescriptor | ARIB STD-B10, Part 3, 5.2.2
| registration_descriptor | ts::RegistrationDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.8
| related_content_descriptor | ts::RelatedContentDescriptor | ETSI TS 102 323, 10.3
| S2_satellite_delivery_system_descriptor | ts::S2SatelliteDeliverySystemDescriptor | ETSI EN 300 468, 6.2.13.3
| S2X_satellite_delivery_system_descriptor | ts::S2XSatelliteDeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.5
| satellite_delivery_system_descriptor | ts::SatelliteDeliverySystemDescriptor | ETSI EN 300 468, 6.2.13.2
| scheduling_descriptor | ts::SchedulingDescriptor | ETSI TS 102 006, 9.5.2.9
| scrambling_descriptor | ts::ScramblingDescriptor | ETSI EN 300 468, 6.2.32
| series_descriptor | ts::SeriesDescriptor | ARIB STD-B10, Part 2, 6.2.33
| service_availability_descriptor | ts::ServiceAvailabilityDescriptor | ETSI EN 300 468, 6.2.34
| service_descriptor | ts::ServiceDescriptor | ETSI EN 300 468, 6.2.33
| service_group_descriptor | ts::ServiceGroupDescriptor | ARIB STD-B10, Part 2, 6.2.49
| service_identifier_descriptor | ts::ServiceIdentifierDescriptor | ETSI TS 102 809, 6.2.1
| service_list_descriptor | ts::ServiceListDescriptor | ETSI EN 300 468, 6.2.35
| service_location_descriptor | ts::ServiceLocationDescriptor | ATSC A/65, 6.9.5
| service_move_descriptor | ts::ServiceMoveDescriptor | ETSI EN 300 468, 6.2.34
| service_relocated_descriptor | ts::ServiceRelocatedDescriptor | ETSI EN 300 468, 6.4.9
| SH_delivery_system_descriptor | ts::SHDeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.2
| short_event_descriptor | ts::ShortEventDescriptor | ETSI EN 300 468, 6.2.37
| short_node_information_descriptor | ts::ShortNodeInformationDescriptor | ARIB STD-B10, Part 3, 5.2.4
| short_smoothing_buffer_descriptor | ts::ShortSmoothingBufferDescriptor | ETSI EN 300 468, 6.2.38
| simple_application_boundary_descriptor | ts::SimpleApplicationBoundaryDescriptor | ETSI TS 102 809, 5.3.8
| simple_application_location_descriptor | ts::SimpleApplicationLocationDescriptor | ETSI TS 102 809, 5.3.7
| SI_parameter_descriptor | ts::SIParameterDescriptor | ARIB STD-B10, Part 2, 6.2.35
| SI_prime_TS_descriptor | ts::SIPrimeTSDescriptor | ARIB STD-B10, Part 2, 6.2.38
| SL_descriptor | ts::SLDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.42
| smoothing_buffer_descriptor | ts::SmoothingBufferDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.30
| splice_avail_descriptor | ts::SpliceAvailDescriptor | ANSI/SCTE 35, 10.3.1
| splice_DTMF_descriptor | ts::SpliceDTMFDescriptor | ANSI/SCTE 35, 10.3.2
| splice_segmentation_descriptor | ts::SpliceSegmentationDescriptor | ANSI/SCTE 35, 10.3.3
| splice_time_descriptor | ts::SpliceTimeDescriptor | ANSI/SCTE 35, 10.3.4
| SSU_enhanced_message_descriptor | ts::SSUEnhancedMessageDescriptor | ETSI TS 102 006, 9.5.2.14
| SSU_event_name_descriptor | ts::SSUEventNameDescriptor | ETSI TS 102 006, 9.5.2.11
| SSU_location_descriptor | ts::SSULocationDescriptor | ETSI TS 102 006, 9.5.2.7
| SSU_message_descriptor | ts::SSUMessageDescriptor | ETSI TS 102 006, 9.5.2.12
| SSU_subgroup_association_descriptor | ts::SSUSubgroupAssociationDescriptor | ETSI TS 102 006, 9.5.2.8
| SSU_uri_descriptor | ts::SSUURIDescriptor | ETSI TS 102 006, 9.5.2.15
| STC_reference_descriptor | ts::STCReferenceDescriptor | ARIB STD-B10, Part 3, 5.2.5
| STD_descriptor | ts::STDDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.32
| stereoscopic_program_info_descriptor | ts::StereoscopicProgramInfoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.86
| stereoscopic_video_info_descriptor | ts::StereoscopicVideoInfoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.88
| stream_event_descriptor | ts::StreamEventDescriptor | ISO/IEC 13818-6, 8.3
| stream_identifier_descriptor | ts::StreamIdentifierDescriptor | ETSI EN 300 468, 6.2.39
| stream_mode_descriptor | ts::StreamModeDescriptor | ISO/IEC 13818-6, 8.2
| subtitling_descriptor | ts::SubtitlingDescriptor | ETSI EN 300 468, 6.2.41
| supplementary_audio_descriptor | ts::SupplementaryAudioDescriptor | ETSI EN 300 468, 6.4.11
| SVC_extension_descriptor | ts::SVCExtensionDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.76
| system_clock_descriptor | ts::SystemClockDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.20
| system_management_descriptor | ts::SystemManagementDescriptor | ARIB STD-B10, Part 2, 6.2.21
| T2_delivery_system_descriptor | ts::T2DeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.3
| T2MI_descriptor | ts::T2MIDescriptor | ETSI EN 300 468, 6.4.14
| target_background_grid_descriptor | ts::TargetBackgroundGridDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.12
| target_IP_address_descriptor | ts::TargetIPAddressDescriptor | ETSI EN 301 192, 8.4.5.8
| target_IP_slash_descriptor | ts::TargetIPSlashDescriptor | ETSI EN 301 192, 8.4.5.9
| target_IP_source_slash_descriptor | ts::TargetIPSourceSlashDescriptor | ETSI EN 301 192, 8.4.5.10
| target_IPv6_address_descriptor | ts::TargetIPv6AddressDescriptor | ETSI EN 301 192, 8.4.5.11
| target_IPv6_slash_descriptor | ts::TargetIPv6SlashDescriptor | ETSI EN 301 192, 8.4.5.12
| target_IPv6_source_slash_descriptor | ts::TargetIPv6SourceSlashDescriptor | ETSI EN 301 192, 8.4.5.13
| target_MAC_address_descriptor | ts::TargetMACAddressDescriptor | ETSI EN 301 192, 8.4.5.6
| target_MAC_address_range_descriptor | ts::TargetMACAddressRangeDescriptor | ETSI EN 301 192, 8.4.5.7
| target_region_descriptor | ts::TargetRegionDescriptor | ETSI EN 300 468, 6.4.12
| target_region_name_descriptor | ts::TargetRegionNameDescriptor | ETSI EN 300 468, 6.4.13
| target_serial_number_descriptor | ts::TargetSerialNumberDescriptor | ETSI EN 301 192, 8.4.5.4
| target_smartcard_descriptor | ts::TargetSmartcardDescriptor | ETSI EN 301 192, 8.4.5.5
| telephone_descriptor | ts::TelephoneDescriptor | ETSI EN 300 468, 6.2.42
| teletext_descriptor | ts::TeletextDescriptor | ETSI EN 300 468, 6.2.43
| terrestrial_delivery_system_descriptor | ts::TerrestrialDeliverySystemDescriptor | ETSI EN 300 468, 6.2.13.4
| time_shifted_event_descriptor | ts::TimeShiftedEventDescriptor | ETSI EN 300 468, 6.2.44
| time_slice_fec_identifier_descriptor | ts::TimeSliceFECIdentifierDescriptor | ETSI EN 301 192, 9.5
| transport_profile_descriptor | ts::TransportProfileDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.93
| transport_protocol_descriptor | ts::TransportProtocolDescriptor | ETSI TS 101 812, 10.8.1
| transport_stream_descriptor | ts::TransportStreamDescriptor | ETSI EN 300 468, 6.2.46
| TS_information_descriptor | ts::TSInformationDescriptor | ARIB STD-B10, Part 2, 6.2.42
| TVA_id_descriptor | ts::TVAIdDescriptor | ETSI TS 102 323, 11.2.4
| update_descriptor | ts::UpdateDescriptor | ETSI TS 102 006, 9.5.2.6
| URI_linkage_descriptor | ts::URILinkageDescriptor | ETSI EN 300 468, 6.4.15
| VBI_data_descriptor | ts::VBIDataDescriptor | ETSI EN 300 468, 6.2.47
| VBI_teletext_descriptor | ts::VBITeletextDescriptor | ETSI EN 300 468, 6.2.48
| video_decode_control_descriptor | ts::VideoDecodeControlDescriptor | ARIB STD-B10, Part 2, 6.2.30
| video_depth_range_descriptor | ts::VideoDepthRangeDescriptor | ETSI EN 300 468, 6.4.16
| video_stream_descriptor | ts::VideoStreamDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.2
| video_window_descriptor | ts::VideoWindowDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.14
| virtual_segmentation_descriptor | ts::VirtualSegmentationDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.120
