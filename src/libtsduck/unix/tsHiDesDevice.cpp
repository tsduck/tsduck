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
//
//  An encapsulation of a HiDes modulator device - Unix implementation.
//  Currently, the ITE 950x is implemented on Linux only.
//  On other Unix flavors, this code compiles but no device wil be found.
//
//----------------------------------------------------------------------------

#include "tsHiDesDevice.h"
#include "tsNullReport.h"
#include "tsMemoryUtils.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Type definitions from HiDes / ITE.
//----------------------------------------------------------------------------

// WARNING: There are INCONSISTENCIES between the int types and the associated
// comment. Typically, the size of a 'long' depends on the platform (32 vs.
// 64 bits). And a 'long long' is often 64-bit on 32-bit platforms despite
// the comment (32 bits). So, there is a bug somewhere:
// - Either the definitions are correct and consistently used in the driver
//   code and application code. And the comments are incorrect.
// - Or the comments are correct and the definitions are broken on some
//   platforms. Extensive testing is required on 32 and 64-bit platforms.

typedef void* Handle;

// 8-bits unsigned type.
typedef unsigned char Byte;

// 16-bits unsigned type.
typedef unsigned short Word;

// 32-bits unsigned type <== incorrect on x86_64
typedef unsigned long Dword;

// 32-bits unsigned type <== incorrect everywhere
typedef unsigned long long ULONGLONG;

// 16-bits signed type.
typedef short Short;

// 32-bits signed type <== incorrect on x86_64
typedef long Long;

/* Use 'k' as magic number */
#define AFA_IOC_MAGIC  'k'

typedef enum {
    False = 0,
    True  = 1
} Bool;

typedef struct {
    Dword errorCount;
    Dword snr;
    double errorRate;
} SnrTable;

typedef struct {
    double doSetVolt;
    double doPuUpVolt;
} AgcVoltage;

typedef struct {
    Dword frequency;
    int  dAmp;
    int  dPhi;
} IQtable;

typedef struct {
    IQtable *ptrIQtableEx;
    Word    tableGroups;        //Number of IQtable groups;
    Dword   tableVersion;       //tableVersion;
    int     outputGain;
    Word    c1DefaultValue;
    Word    c2DefaultValue;
    Word    c3DefaultValue;
} CalibrationInfo;

typedef struct {
    Dword startFrequency;
    int  i;
    int  q;
} DCtable;

typedef struct {
    DCtable *ptrDCtable;
    DCtable *ptrOFStable;
    Word    tableGroups;        //Number of IQtable groups;
} DCInfo;

typedef enum {
    Polarity_NORMAL = 0,
    Polarity_INVERSE
} Polarity;

typedef enum {
    Processor_LINK = 0,
    Processor_OFDM = 8
} Processor;

typedef enum {
    Product_GANYMEDE = 0,
    Product_JUPITER,
    Product_GEMINI,
} Product;

typedef enum {
    BurstSize_1024 = 0,
    BurstSize_2048,
    BurstSize_4096
} BurstSize;

typedef struct {
    Byte segmentType;           /** 0:Firmware download 1:Rom copy 2:Direct command */
    Dword segmentLength;
} Segment;

typedef enum {
    Bandwidth_6M = 0,           /** Signal bandwidth is 6MHz */
    Bandwidth_7M,               /** Signal bandwidth is 7MHz */
    Bandwidth_8M,               /** Signal bandwidth is 8MHz */
    Bandwidth_5M                /** Signal bandwidth is 5MHz */
} Bandwidth;

typedef enum {
    Mode_QPSK = 0,
    Mode_16QAM,
    Mode_64QAM
} Mode;

typedef enum {
    Fft_2K = 0,
    Fft_8K = 1,
    Fft_4K = 2
} Fft;

typedef enum {
    Interval_1_OVER_32 = 0,     /** Guard interval is 1/32 of symbol length */
    Interval_1_OVER_16,         /** Guard interval is 1/16 of symbol length */
    Interval_1_OVER_8,          /** Guard interval is 1/8 of symbol length  */
    Interval_1_OVER_4           /** Guard interval is 1/4 of symbol length  */
} Interval;

typedef enum {
    Priority_HIGH = 0,          /** DVB-T - identifies high-priority stream */
    Priority_LOW                /** DVB-T - identifies low-priority stream  */
} Priority;             // High Priority or Low Priority

typedef enum {
    CodeRate_1_OVER_2 = 0,      /** Signal uses FEC coding ratio of 1/2 */
    CodeRate_2_OVER_3,          /** Signal uses FEC coding ratio of 2/3 */
    CodeRate_3_OVER_4,          /** Signal uses FEC coding ratio of 3/4 */
    CodeRate_5_OVER_6,          /** Signal uses FEC coding ratio of 5/6 */
    CodeRate_7_OVER_8,          /** Signal uses FEC coding ratio of 7/8 */
    CodeRate_NONE               /** None, NXT doesn't have this one     */
} CodeRate;

typedef enum {
    Hierarchy_NONE = 0,         /** Signal is non-hierarchical        */
    Hierarchy_ALPHA_1,          /** Signalling format uses alpha of 1 */
    Hierarchy_ALPHA_2,          /** Signalling format uses alpha of 2 */
    Hierarchy_ALPHA_4           /** Signalling format uses alpha of 4 */
} Hierarchy;

typedef enum {
    SubchannelType_AUDIO = 0,
    SubchannelType_VIDEO = 1,
    SubchannelType_PACKET = 3,
    SubchannelType_ENHANCEPACKET = 4
} SubchannelType;

typedef enum {
    ProtectionLevel_NONE = 0x00,
    ProtectionLevel_PL1 = 0x01,
    ProtectionLevel_PL2 = 0x02,
    ProtectionLevel_PL3 = 0x03,
    ProtectionLevel_PL4 = 0x04,
    ProtectionLevel_PL5 = 0x05,
    ProtectionLevel_PL1A = 0x1A,
    ProtectionLevel_PL2A = 0x2A,
    ProtectionLevel_PL3A = 0x3A,
    ProtectionLevel_PL4A = 0x4A,
    ProtectionLevel_PL1B = 0x1B,
    ProtectionLevel_PL2B = 0x2B,
    ProtectionLevel_PL3B = 0x3B,
    ProtectionLevel_PL4B = 0x4B
} ProtectionLevel;

