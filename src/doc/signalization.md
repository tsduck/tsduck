# Tables and descriptors cross-reference   {#sigxref}
[TOC]

All signalization tables and descriptors which are supported by TSDuck are
documented in the TSDuck user's guide, appendix C "PSI/SI XML Reference Model".

The tables below summarize all available structures and the reference of
the standard which specifies them.

## Tables

| XML name | C++ class | Defining document
| -------- | --------- | -----------------
| AIT | AIT | ETSI TS 101 812, 10.4.6
| ATSC_EIT | ATSCEIT | ATSC A/65, 6.5
| BAT | BAT | ETSI EN 300 468, 5.2.2
| BIT | BIT | ARIB STD-B10, Part 2, 5.2.13
| cable_emergency_alert_table | CableEmergencyAlertTable | ANSI/SCTE 18, 5
| CAT | CAT | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.6
| CIT | CIT | ETSI TS 102 323, 12.2
| CVCT | CVCT | ATSC A/65, 6.3.2
| DCCSCT | DCCSCT | ATSC A/65, 6.8
| DCCT | DCCT | ATSC A/65, 6.7
| discontinuity_information_table | DiscontinuityInformationTable | ETSI EN 300 468, 7.1.1
| DSMCC_stream_descriptors_table | DSMCCStreamDescriptorsTable | ISO/IEC 13818-6, 9.2.2 and 9.2.7
| EIT | EIT | ETSI EN 300 468, 5.2.4
| ERT | ERT | ARIB STD-B10, Part 3, 5.1.2
| ETT | ETT | ATSC A/65, 6.6
| INT | INT | ETSI EN 301 192, 8.4.3
| ITT | ITT | ARIB STD-B10, Part 3, 5.1.3
| LDT | LDT | ARIB STD-B10, Part 2, 5.2.15
| LIT | LIT | ARIB STD-B10, Part 3, 5.1.1
| MGT | MGT | ATSC A/65, 6.2
| NBIT | NBIT | ARIB STD-B10, Part 2, 5.2.14
| NIT | NIT | ETSI EN 300 468, 5.2.1
| PAT | PAT | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.3
| PCAT | PCAT | ARIB STD-B10, Part 2, 5.2.12
| PMT | PMT | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.8
| RNT | RNT | ETSI TS 102 323, 5.2.2
| RRT | RRT | ATSC A/65, 6.4
| RST | RST | ETSI EN 300 468, 5.2.7
| SDT | SDT | ETSI EN 300 468, 5.2.3
| selection_information_table | SelectionInformationTable | ETSI EN 300 468, 7.1.2
| splice_information_table | SpliceInformationTable | ANSI/SCTE 35, 9.2
| STT | STT | ATSC A/65, 6.1
| TDT | TDT | ETSI EN 300 468, 5.2.5
| TOT | TOT | ETSI EN 300 468, 5.2.6
| TSDT | TSDT | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.12
| TVCT | TVCT | ATSC A/65, 6.3.1
| UNT | UNT | ETSI TS 102 006, 9.4.1

## Descriptors