typedef struct {
    Dword frequency;
    Mode mode;
    Fft fft;
    Interval interval;
    Priority priority;
    CodeRate highCodeRate;
    CodeRate lowCodeRate;
    Hierarchy hierarchy;
    Bandwidth bandwidth;
} ChannelModulation;

typedef struct {
    Byte subchannelId;
    Word subchannelSize;
    Word bitRate;
    Byte transmissionMode;				// transmissionMode = 1, 2, 3, 4
    ProtectionLevel protectionLevel;
    SubchannelType subchannelType;
    Byte conditionalAccess;
    Byte tiiPrimary;
    Byte tiiCombination;
} SubchannelModulation;

typedef enum {
    IpVersion_IPV4 = 0,
    IpVersion_IPV6 = 1
} IpVersion;

typedef struct {
    IpVersion version;
    Priority priority;
    Bool cache;
    Byte address[16];
} Ip;

typedef struct {
    Dword platformId;
    char iso639LanguageCode[3];
    Byte platformNameLength;
    char platformName[32];
    Word bandwidth;
    Dword frequency;
    Byte* information;
    Word informationLength;
    Bool hasInformation;
    IpVersion ipVersion;
} Platform;

typedef struct
{
    Byte charSet;
    Word charFlag;
    Byte string[16];
} Label;

typedef struct
{
    Word ensembleId;
    Label ensembleLabel;
    Byte totalServices;
} Ensemble;

typedef struct {
    Byte serviceType;		// Service Type(P/D): 0x00: Program, 0x80: Data
    Dword serviceId;
    Dword frequency;
    Label serviceLabel;
    Byte totalComponents;
} Service;

typedef struct {
    Byte serviceType;			// Service Type(P/D): 0x00: Program, 0x80: Data
    Dword serviceId;			// Service ID
    Word componentId;			// Stream audio/data is subchid, packet mode is SCId
    Byte componentIdService;	// Component ID within Service
    Label componentLabel;
    Byte language;				// Language code
    Byte primary;				// Primary/Secondary
    Byte conditionalAccess;		// Conditional Access flag
    Byte componentType;			// Component Type (A/D)
    Byte transmissionId;		// Transmission Mechanism ID
} Component;

typedef enum {
    SectionType_MPE = 0,
    SectionType_SIPSI,
    SectionType_TABLE
} SectionType;

typedef enum {
    FrameRow_256 = 0,
    FrameRow_512,
    FrameRow_768,
    FrameRow_1024
} FrameRow;

/*
 * In DVB-T mode, only value is valid. In DVB-H mode,
 * as sectionType = SectionType_SIPSI: only value is valid.
 * as sectionType = SectionType_TABLE: both value and table is valid.
 * as sectionType = SectionType_MPE: except table all other fields is valid.
 */
typedef struct {
    Byte table;                 /** The table ID. Which is used to filter specific SI/PSI table.                                  */
    Byte duration;              /** The maximum burst duration. It can be specify to 0xFF if user don't know the exact value.     */
    FrameRow frameRow;          /** The frame row of MPE-FEC. It means the exact number of rows for each column in MPE-FEC frame. */
    SectionType sectionType;    /** The section type of pid. See the defination of SectionType.                                   */
    Priority priority;          /** The priority of MPE data. Only valid when sectionType is set to SectionType_MPE.              */
    IpVersion version;          /** The IP version of MPE data. Only valid when sectionType is set to SectionType_MPE.            */
    Bool cache;                 /** True: MPE data will be cached in device's buffer. Fasle: MPE will be transfer to host.        */
    Word value;                 /** The 13 bits Packet ID.                                                                        */
} Pid;

typedef struct {
    Dword address;      /** The address of target register */
    Byte value;         /** The value of target register   */
} ValueSet;

typedef struct {
    Dword address;
    Byte length;
    Byte* value;
} MultiValueSet;

typedef struct {
    Dword mjd;
    Byte configuration;
    Byte hours;
    Byte minutes;
    Byte seconds;
    Word milliseconds;
} Datetime;

typedef struct _TPS{
    Byte highCodeRate;
    Byte lowCodeRate;
    Byte transmissionMode;
    Byte constellation;
    Byte interval;
    Word cellid;
} TPS, *pTPS;

typedef struct {
    Word pid[32];
} PidTable;

typedef struct {
    Product product;
    Handle userData;
    Handle driver;
} Demodulator;

typedef struct {
    Bool signalPresented;       // Signal is presented.
    Bool signalLocked;          // Signal is locked.
    Byte signalQuality;         // Signal quality, from 0 (poor) to 100 (good).
    Byte signalStrength;        // Signal strength from 0 (weak) to 100 (strong).
    Byte frameErrorRatio;       // Frame Error Ratio (error ratio before MPE-FEC) = frameErrorRate / 128
    Byte mpefecFrameErrorRatio; // MPE-FEC Frame Error Ratio (error ratio after MPE-FEC) = mpefecFrameErrorCount / 128
} Statistic;

typedef struct {
    Word abortCount;
    Dword postVitBitCount;
    Dword postVitErrorCount;
#ifdef User_FLOATING_POINT
    Dword softBitCount;
    Dword softErrorCount;
    Dword preVitBitCount;
    Dword preVitErrorCount;
    double snr;
#endif
} ChannelStatistic;

typedef struct {
    Word abortCount;
    Dword postVitBitCount;
    Dword postVitErrorCount;
    Word ficCount;              // Total FIC error count
    Word ficErrorCount;         // Total FIC count
#ifdef User_FLOATING_POINT
    Dword softBitCount;
    Dword softErrorCount;
    Dword preVitBitCount;
    Dword preVitErrorCount;
    double snr;
#endif
} SubchannelStatistic;

#ifdef User_FLOATING_POINT
typedef struct {
    double doSetVolt;
    double doPuUpVolt;
} AgcVoltage;
#endif

typedef enum {
    Constellation_QPSK = 0,     /** Signal uses QPSK constellation  */
    Constellation_16QAM,        /** Signal uses 16QAM constellation */
    Constellation_64QAM         /** Signal uses 64QAM constellation */
} Constellation;

typedef enum {
    ARIB_STD_B31 = 0,   // System based on this specification
    ISDB_TSB            // System for ISDB-TSB
} SystemIdentification;

typedef struct {
    Constellation   constellation;        /** Constellation scheme (FFT mode) in use                   */
    CodeRate        codeRate;             /** FEC coding ratio of high-priority stream                 */
} TMCC;

typedef struct _TMCCINFO{
    TMCC                    layerA;
    TMCC                    layerB;
    Bool                    isPartialReception;
    SystemIdentification    systemIdentification;
} TMCCINFO, *pTMCCINFO;

typedef enum {
   filter = 0,
   LayerB = 1,
   LayerA = 2,
   LayerAB = 3
} TransportLayer;

typedef enum {
    DownSampleRate_21_OVER_1 = 0,      /** Signal uses FEC coding ratio of 21/1 */
    DownSampleRate_21_OVER_2,          /** Signal uses FEC coding ratio of 21/2 */
    DownSampleRate_21_OVER_3,          /** Signal uses FEC coding ratio of 21/3 */
    DownSampleRate_21_OVER_4,          /** Signal uses FEC coding ratio of 21/4 */
    DownSampleRate_21_OVER_5,          /** Signal uses FEC coding ratio of 21/5 */
    DownSampleRate_21_OVER_6,          /** Signal uses FEC coding ratio of 21/6 */
} DownSampleRate;

typedef enum {
    TransmissionMode_2K = 0,    /** OFDM frame consists of 2048 different carriers (2K FFT mode) */
    TransmissionMode_8K = 1,    /** OFDM frame consists of 8192 different carriers (8K FFT mode) */
    TransmissionMode_4K = 2     /** OFDM frame consists of 4096 different carriers (4K FFT mode) */
} TransmissionModes;

typedef struct {
    Dword       frequency;                /** Channel frequency in KHz.                                */
    Bandwidth   bandwidth;
    TransmissionModes transmissionMode; /** Number of carriers used for OFDM signal                  */
    Interval    interval;                 /** Fraction of symbol length used as guard (Guard Interval) */
    //DownSampleRate ds;
    TMCC        layerA;
    TMCC        layerB;
    Bool        isPartialReception;
} ISDBTModulation;

typedef enum {
    PcrModeDisable = 0,
    PcrMode1 = 1,
    PcrMode2,
    PcrMode3
} PcrMode;

typedef struct {
    Byte            chip;
    Processor       processor;
    __u32           registerAddress;
    Byte            bufferLength;
    Byte            buffer[256];
    __u32           error;
    Byte            reserved[16];
} WriteRegistersRequest, *PWriteRegistersRequest;

typedef struct {
    Byte            chip;
    Processor       processor;
    __u32           registerAddress;
    Byte            bufferLength;
    Byte            buffer[256];
    __u32           error;
    Byte            reserved[16];
} TxWriteRegistersRequest, *PTxWriteRegistersRequest;

typedef struct {
    Byte            chip;
    Word            registerAddress;
    Byte            bufferLength;
    Byte            buffer[256];
    __u32           error;
    Byte            reserved[16];
} TxWriteEepromValuesRequest, *PTxWriteEepromValuesRequest;

typedef struct {
    Byte            chip;
    Processor       processor;
    __u32           registerAddress;
    Byte            bufferLength;
    Byte            buffer[256];
    __u32           error;
    Byte            reserved[16];
} ReadRegistersRequest, *PReadRegistersRequest;

typedef struct {
    Byte            chip;
    Processor       processor;
    __u32           registerAddress;
    Byte            bufferLength;
    Byte            buffer[256];
    __u32           error;
    Byte            reserved[16];
} TxReadRegistersRequest, *PTxReadRegistersRequest;

typedef struct {
    Byte            chip;
    Word            registerAddress;
    Byte            bufferLength;
    Byte            buffer[256];
    __u32           error;
    Byte            reserved[16];
} TxReadEepromValuesRequest, *PTxReadEepromValuesRequest;

typedef struct {
    Byte                chip;
    Word                bandwidth;
    __u32               frequency;
    __u32               error;
    Byte                reserved[16];
} AcquireChannelRequest, *PAcquireChannelRequest;

typedef struct {
    Byte                chip;
    Byte                transmissionMode;
    Byte                constellation;
    Byte                interval;
    Byte                highCodeRate;
    __u32               error;
    Byte                reserved[16];
} TxSetModuleRequest, *PTxSetModuleRequest;

typedef struct {
    Byte                chip;
    Word                bandwidth;
    __u32               frequency;
    __u32               error;
    Byte                reserved[16];
} TxAcquireChannelRequest, *PTxAcquireChannelRequest;

typedef struct {
    Byte                OnOff;
    __u32               error;
    Byte                reserved[16];
} TxModeRequest, *PTxModeRequest;

typedef struct {
    Byte                DeviceType;
    __u32               error;
    Byte                reserved[16];
} TxSetDeviceTypeRequest, *PTxSetDeviceTypeRequest;

typedef struct {
    Byte                DeviceType;
    __u32               error;
    Byte                reserved[16];
} TxGetDeviceTypeRequest, *PTxGetDeviceTypeRequest;

typedef struct {
    int             GainValue;
    __u32           error;
} TxSetGainRequest, *PTxSetGainRequest;

typedef struct {
    Byte                chip;
    Bool                locked;
    Dword               error;
    Byte                reserved[16];
} IsLockedRequest, *PIsLockedRequest;

typedef struct {
    Byte*               platformLength;
    Platform*           platforms;
    Dword               error;
    Byte                reserved[16];
} AcquirePlatformRequest, *PAcquirePlatformRequest;