| XML name | C++ class | Defining document
| -------- | --------- | -----------------
| AAC_descriptor | AACDescriptor | ETSI EN 300 468, H.2.1
| adaptation_field_data_descriptor | AdaptationFieldDataDescriptor | ETSI EN 300 468, 6.2.1
| af_extensions_descriptor | AFExtensionsDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.99
| ancillary_data_descriptor | AncillaryDataDescriptor | ETSI EN 300 468, 6.2.2
| announcement_support_descriptor | AnnouncementSupportDescriptor | ETSI EN 300 468, 6.2.3
| application_descriptor | ApplicationDescriptor | ETSI TS 102 809, 5.3.5.3
| application_icons_descriptor | ApplicationIconsDescriptor | ETSI TS 102 809, 5.3.5.6.2
| application_name_descriptor | ApplicationNameDescriptor | ETSI TS 101 812, 10.7.4.1
| application_recording_descriptor | ApplicationRecordingDescriptor | ETSI TS 102 809, 5.3.5.4
| application_signalling_descriptor | ApplicationSignallingDescriptor | ETSI TS 102 809, 5.3.5.1
| application_storage_descriptor | ApplicationStorageDescriptor | ETSI TS 102 809, 5.3.10.1
| application_usage_descriptor | ApplicationUsageDescriptor | ETSI TS 102 809, 5.3.5.5
| area_broadcasting_information_descriptor | AreaBroadcastingInformationDescriptor | ARIB STD-B10, Part 2, 6.2.55
| association_tag_descriptor | AssociationTagDescriptor | ISO/IEC 13818-6 (DSM-CC), 11.4.2
| ATSC_AC3_audio_stream_descriptor | ATSCAC3AudioStreamDescriptor | ATSC A/52, A.4.3
| ATSC_EAC3_audio_descriptor | ATSCEAC3AudioDescriptor | ATSC A/52, G.3.5
| ATSC_stuffing_descriptor | ATSCStuffingDescriptor | ATSC A/65, 6.9.8
| ATSC_time_shifted_service_descriptor | ATSCTimeShiftedServiceDescriptor | ATSC A/65, 6.9.6
| audio_component_descriptor | AudioComponentDescriptor | ARIB STD-B10, Part 2, 6.2.26
| audio_preselection_descriptor | AudioPreselectionDescriptor | ETSI EN 300 468, 6.4.1
| audio_stream_descriptor | AudioStreamDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.4
| AVC_timing_and_HRD_descriptor | AVCTimingAndHRDDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.66
| AVC_video_descriptor | AVCVideoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.64
| basic_local_event_descriptor | BasicLocalEventDescriptor | ARIB STD-B10, Part 3, 5.2.1
| board_information_descriptor | BoardInformationDescriptor | ARIB STD-B10, Part 2, 6.2.39
| bouquet_name_descriptor | BouquetNameDescriptor | ETSI EN 300 468, 6.2.4
| broadcaster_name_descriptor | BroadcasterNameDescriptor | ARIB STD-B10, Part 2, 6.2.36
| C2_bundle_delivery_system_descriptor | C2BundleDeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.4
| C2_delivery_system_descriptor | C2DeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.1
| cable_delivery_system_descriptor | CableDeliverySystemDescriptor | ETSI EN 300 468, 6.2.13.1
| CA_contract_info_descriptor | CAContractInfoDescriptor | ARIB STD-B25, Part 1, 4.7.2
| CA_descriptor | CADescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.16
| CA_EMM_TS_descriptor | CAEMMTSDescriptor | ARIB STD-B25, Part 1, 4.7.1
| CA_identifier_descriptor | CAIdentifierDescriptor | ETSI EN 300 468, 6.2.5
| caption_service_descriptor | CaptionServiceDescriptor | ATSC A/65, 6.9.2
| carousel_identifier_descriptor | CarouselIdentifierDescriptor | ISO/IEC 13818-6 (DSM-CC), 11.4.1
| CA_service_descriptor | CAServiceDescriptor | ARIB STD-B25, Part 1, 4.7.3
| cell_frequency_link_descriptor | CellFrequencyLinkDescriptor | ETSI EN 300 468, 6.2.6
| cell_list_descriptor | CellListDescriptor | ETSI EN 300 468, 6.2.7
| CI_ancillary_data_descriptor | CIAncillaryDataDescriptor | ETSI EN 300 468, 6.4.1
| component_descriptor | ComponentDescriptor | ETSI EN 300 468, 6.2.8
| component_name_descriptor | ComponentNameDescriptor | ATSC A/65, 6.9.7
| conditional_playback_descriptor | ConditionalPlaybackDescriptor | ARIB STD-B25, Part 2, 2.3.2.6.4
| content_advisory_descriptor | ContentAdvisoryDescriptor | ATSC A/65, 6.9.3
| content_availability_descriptor | ContentAvailabilityDescriptor | ARIB STD-B10, Part 2, 6.2.45
| content_descriptor | ContentDescriptor | ETSI EN 300 468, 6.2.9
| content_identifier_descriptor | ContentIdentifierDescriptor | ETSI TS 102 323, 12.1
| content_labelling_descriptor | ContentLabellingDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.56
| copyright_descriptor | CopyrightDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.24
| country_availability_descriptor | CountryAvailabilityDescriptor | ETSI EN 300 468, 6.2.10
| CP_descriptor | CPDescriptor | ETSI EN 300 468, 6.4.2
| CP_identifier_descriptor | CPIdentifierDescriptor | ETSI EN 300 468, 6.4.3
| cue_identifier_descriptor | CueIdentifierDescriptor | ANSI/SCTE 35, 8.2
| data_broadcast_descriptor | DataBroadcastDescriptor | ETSI EN 300 468, 6.2.11
| data_broadcast_id_descriptor | DataBroadcastIdDescriptor | ETSI EN 300 468, 6.2.12
| data_component_descriptor | DataComponentDescriptor | ARIB STD-B10, Part 2, 6.2.20
| data_content_descriptor | DataContentDescriptor | ARIB STD-B10, Part 2, 6.2.28
| data_stream_alignment_descriptor | DataStreamAlignmentDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.10
| dcc_arriving_request_descriptor | DCCArrivingRequestDescriptor | ATSC A/65, 6.9.11
| dcc_departing_request_descriptor | DCCDepartingRequestDescriptor | ATSC A/65, 6.9.10
| deferred_association_tags_descriptor | DeferredAssociationTagsDescriptor | ISO/IEC 13818-6 (DSM-CC), 11.4.3
| digital_copy_control_descriptor | DigitalCopyControlDescriptor | ARIB STD-B10, Part 2, 6.2.23
| DII_location_descriptor | DIILocationDescriptor | ETSI TS 101 812, 10.8.3.3
| DSNG_descriptor | DSNGDescriptor | ETSI EN 300 468, 6.2.14
| dtg_guidance_descriptor | DTGGuidanceDescriptor | The D-Book 7 Part A (DTG), 8.5.3.20
| dtg_HD_simulcast_logical_channel_descriptor | DTGHDSimulcastLogicalChannelDescriptor | The D-Book 7 Part A (DTG), 8.5.3.23
| dtg_logical_channel_descriptor | DTGLogicalChannelDescriptor | The D-Book 7 Part A (DTG), 8.5.3.6
| dtg_preferred_name_identifier_descriptor | DTGPreferredNameIdentifierDescriptor | The D-Book 7 Part A (DTG), 8.5.3.8
| dtg_preferred_name_list_descriptor | DTGPreferredNameListDescriptor | The D-Book 7 Part A (DTG), 8.5.3.7
| dtg_service_attribute_descriptor | DTGServiceAttributeDescriptor | The D-Book 7 Part A (DTG), 8.5.3.9
| dtg_short_service_name_descriptor | DTGShortServiceNameDescriptor | The D-Book 7 Part A (DTG), 8.5.3.10
| DTS_descriptor | DTSDescriptor | ETSI EN 300 468, G.2.1
| DTS_HD_descriptor | DTSHDDescriptor | ETSI EN 300 468, G.3.1
| DTS_neural_descriptor | DTSNeuralDescriptor | ETSI EN 300 468, L.1
| DVB_AC3_descriptor | DVBAC3Descriptor | ETSI EN 300 468, D.3
| DVB_AC4_descriptor | DVBAC4Descriptor | ETSI EN 300 468, D.7
| DVB_enhanced_AC3_descriptor | DVBEnhancedAC3Descriptor | ETSI EN 300 468, D.5
| dvb_html_application_boundary_descriptor | DVBHTMLApplicationBoundaryDescriptor | ETSI TS 101 812, 10.10.3
| dvb_html_application_descriptor | DVBHTMLApplicationDescriptor | ETSI TS 101 812, 10.10.1
| dvb_html_application_location_descriptor | DVBHTMLApplicationLocationDescriptor | ETSI TS 101 812, 10.10.2
| dvb_j_application_descriptor | DVBJApplicationDescriptor | ETSI TS 101 812, 10.9.1
| dvb_j_application_location_descriptor | DVBJApplicationLocationDescriptor | ETSI TS 101 812, 10.9.2
| DVB_stuffing_descriptor | DVBStuffingDescriptor | ETSI EN 300 468, 6.2.40
| DVB_time_shifted_service_descriptor | DVBTimeShiftedServiceDescriptor | ETSI EN 300 468, 6.2.45
| eacem_logical_channel_number_descriptor | EacemLogicalChannelNumberDescriptor | EACEM Technical Report Number TR-030, 9.2.11.2
| eacem_preferred_name_identifier_descriptor | EacemPreferredNameIdentifierDescriptor | EACEM Technical Report Number TR-030, 9.2.11.2
| eacem_preferred_name_list_descriptor | EacemPreferredNameListDescriptor | EACEM Technical Report Number TR-030, 9.2.11.2
| eacem_stream_identifier_descriptor | EacemStreamIdentifierDescriptor | EACEM Technical Report Number TR-030, 9.2.11.2
| EAS_audio_file_descriptor | EASAudioFileDescriptor | ANSI/SCTE 18, 5.1.3
| EAS_inband_details_channel_descriptor | EASInbandDetailsChannelDescriptor | ANSI/SCTE 18, 5.1.1
| EAS_inband_exception_channels_descriptor | EASInbandExceptionChannelsDescriptor | ANSI/SCTE 18, 5.1.2
| EAS_metadata_descriptor | EASMetadataDescriptor | ANSI/SCTE 164, 5.0
| ECM_repetition_rate_descriptor | ECMRepetitionRateDescriptor | ETSI EN 301 192, 9.7
| emergency_information_descriptor | EmergencyInformationDescriptor | ARIB STD-B10, Part 2, 6.2.24
| event_group_descriptor | EventGroupDescriptor | ARIB STD-B10, Part 2, 6.2.34
| extended_broadcaster_descriptor | ExtendedBroadcasterDescriptor | ARIB STD-B10, Part 2, 6.2.43
| extended_channel_name_descriptor | ExtendedChannelNameDescriptor | ATSC A/65, 6.9.4
| extended_event_descriptor | ExtendedEventDescriptor | ETSI EN 300 468, 6.2.15
| external_application_authorization_descriptor | ExternalApplicationAuthorizationDescriptor | ETSI TS 102 809, 5.3.5.7
| external_ES_ID_descriptor | ExternalESIdDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.46
| flexmux_timing_descriptor | FlexMuxTimingDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.54
| FMC_descriptor | FMCDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.44
| frequency_list_descriptor | FrequencyListDescriptor | ETSI EN 300 468, 6.2.17
| FTA_content_management_descriptor | FTAContentManagementDescriptor | ETSI EN 300 468, 6.2.18
| genre_descriptor | GenreDescriptor | ATSC A/65, 6.9.13
| graphics_constraints_descriptor | GraphicsConstraintsDescriptor | ETSI TS 102 809, 5.3.5.8
| green_extension_descriptor | GreenExtensionDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.104
| HEVC_hierarchy_extension_descriptor | HEVCHierarchyExtensionDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.102
| HEVC_timing_and_HRD_descriptor | HEVCTimingAndHRDDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.97
| HEVC_video_descriptor | HEVCVideoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.95
| hierarchical_transmission_descriptor | HierarchicalTransmissionDescriptor | ARIB STD-B10, Part 2, 6.2.22
| hierarchy_descriptor | HierarchyDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.6
| hybrid_information_descriptor | HybridInformationDescriptor | ARIB STD-B10, Part 2, 6.2.58
| IBP_descriptor | IBPDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.34
| image_icon_descriptor | ImageIconDescriptor | ETSI EN 300 468, 6.4.7
| IPMAC_generic_stream_location_descriptor | IPMACGenericStreamLocationDescriptor | ETSI EN 301 192, 8.4.5.15
| IPMAC_platform_name_descriptor | IPMACPlatformNameDescriptor | ETSI EN 301 192, 8.4.5.2
| IPMAC_platform_provider_name_descriptor | IPMACPlatformProviderNameDescriptor | ETSI EN 301 192, 8.4.5.3
| IPMAC_stream_location_descriptor | IPMACStreamLocationDescriptor | ETSI EN 301 192, 8.4.5.14
| ip_signalling_descriptor | IPSignallingDescriptor | ETSI TS 101 812, 10.8.2
| ISDB_access_control_descriptor | ISDBAccessControlDescriptor | ARIB STD-B10, Part 2, 6.2.54
| ISDB_terrestrial_delivery_system_descriptor | ISDBTerrestrialDeliverySystemDescriptor | ARIB STD-B10, Part 2, 6.2.31
| ISO_639_language_descriptor | ISO639LanguageDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.18
| ISP_access_mode_descriptor | ISPAccessModeDescriptor | ETSI EN 301 192, 8.4.5.16
| J2K_video_descriptor | J2KVideoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.80
| linkage_descriptor | LinkageDescriptor | ETSI EN 300 468, 6.2.19
| linkage_descriptor | SSULinkageDescriptor | ETSI EN 300 468, 6.2.19
| local_time_offset_descriptor | LocalTimeOffsetDescriptor | ETSI EN 300 468, 6.2.20
| logo_transmission_descriptor | LogoTransmissionDescriptor | ARIB STD-B10, Part 2, 6.2.44
| maximum_bitrate_descriptor | MaximumBitrateDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.26
| message_descriptor | MessageDescriptor | ETSI EN 300 468, 6.4.7
| metadata_descriptor | MetadataDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.60
| metadata_pointer_descriptor | MetadataPointerDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.58
| metadata_STD_descriptor | MetadataSTDDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.62
| mosaic_descriptor | MosaicDescriptor | ETSI EN 300 468, 6.2.21
| MPEG2_AAC_audio_descriptor | MPEG2AACAudioDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.68
| MPEG2_stereoscopic_video_format_descriptor | MPEG2StereoscopicVideoFormatDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.84
| MPEG4_audio_descriptor | MPEG4AudioDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.38
| MPEG4_video_descriptor | MPEG4VideoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.36
| MPEGH_3D_audio_descriptor | MPEGH3DAudioDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.106
| MPEGH_3D_audio_multi_stream_descriptor | MPEGH3DAudioMultiStreamDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.114
| multilingual_bouquet_name_descriptor | MultilingualBouquetNameDescriptor | ETSI EN 300 468, 6.2.22
| multilingual_component_descriptor | MultilingualComponentDescriptor | ETSI EN 300 468, 6.2.23
| multilingual_network_name_descriptor | MultilingualNetworkNameDescriptor | ETSI EN 300 468, 6.2.24
| multilingual_service_name_descriptor | MultilingualServiceNameDescriptor | ETSI EN 300 468, 6.2.25
| multiplex_buffer_descriptor | MultiplexBufferDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.52
| multiplex_buffer_utilization_descriptor | MultiplexBufferUtilizationDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.22
| MVC_extension_descriptor | MVCExtensionDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.78
| MVC_operation_point_descriptor | MVCOperationPointDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.82
| network_change_notify_descriptor | NetworkChangeNotifyDescriptor | ETSI EN 300 468, 6.4.9
| network_name_descriptor | NetworkNameDescriptor | ETSI EN 300 468, 6.2.27
| node_relation_descriptor | NodeRelationDescriptor | ARIB STD-B10, Part 3, 5.2.3
| nordig_logical_channel_descriptor_v1 | NorDigLogicalChannelDescriptorV1 | NorDig Unified Requirements ver. 3.1.1, 12.2.9.2
| nordig_logical_channel_descriptor_v2 | NorDigLogicalChannelDescriptorV2 | NorDig Unified Requirements ver. 3.1.1, 12.2.9.3
| NPT_endpoint_descriptor | NPTEndpointDescriptor | ISO/IEC 13818-6, 8.1.5
| NPT_reference_descriptor | NPTReferenceDescriptor | ISO/IEC 13818-6, 8.1.1
| NVOD_reference_descriptor | NVODReferenceDescriptor | ETSI EN 300 468, 6.2.26
| parental_rating_descriptor | ParentalRatingDescriptor | ETSI EN 300 468, 6.2.28
| partial_reception_descriptor | PartialReceptionDescriptor | ARIB STD-B10, Part 2, 6.2.32
| partial_transport_stream_descriptor | PartialTransportStreamDescriptor | ETSI EN 300 468, 7.2.1
| PDC_descriptor | PDCDescriptor | ETSI EN 300 468, 6.2.30
| prefetch_descriptor | PrefetchDescriptor | ETSI TS 101 812, 10.8.3.2
| private_data_indicator_descriptor | PrivateDataIndicatorDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.28
| private_data_specifier_descriptor | PrivateDataSpecifierDescriptor | ETSI EN 300 468, 6.2.31
| protection_message_descriptor | ProtectionMessageDescriptor | ETSI TS 102 809, 9.3.3
| redistribution_control_descriptor | RedistributionControlDescriptor | ATSC A/65, 6.9.12
| reference_descriptor | ReferenceDescriptor | ARIB STD-B10, Part 3, 5.2.2
| registration_descriptor | RegistrationDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.8
| related_content_descriptor | RelatedContentDescriptor | ETSI TS 102 323, 10.3
| S2_satellite_delivery_system_descriptor | S2SatelliteDeliverySystemDescriptor | ETSI EN 300 468, 6.2.13.3
| S2X_satellite_delivery_system_descriptor | S2XSatelliteDeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.5
| satellite_delivery_system_descriptor | SatelliteDeliverySystemDescriptor | ETSI EN 300 468, 6.2.13.2
| scheduling_descriptor | SchedulingDescriptor | ETSI TS 102 006, 9.5.2.9
| scrambling_descriptor | ScramblingDescriptor | ETSI EN 300 468, 6.2.32
| series_descriptor | SeriesDescriptor | ARIB STD-B10, Part 2, 6.2.33
| service_availability_descriptor | ServiceAvailabilityDescriptor | ETSI EN 300 468, 6.2.34
| service_descriptor | ServiceDescriptor | ETSI EN 300 468, 6.2.33
| service_group_descriptor | ServiceGroupDescriptor | ARIB STD-B10, Part 2, 6.2.49
| service_identifier_descriptor | ServiceIdentifierDescriptor | ETSI TS 102 809, 6.2.1
| service_list_descriptor | ServiceListDescriptor | ETSI EN 300 468, 6.2.35
| service_location_descriptor | ServiceLocationDescriptor | ATSC A/65, 6.9.5
| service_move_descriptor | ServiceMoveDescriptor | ETSI EN 300 468, 6.2.34
| service_relocated_descriptor | ServiceRelocatedDescriptor | ETSI EN 300 468, 6.4.9
| SH_delivery_system_descriptor | SHDeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.2
| short_event_descriptor | ShortEventDescriptor | ETSI EN 300 468, 6.2.37
| short_node_information_descriptor | ShortNodeInformationDescriptor | ARIB STD-B10, Part 3, 5.2.4
| short_smoothing_buffer_descriptor | ShortSmoothingBufferDescriptor | ETSI EN 300 468, 6.2.38
| simple_application_boundary_descriptor | SimpleApplicationBoundaryDescriptor | ETSI TS 102 809, 5.3.8
| simple_application_location_descriptor | SimpleApplicationLocationDescriptor | ETSI TS 102 809, 5.3.7
| SI_parameter_descriptor | SIParameterDescriptor | ARIB STD-B10, Part 2, 6.2.35
| SI_prime_TS_descriptor | SIPrimeTSDescriptor | ARIB STD-B10, Part 2, 6.2.38
| SL_descriptor | SLDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.42
| smoothing_buffer_descriptor | SmoothingBufferDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.30
| splice_avail_descriptor | SpliceAvailDescriptor | ANSI/SCTE 35, 10.3.1
| splice_DTMF_descriptor | SpliceDTMFDescriptor | ANSI/SCTE 35, 10.3.2
| splice_segmentation_descriptor | SpliceSegmentationDescriptor | ANSI/SCTE 35, 10.3.3
| splice_time_descriptor | SpliceTimeDescriptor | ANSI/SCTE 35, 10.3.4
| SSU_enhanced_message_descriptor | SSUEnhancedMessageDescriptor | ETSI TS 102 006, 9.5.2.14
| SSU_event_name_descriptor | SSUEventNameDescriptor | ETSI TS 102 006, 9.5.2.11
| SSU_location_descriptor | SSULocationDescriptor | ETSI TS 102 006, 9.5.2.7
| SSU_message_descriptor | SSUMessageDescriptor | ETSI TS 102 006, 9.5.2.12
| SSU_subgroup_association_descriptor | SSUSubgroupAssociationDescriptor | ETSI TS 102 006, 9.5.2.8
| SSU_uri_descriptor | SSUURIDescriptor | ETSI TS 102 006, 9.5.2.15
| STC_reference_descriptor | STCReferenceDescriptor | ARIB STD-B10, Part 3, 5.2.5
| STD_descriptor | STDDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.32
| stereoscopic_program_info_descriptor | StereoscopicProgramInfoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.86
| stereoscopic_video_info_descriptor | StereoscopicVideoInfoDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.88
| stream_event_descriptor | StreamEventDescriptor | ISO/IEC 13818-6, 8.3
| stream_identifier_descriptor | StreamIdentifierDescriptor | ETSI EN 300 468, 6.2.39
| stream_mode_descriptor | StreamModeDescriptor | ISO/IEC 13818-6, 8.2
| subtitling_descriptor | SubtitlingDescriptor | ETSI EN 300 468, 6.2.41
| supplementary_audio_descriptor | SupplementaryAudioDescriptor | ETSI EN 300 468, 6.4.11
| SVC_extension_descriptor | SVCExtensionDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.76
| system_clock_descriptor | SystemClockDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.20
| system_management_descriptor | SystemManagementDescriptor | ARIB STD-B10, Part 2, 6.2.21
| T2_delivery_system_descriptor | T2DeliverySystemDescriptor | ETSI EN 300 468, 6.4.6.3
| T2MI_descriptor | T2MIDescriptor | ETSI EN 300 468, 6.4.14
| target_background_grid_descriptor | TargetBackgroundGridDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.12
| target_IP_address_descriptor | TargetIPAddressDescriptor | ETSI EN 301 192, 8.4.5.8
| target_IP_slash_descriptor | TargetIPSlashDescriptor | ETSI EN 301 192, 8.4.5.9
| target_IP_source_slash_descriptor | TargetIPSourceSlashDescriptor | ETSI EN 301 192, 8.4.5.10
| target_IPv6_address_descriptor | TargetIPv6AddressDescriptor | ETSI EN 301 192, 8.4.5.11
| target_IPv6_slash_descriptor | TargetIPv6SlashDescriptor | ETSI EN 301 192, 8.4.5.12
| target_IPv6_source_slash_descriptor | TargetIPv6SourceSlashDescriptor | ETSI EN 301 192, 8.4.5.13
| target_MAC_address_descriptor | TargetMACAddressDescriptor | ETSI EN 301 192, 8.4.5.6
| target_MAC_address_range_descriptor | TargetMACAddressRangeDescriptor | ETSI EN 301 192, 8.4.5.7
| target_region_descriptor | TargetRegionDescriptor | ETSI EN 300 468, 6.4.12
| target_region_name_descriptor | TargetRegionNameDescriptor | ETSI EN 300 468, 6.4.13
| target_serial_number_descriptor | TargetSerialNumberDescriptor | ETSI EN 301 192, 8.4.5.4
| target_smartcard_descriptor | TargetSmartcardDescriptor | ETSI EN 301 192, 8.4.5.5
| telephone_descriptor | TelephoneDescriptor | ETSI EN 300 468, 6.2.42
| teletext_descriptor | TeletextDescriptor | ETSI EN 300 468, 6.2.43
| terrestrial_delivery_system_descriptor | TerrestrialDeliverySystemDescriptor | ETSI EN 300 468, 6.2.13.4
| time_shifted_event_descriptor | TimeShiftedEventDescriptor | ETSI EN 300 468, 6.2.44
| time_slice_fec_identifier_descriptor | TimeSliceFECIdentifierDescriptor | ETSI EN 301 192, 9.5
| transport_profile_descriptor | TransportProfileDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.93
| transport_protocol_descriptor | TransportProtocolDescriptor | ETSI TS 101 812, 10.8.1
| transport_stream_descriptor | TransportStreamDescriptor | ETSI EN 300 468, 6.2.46
| TS_information_descriptor | TSInformationDescriptor | ARIB STD-B10, Part 2, 6.2.42
| TVA_id_descriptor | TVAIdDescriptor | ETSI TS 102 323, 11.2.4
| update_descriptor | UpdateDescriptor | ETSI TS 102 006, 9.5.2.6
| URI_linkage_descriptor | URILinkageDescriptor | ETSI EN 300 468, 6.4.15
| VBI_data_descriptor | VBIDataDescriptor | ETSI EN 300 468, 6.2.47
| VBI_teletext_descriptor | VBITeletextDescriptor | ETSI EN 300 468, 6.2.48
| video_decode_control_descriptor | VideoDecodeControlDescriptor | ARIB STD-B10, Part 2, 6.2.30
| video_depth_range_descriptor | VideoDepthRangeDescriptor | ETSI EN 300 468, 6.4.16
| video_stream_descriptor | VideoStreamDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.2
| video_window_descriptor | VideoWindowDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.14
| virtual_segmentation_descriptor | VirtualSegmentationDescriptor | ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.120