typedef struct {
    Byte                chip;
    Byte                index;
    Pid                 pid;
    __u32               error;
    Byte                reserved[16];
} AddPidAtRequest, *PAddPidAtRequest;

typedef struct {
    Byte                chip;
    Byte                index;
    Pid                 pid;
    __u32               error;
    Byte                reserved[16];
} TxAddPidAtRequest, *PTxAddPidAtRequest;

typedef struct {
    Byte            chip;
    __u32           error;
    Byte            reserved[16];
} ResetPidRequest, *PResetPidRequest;

typedef struct {
    Byte            chip;
    __u32           error;
    Byte            reserved[16];
} TxResetPidRequest, *PTxResetPidRequest;

typedef struct {
    Byte                chip;
    __u32               channelStatisticAddr;       // ChannelStatistic*
    __u32               error;
    Byte                reserved[16];
} GetChannelStatisticRequest, *PGetChannelStatisticRequest;

typedef struct {
    Byte                chip;
    Statistic           statistic;
    __u32               error;
    Byte                reserved[16];
} GetStatisticRequest, *PGetStatisticRequest;

typedef struct {
    Byte            chip;
    Byte            control;
    __u32           error;
    Byte            reserved[16];
} ControlPidFilterRequest, *PControlPidFilterRequest;

typedef struct {
    Byte            control;
    Byte            enable;
    __u32           error;
    Byte            reserved[16];
} TxControlPidFilterRequest, *PTxControlPidFilterRequest;

typedef struct {
    Byte                chip;
    Byte                control;
    __u32               error;
    Byte                reserved[16];
} ControlPowerSavingRequest, *PControlPowerSavingRequest;

typedef struct {
    Byte                chip;
    Byte                control;
    __u32               error;
    Byte                reserved[16];
} TxControlPowerSavingRequest, *PTxControlPowerSavingRequest;

typedef struct {
    Byte                DriverVerion[16];   /** XX.XX.XX.XX Ex., 1.2.3.4            */
    Byte                APIVerion[32];      /** XX.XX.XXXXXXXX.XX Ex., 1.2.3.4  */
    Byte                FWVerionLink[16];   /** XX.XX.XX.XX Ex., 1.2.3.4            */
    Byte                FWVerionOFDM[16];   /** XX.XX.XX.XX Ex., 1.2.3.4            */
    Byte                DateTime[24];       /** Ex.,"2004-12-20 18:30:00" or "DEC 20 2004 10:22:10" with compiler __DATE__ and __TIME__  definitions */
    Byte                Company[8];         /** Ex.,"ITEtech"                   */
    Byte                SupportHWInfo[32];  /** Ex.,"Jupiter DVBT/DVBH"         */
    __u32               error;
    Byte                reserved[128];
} DemodDriverInfo, *PDemodDriverInfo;

typedef struct {
    Byte                DriverVerion[16];   /** XX.XX.XX.XX Ex., 1.2.3.4            */
    Byte                APIVerion[32];      /** XX.XX.XXXXXXXX.XX Ex., 1.2.3.4  */
    Byte                FWVerionLink[16];   /** XX.XX.XX.XX Ex., 1.2.3.4            */
    Byte                FWVerionOFDM[16];   /** XX.XX.XX.XX Ex., 1.2.3.4            */
    Byte                DateTime[24];       /** Ex.,"2004-12-20 18:30:00" or "DEC 20 2004 10:22:10" with compiler __DATE__ and __TIME__  definitions */
    Byte                Company[8];         /** Ex.,"ITEtech"                   */
    Byte                SupportHWInfo[32];  /** Ex.,"Jupiter DVBT/DVBH"         */
    __u32               error;
    Byte                reserved[128];
} TxModDriverInfo, *PTxModDriverInfo;

/**
 * Demodulator Stream control API commands
 */
typedef struct {
    Byte            chip;
    Dword           error;
    Byte            reserved[16];
} StartCaptureRequest, *PStartCaptureRequest;

typedef struct {
    Byte            chip;
    Dword           error;
    Byte            reserved[16];
} TxStartTransferRequest, *PTxStartTransferRequest;

typedef struct {
    Byte            chip;
    Dword           error;
    Byte            reserved[16];
} TxStopTransferRequest, *PTxStopTransferRequest;

typedef struct {
    Byte            chip;
    Dword           error;
    Byte            reserved[16];
} StopCaptureRequest, *PStopCaptureRequest;

typedef struct {
    __u32           len;
    __u32           cmdAddr;        // Byte*
    __u32           error;
    Byte            reserved[16];
} TxCmdRequest, *PTxCmdRequest;

typedef struct {
    __u32           error;
    __u32          frequency;
    Word           bandwidth;
    int             maxGain;
    int             minGain;
    Byte            reserved[16];
} TxGetGainRangeRequest, *PTxGetGainRangeRequest;

typedef struct {
    TPS             tps;
    __u32           error;
    Byte            reserved[16];
} TxGetTPSRequest, *PTxGetTPSRequest;

typedef struct {
    TPS            tps;
    Bool           actualInfo;
    __u32          error;
    Byte           reserved[16];
} TxSetTPSRequest, *PTxSetTPSRequest;

typedef struct {
    int            gain;
    __u32          error;
    Byte           reserved[16];
} TxGetOutputGainRequest, *PTxGetOutputGainRequest;

typedef struct {
    __u32          error;
    __u32         pbufferAddr;
    Byte           reserved[16];
} TxSendHwPSITableRequest, *PTxSendHwPSITableRequest;

typedef struct {
    Byte           psiTableIndex;
    __u32          pbufferAddr;
    __u32          error;
    Byte           reserved[16];
} TxAccessFwPSITableRequest, *PTxAccessFwPSITableRequest;

typedef struct {
    Byte            psiTableIndex;
    Word            timer;
    __u32           error;
    Byte            reserved[16];
} TxSetFwPSITableTimerRequest, *PTxSetFwPSITableTimerRequest;

typedef struct {
    __u32               pBufferAddr;            // Byte*
    __u32               pdwBufferLength;
    __u32               error;
    Byte                reserved[16];
} TxSetLowBitRateTransferRequest, *PTxSetLowBitRateTransferRequest;

typedef struct {
    __u32               pIQtableAddr;       // Byte*
    Word                IQtableSize;
    __u32               error;
    Byte                reserved[16];
} TxSetIQTableRequest, *PTxSetIQTableRequest;

typedef struct {
    int                 dc_i;
    int                 dc_q;
    __u32               error;
    Byte                reserved[16];
} TxSetDCCalibrationValueRequest, *PTxSetDCCalibrationValueRequest;

typedef struct {
    Word            chipType;
    __u32           error;
    Byte            reserved[16];
} TxGetChipTypeRequest, *PTxGetChipTypeRequest;

typedef struct {
    __u32               isdbtModulationAddr;    //  ISDBTModulation
    __u32               error;
    Byte                reserved[16];
} TXSetISDBTChannelModulationRequest, *PTXSetISDBTChannelModulationRequest;

typedef struct {
    TMCCINFO            TmccInfo;
    Bool                actualInfo;
    __u32               error;
    Byte                reserved[16];
} TXSetTMCCInfoRequest, *PTXSetTMCCInfoRequest;

typedef struct {
    TMCCINFO            TmccInfo;
    __u32               error;
    Byte                reserved[16];
} TXGetTMCCInfoRequest, *PTXGetTMCCInfoRequest;

typedef struct {
    Word                BitRate_Kbps;
    __u32               error;
    Byte                reserved[16];
} TXGetTSinputBitRateRequest, *PTXGetTSinputBitRateRequest;

typedef struct {
    Byte                index;
    Pid                 pid;
    TransportLayer      layer;
    __u32               error;
    Byte                reserved[16];
} TXAddPidToISDBTPidFilterRequest, *PTXAddPidToISDBTPidFilterRequest;

typedef struct {
    PcrMode         mode;
    __u32               error;
    Byte                reserved[16];
} TxSetPcrModeRequest, *PTxSetPcrModeRequest;

typedef struct {
    __u32               DCInfoAddr; //DCInfo*
    __u32               error;
    Byte                reserved[16];
} TxSetDCTableRequest, *PTxSetDCTableRequest;

typedef struct {
    Byte                frequencyindex;
    __u32               error;
    Byte                reserved[16];
} TxGetFrequencyIndexRequest, *PTxGetFrequencyIndexRequest;

typedef struct {
    Byte            DTVMode;
    __u32           error;
    Byte            reserved[16];
} TxGetDTVModeRequest, *PTxGetDTVModeRequest;

typedef struct {
    __u32           key ;
    __u32           error;
    Byte            reserved[16];
} TxEnableTpsEncryptionRequest, *PTxEnableTpsEncryptionRequest;

typedef struct {
    __u32           error;
    Byte            reserved[16];
} TxDisableTpsEncryptionRequest, *PTxDisableTpsEncryptionRequest;

typedef struct {
    __u32           decryptKey;
    Byte            decryptEnable;
    __u32           error;
    Byte            reserved[16];
} TxSetDecryptRequest, *PTxSetDecryptRequest;

typedef struct {
    Bool            isInversion;
    __u32           error;
    Byte            reserved[16];
} TxSetSpectralInversionRequest, *PTxSetSpectralInversionRequest;

/**
 * Modulator & Demodulator API commands
 */
#define IOCTRL_ITE_GROUP_STANDARD           0x000
#define IOCTRL_ITE_GROUP_DVBT               0x100
#define IOCTRL_ITE_GROUP_DVBH               0x200
#define IOCTRL_ITE_GROUP_FM                 0x300
#define IOCTRL_ITE_GROUP_TDMB               0x400
#define IOCTRL_ITE_GROUP_OTHER              0x500
#define IOCTRL_ITE_GROUP_ISDBT              0x600
#define IOCTRL_ITE_GROUP_SECURITY           0x700

/* STANDARD */

/**
 * Write a sequence of bytes to the contiguous registers in demodulator.
 * Paramters:   WriteRegistersRequest struct
 */
#define IOCTL_ITE_DEMOD_WRITEREGISTERS \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x00, WriteRegistersRequest)

/**
 * Read a sequence of bytes from the contiguous registers in demodulator.
 * Paramters:   ReadRegistersRequest struct
 */
#define IOCTL_ITE_DEMOD_READREGISTERS \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x06, ReadRegistersRequest)

/**
 * Specify the bandwidth of channel and tune the channel to the specific
 * frequency. Afterwards, host could use output parameter dvbH to determine
 * if there is a DVB-H signal.
 * In DVB-T mode, after calling this function output parameter dvbH should
 * be False and host could use output parameter "locked" to indicate if the
 * TS is correct.
 * In DVB-H mode, after calling this function output parameter dvbH should
 * be True and host could use Jupiter_acquirePlatorm to get platform.
 * Paramters:   AcquireChannelRequest struct
 */
#define IOCTL_ITE_DEMOD_ACQUIRECHANNEL \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x14, AcquireChannelRequest)

/**
 * Get all the platforms found in current frequency.
 * Paramters:   IsLockedRequest struct
 */
#define IOCTL_ITE_DEMOD_ISLOCKED \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x15, IsLockedRequest)

/**
 * Get the statistic values of demodulator, it includes Pre-Viterbi BER,
 * Post-Viterbi BER, Abort Count, Signal Presented Flag, Signal Locked Flag,
 * Signal Quality, Signal Strength, Delta-T for DVB-H time slicing.
 * Paramters:   GetStatisticRequest struct
 */
#define IOCTL_ITE_DEMOD_GETSTATISTIC \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x18, GetStatisticRequest)

/**
 * Get the statistic values of demodulator, it includes Pre-Viterbi BER,
 * Post-Viterbi BER, Abort Count, Signal Presented Flag, Signal Locked Flag,
 * Signal Quality, Signal Strength, Delta-T for DVB-H time slicing.
 * Paramters:   GetChannelStatisticRequest struct
 */
#define IOCTL_ITE_DEMOD_GETCHANNELSTATISTIC \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x19, GetChannelStatisticRequest)

/**
 * Paramters:   ControlPowerSavingRequest struct
 */
#define IOCTL_ITE_DEMOD_CONTROLPOWERSAVING \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x1E, ControlPowerSavingRequest)

/**
 * Modulator Set Modulation.
 * Paramters:   TxSetModuleRequest struct
 */
#define IOCTL_ITE_MOD_SETMODULE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x21, TxSetModuleRequest)

/**
 * Modulator Acquire Channel.
 * Paramters:   TxAcquireChannelRequest struct
 */
#define IOCTL_ITE_MOD_ACQUIRECHANNEL \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x22, TxAcquireChannelRequest)

/**
 * Modulator Null Packet Enable.
 * Paramters:   TxModeRequest struct
 */
#define IOCTL_ITE_MOD_ENABLETXMODE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x23, TxModeRequest)

/**
 * Read a sequence of bytes from the contiguous registers in demodulator.
 * Paramters:   ReadRegistersRequest struct
 */
#define IOCTL_ITE_MOD_READREGISTERS \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x24, TxReadRegistersRequest)

/**
 * Write a sequence of bytes to the contiguous registers in demodulator.
 * Paramters:   TxWriteRegistersRequest struct
 */
#define IOCTL_ITE_MOD_WRITEREGISTERS \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x27, TxWriteRegistersRequest)

/**
 * Modulator Device Type Setting.
 * Paramters:   TxSetDeviceTypeRequest struct
 */
#define IOCTL_ITE_MOD_SETDEVICETYPE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x28, TxSetDeviceTypeRequest)

/**
 * Modulator Device Type Getting.
 * Paramters:   TxGetDeviceTypeRequest struct
 */
#define IOCTL_ITE_MOD_GETDEVICETYPE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x29, TxGetDeviceTypeRequest)

/**
 * Modulator Set Gain Range.
 * Paramters:   TxSetGainRequest struct
 */
#define IOCTL_ITE_MOD_ADJUSTOUTPUTGAIN \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2B, TxSetGainRequest)

/**
 * Modulator Get Gain Range.
 * Paramters:   TxGetGainRangeRequest struct
 */
#define IOCTL_ITE_MOD_GETGAINRANGE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2C, TxGetGainRangeRequest)

/**
 * Modulator Get Output Gain Range.
 * Paramters:   TxGetOutputGainRangeRequest struct
 */
#define IOCTL_ITE_MOD_GETOUTPUTGAIN \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2D, TxGetOutputGainRequest)

/**
 * Paramters:   TxControlPowerSavingRequest struct
 */
#define IOCTL_ITE_MOD_CONTROLPOWERSAVING \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2F, TxControlPowerSavingRequest)

/**
 * Write a sequence of bytes to the contiguous cells in the EEPROM.
 * Paramters:   WriteEepromValuesRequest struct
 */
#define IOCTL_ITE_MOD_WRITEEEPROMVALUES \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x31, TxWriteEepromValuesRequest)

/**
 * Read a sequence of bytes from the contiguous cells in the EEPROM.
 * Paramters:   ReadEepromValuesRequest struct
 */
#define IOCTL_ITE_MOD_READEEPROMVALUES \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x32, TxReadEepromValuesRequest)

/**
 * Get Chip Type IT9507/IT9503 in modulator.
 * Paramters:   TxGetChipTypeRequest struct
 */
#define IOCTL_ITE_MOD_GETCHIPTYPE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x3B, TxGetChipTypeRequest)

/**
 * Get Chip Type IT9507/IT9503 in modulator.
 * Paramters:   TxSetSpectralInversion struct
 */
#define IOCTL_ITE_MOD_SETSPECTRALINVERSION \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x3C, TxSetSpectralInversionRequest)

/* DVBT */

/**
 * Reset PID from PID filter.
 * Paramters:   ResetPidRequest struct
 */
#define IOCTL_ITE_DEMOD_RESETPID \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x02, ResetPidRequest)

/**
 * Enable PID filter.
 * Paramters:   ControlPidFilterRequest struct
 */
#define IOCTL_ITE_DEMOD_CONTROLPIDFILTER \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x03, ControlPidFilterRequest)

/**
 * Add PID to PID filter.
 * Paramters:   AddPidAtRequest struct
 */
#define IOCTL_ITE_DEMOD_ADDPIDAT \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x04, AddPidAtRequest)

/**
 * Add PID to PID filter.
 * Paramters:   AddPidAtRequest struct
 */
#define IOCTL_ITE_MOD_ADDPIDAT \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x08, TxAddPidAtRequest)

/**
 * Reset PID from PID filter.
 * Paramters:   ResetPidRequest struct
 */
#define IOCTL_ITE_MOD_RESETPID \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x10, TxResetPidRequest)

/**
 * Enable PID filter.
 * Paramters:   TxControlPidFilterRequest struct
 */
#define IOCTL_ITE_MOD_CONTROLPIDFILTER \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x11, TxControlPidFilterRequest)

/**
 * Enable Set IQTable From File.
 * Paramters:   TxSetIQTableRequest struct
 */
#define IOCTL_ITE_MOD_SETIQTABLE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x12, TxSetIQTableRequest)

/**
 * Enable Set DC Calibration Value From File.
 * Paramters:   TxSetDCCalibrationValueRequest struct
 */
#define IOCTL_ITE_MOD_SETDCCALIBRATIONVALUE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x13, TxSetDCCalibrationValueRequest)

/* OTHER */

/**
 * Get driver information.
 * Paramters:   DemodDriverInfo struct
 */
#define IOCTL_ITE_DEMOD_GETDRIVERINFO \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x00, DemodDriverInfo)

/**
 * Start capture data stream
 * Paramters: StartCaptureRequest struct
 */
#define IOCTL_ITE_DEMOD_STARTCAPTURE \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x01, StartCaptureRequest)

/**
 * Stop capture data stream
 * Paramters: StopCaptureRequest struct
 */
#define IOCTL_ITE_DEMOD_STOPCAPTURE \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x02, StopCaptureRequest)

/**
 * Start Transfer data stream
 * Paramters: StartTransferRequest struct
 */
#define IOCTL_ITE_MOD_STARTTRANSFER \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x07, TxStartTransferRequest)

/**
 * Stop capture data stream
 * Paramters: StopTransferRequest struct
 */
#define IOCTL_ITE_MOD_STOPTRANSFER \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x08, TxStopTransferRequest)

/**
 * Modulator: Get Driver information.
 * Paramters: TxModDriverInfo struct
 */
#define IOCTL_ITE_MOD_GETDRIVERINFO \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x09, TxModDriverInfo)

/**
 * Modulator: Set Start Transfer data Streaming.
 * Paramters: StopTransferRequest struct
 */
#define IOCTL_ITE_MOD_STARTTRANSFER_CMD \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0A, TxStartTransferRequest)

/**
 * Modulator: Set Stop Transfer data Streaming.
 * Paramters: TxStopTransferRequest struct
 */
#define IOCTL_ITE_MOD_STOPTRANSFER_CMD \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0B, TxStopTransferRequest)

/**
 * Modulator: Set Command.
 * Paramters: TxCmdRequest struct
 */
#define IOCTL_ITE_MOD_WRITE_CMD \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0C, TxCmdRequest)

/**
 * Modulator: Get TPS.
 * Paramters: TxGetTPSRequest struct
 */
#define IOCTL_ITE_MOD_GETTPS \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0D, TxGetTPSRequest)

/**
 * Modulator: Set TPS.
 * Paramters: TxSetTPSRequest struct
 */
#define IOCTL_ITE_MOD_SETTPS \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0E, TxSetTPSRequest)

/**
 * Modulator: Send PSI Table to Hardware.
 * Paramters: TxSetTPSRequest struct
 */
#define IOCTL_ITE_MOD_SENDHWPSITABLE \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0F, TxSendHwPSITableRequest)

/**
 * Modulator: Access PSI Table to firmware.
 * Paramters: TxSetTPSRequest struct
 */
#define IOCTL_ITE_MOD_ACCESSFWPSITABLE \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x10, TxAccessFwPSITableRequest)

/**
 * Modulator: Access PSI Table to firmware.
 * Paramters: TxSetTPSRequest struct
 */
#define IOCTL_ITE_MOD_SETFWPSITABLETIMER \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x11, TxSetFwPSITableTimerRequest)

/**
 * Modulator: Write Low Bit Rate Date.
 * Paramters: TxSetLowBitRateTransferRequest struct
 */
#define IOCTL_ITE_MOD_WRITE_LOWBITRATEDATA \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x12, TxSetLowBitRateTransferRequest)

/**
 * Modulator: Set PCR Mode.
 * Paramters: TxSetPcrModeRequest struct
 */
#define IOCTL_ITE_MOD_SETPCRMODE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x13, TxSetPcrModeRequest)

/**
 * Modulator: Set DC Table.
 * Paramters: TxSetPcrModeRequest struct
 */
#define IOCTL_ITE_MOD_SETDCTABLE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x14, TxSetDCTableRequest)

/**
 * Enable Get Frequency Index Value From API.
 * Paramters:   GetFrequencyIndexRequest struct
 */
#define IOCTL_ITE_MOD_GETFREQUENCYINDEX \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x15, TxGetFrequencyIndexRequest)

/* ISDB-T */

/**
 * Set ISDB-T Channel Modulation.
 * Paramters:   TXSetISDBTChannelModulationRequest struct
 */
#define IOCTL_ITE_MOD_SETISDBTCHANNELMODULATION \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x00, TXSetISDBTChannelModulationRequest)

/**
 * Set TMCC Information.
 * Paramters:   TXSetTMCCInfoRequest struct
 */
#define IOCTL_ITE_MOD_SETTMCCINFO \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x01, TXSetTMCCInfoRequest)

/**
 * Get TMCC Information.
 * Paramters:   TXGetTMCCInfoRequest struct
 */
#define IOCTL_ITE_MOD_GETTMCCINFO \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x02, TXGetTMCCInfoRequest)

/**
 * Get TS Input Bit Rate.
 * Paramters:   TXGetTSinputBitRate struct
 */
#define IOCTL_ITE_MOD_GETTSINPUTBITRATE \
    _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x03, TXGetTSinputBitRateRequest)

/**
 * Get Add Pid To ISDBT Pid Filter.
 * Paramters:   TXGetTSinputBitRate struct
 */
#define IOCTL_ITE_MOD_ADDPIDTOISDBTPIDFILTER \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x04, TXAddPidToISDBTPidFilterRequest)

/**
 * Get DTV Mode.
 * Paramters:   TxGetDTVModeRequest struct
 */
#define IOCTL_ITE_MOD_GETDTVMODE \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x05, TxGetDTVModeRequest)

/* SECURITY */

/**
 * Enable TPS Encryption.
 * Paramters:   TxEnableTpsEncryptionRequest struct
 */
#define IOCTL_ITE_MOD_ENABLETPSENCRYPTION \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_SECURITY + 0x01, TxEnableTpsEncryptionRequest)

/**
 * Disable TPS Encryption.
 * Paramters:   TxDisableTpsEncryptionRequest struct
 */
#define IOCTL_ITE_MOD_DISABLETPSENCRYPTION \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_SECURITY + 0x02, TxDisableTpsEncryptionRequest)

/**
 * Set TPS Decryption.
 * Paramters:   TxSetDecryptRequest struct
 */
#define IOCTL_ITE_DEMOD_SETDECRYPT \
    _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_SECURITY + 0x03, TxSetDecryptRequest)

Dword DemodIOCTLFun(
    void *        handle,
    Dword         IOCTLCode,
    unsigned long pIOBuffer);


//----------------------------------------------------------------------------
// Class internals, the "guts" internal class.
//----------------------------------------------------------------------------

class ts::HiDesDevice::Guts
{
public:
    int             fd;    // File descriptor.
    HiDesDeviceInfo info;  // Portable device information.

    // Constructor, destructor.
    Guts();
    ~Guts();

    // Open a device. Index is optional.
    bool open(int index, const UString& name, Report& report);

    // Close the device.
    void close();

    // Get all HiDes device names.
    static void GetAllDeviceNames(UStringVector& names);
};


//----------------------------------------------------------------------------
// Public class, constructor and destructor.
//----------------------------------------------------------------------------

ts::HiDesDevice::HiDesDevice() :
    _is_open(false),
    _guts(new Guts)
{
}

ts::HiDesDevice::~HiDesDevice()
{
    // Free internal resources.
    if (_guts != 0) {
        delete _guts;
        _guts = 0;
    }
}


//----------------------------------------------------------------------------
// Guts, constructor and destructor.
//----------------------------------------------------------------------------

ts::HiDesDevice::Guts::Guts() :
    fd(-1),
    info()
{
}

ts::HiDesDevice::Guts::~Guts()
{
    close();
}

void ts::HiDesDevice::Guts::close()
{
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}


//----------------------------------------------------------------------------
// Get all HiDes device names.
//----------------------------------------------------------------------------

void ts::HiDesDevice::Guts::GetAllDeviceNames(UStringVector& names)
{
    ExpandWildcard(names, u"/dev/usb-it95?x*");
}


//----------------------------------------------------------------------------
// Get all HiDes devices in the system.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::GetAllDevices(HiDesDeviceInfoList& devices, Report& report)
{
    // Clear returned array.
    devices.clear();

    // Get list of devices.
    UStringVector names;
    Guts::GetAllDeviceNames(names);

    // Loop on all devices.
    for (size_t index = 0; index < names.size(); ++index) {

        // Open the device on a dummy Guts object.
        // Ignore errors. We know that index and names are correct and they
        // describe a real device. Errors come from fetching other properties.
        Guts guts;
        guts.open(index, names[index], report);

        // Push the description of the device.
        devices.push_back(guts.info);
        guts.close();
    }

    return true;
}

//----------------------------------------------------------------------------
// Open a device. Internal version.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::Guts::open(int index, const UString& name, Report& report)
{
    // Reinit info structure.
    info.clear();
    info.index = index;
    info.name = BaseName(name);
    info.path = name;

    // Open the device.
    fd = ::open(name.toUTF8().c_str(), O_RDWR);
    if (fd < 0) {
        const int err = LastErrorCode();
        report.error(u"error opening %s: %s", {name, ErrorCodeMessage(err)});
        return false;
    }

    // After this point, we don't return on error, but we report the final status.
    bool status = true;

    // Get chip type.
    TxGetChipTypeRequest chipTypeRequest;
    TS_ZERO(chipTypeRequest);
    if (::ioctl(fd, IOCTL_ITE_MOD_GETCHIPTYPE, &chipTypeRequest) < 0) {
        const int err = LastErrorCode();
        report.error(u"error getting chip type on %s: %s", {info.path, ErrorCodeMessage(err)});
        status = false;
    }
    else {
        info.chip_type = uint16_t(chipTypeRequest.chipType);
    }

    // Get device type
    TxGetDeviceTypeRequest devTypeRequest;
    TS_ZERO(devTypeRequest);
    if (::ioctl(fd, IOCTL_ITE_MOD_GETDEVICETYPE, &devTypeRequest) < 0) {
        const int err = LastErrorCode();
        report.error(u"error getting device type on %s: %s", {info.path, ErrorCodeMessage(err)});
        status = false;
    }
    else {
        info.device_type = int(devTypeRequest.DeviceType);
    }

    // Get driver information.
    TxModDriverInfo driverRequest;
    TS_ZERO(driverRequest);
    if (::ioctl(fd, IOCTL_ITE_MOD_GETDRIVERINFO, &driverRequest) < 0) {
        const int err = LastErrorCode();
        report.error(u"error getting driver info on %s: %s", {info.path, ErrorCodeMessage(err)});
        status = false;
    }
    else {
        // Make sure all strings are nul-terminated.
        // This may result in a sacrifice of the last character.
        // But it is still better than trashing memory.

#define TS_ZCOPY(field1, field2) \
        driverRequest.field2[sizeof(driverRequest.field2) - 1] = 0; \
        info.field1.assignFromUTF8(reinterpret_cast<const char*>(&driverRequest.field2))

        TS_ZCOPY(driver_version, DriverVerion);
        TS_ZCOPY(api_version, APIVerion);
        TS_ZCOPY(link_fw_version, FWVerionLink);
        TS_ZCOPY(ofdm_fw_version, FWVerionOFDM);
        TS_ZCOPY(company, Company);
        TS_ZCOPY(hw_info, SupportHWInfo);
    }

    // In case of error, close file descriptor.
    if (!status) {
        close();
    }
    return status;
}


//----------------------------------------------------------------------------
// Open the HiDes device.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::open(int index, Report& report)
{
    // Error if already open.
    if (_is_open) {
        report.error(u"%s already open", {_guts->info.path});
        return false;
    }

    // Get all devices and check index.
    UStringVector names;
    Guts::GetAllDeviceNames(names);
    if (index < 0 || size_t(index) >= names.size()) {
        report.error(u"HiDes adapter %s not found", {index});
        return false;
    }

    // Perform opening.
    _is_open = _guts->open(index, names[index], report);
    return _is_open;
}

bool ts::HiDesDevice::open(const UString& name, Report& report)
{
    // Error if already open.
    if (_is_open) {
        report.error(u"%s already open", {_guts->info.path});
        return false;
    }

    // Perform opening. No index provided.
    _is_open = _guts->open(-1, name, report);
    return _is_open;
}


//----------------------------------------------------------------------------
// Close the device.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::close(Report& report)
{
    // Silently ignore "already closed".
    _guts->close();
    _is_open = false;
    return true;
}
