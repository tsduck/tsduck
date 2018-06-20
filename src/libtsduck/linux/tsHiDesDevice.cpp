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
//  An encapsulation of a HiDes modulator device - Linux implementation.
//
//  An insane driver:
//
//  The it950x driver is probably the worst Linux driver in terms of design
//  and interface. Here is a non-exhaustive list of discrepancies that were
//  discovered and which have an impact on the application:
//
//  1. The driver interface defines its own integer type and there are
//     INCONSISTENCIES between the int types and the associated comment.
//     Typically, the size of a 'long' depends on the platform (32 vs. 64 bits).
//     And a 'long long' is often 64-bit on 32-bit platforms despite the comment
//     (32 bits). So, there is a bug somewhere:
//     - Either the definitions are correct and consistently used in the driver
//       code and application code. And the comments are incorrect.
//     - Or the comments are correct and the definitions are broken on some
//       platforms. Extensive testing is required on 32 and 64-bit platforms.
//
//  2. The write(2) system call returns an error code instead of a size.
//     For more than 40 years, write(2) is documented as returning the number
//     of written bytes or -1 on error. In Linux kernel, the write(2) returned
//     value is computed by the driver. And the it950x driver is completely
//     insane here: It returns a status code (0 on success). Doing this clearly
//     breaks the Unix file system paradigm "a file is a file" and writing to
//     a file is a consistent operation on all file systems. Additionally, in
//     case of success, we have no clue on the written size (assume all).
//
//  3. The Linux driver cannot regulate its output. The data are written to an
//     internal buffer of the driver and control is immediately returned to
//     the application. Unlike any well-behaved driver, the driver cannot
//     suspend the application when the buffer is full, waiting for space
//     in the buffer. When the buffer is full, the write operation fails with
//     an error, forcing the application to do some polling. This is exactly
//     what a drvier should NOT do! Polling is the enemy of performance and
//     accuracy.
//
//  Implementation notes:
//
//  The documented limitation for transmission size is 348 packets. The it950x
//  driver contains an internal buffer named "URB" to store packets. The size
//  of the URB is #define URB_BUFSIZE_TX 32712 (172 packets, 348/2). To avoid
//  issues, we limit our I/O's to 172 packets at a time, the URB size.
//
//  Any write(2) operation may fail because of the absence of regulation. The
//  "normal" error is an insufficient free buffer size, error code 59. In that
//  case, the application must do some polling (wait and retry). All other
//  error codes are probably "real" errors.
//
//  First, to avoid issues in case of other "normal" error or when the error
//  code values change in a future version, we treat all errors equally. This
//  means that we always retry, but not infinitely.
//
//  Then, t challenge with polling is to wait:
//  - not too long to avoid missed deadlines and holes in the transmission,
//  - not too short to avoid excessince CPU load,
//  - not too many times to avoid hanging an application on real errors.
//
//  In the original HiDes / ITE sample test code, the application infinitely
//  retries after waiting 100 micro-seconds. This is insane...
//
//  Here, we keep track of the transmission time and bitrate since the first
//  transmitted packet. Before a write, we try to predict the amount of time
//  to wait until write will be possible without retry. Then, if retry is
//  needed anyway, we loop a few times on short waits.
//
//----------------------------------------------------------------------------

#include "tsHiDesDevice.h"
#include "tsNullReport.h"
#include "tsMemoryUtils.h"
#include "tsSysUtils.h"
#include "tsMonotonic.h"
#include "tsNames.h"
TSDUCK_SOURCE;


// Maximum size of our transfers. See comments above.
#define ITE_MAX_SEND_PACKETS  172
#define ITE_MAX_SEND_BYTES    (ITE_MAX_SEND_PACKETS * 188)


//----------------------------------------------------------------------------
// Type definitions from HiDes / ITE.
//----------------------------------------------------------------------------

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

// Use 'k' as magic number.
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
    Word    tableGroups;   // Number of IQtable groups
    Dword   tableVersion;
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
    Word    tableGroups;   // Number of IQtable groups
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
    Byte segmentType;       // 0:Firmware download 1:Rom copy 2:Direct command
    Dword segmentLength;
} Segment;

typedef enum {
    Bandwidth_6M = 0,       // Signal bandwidth is 6MHz
    Bandwidth_7M,           // Signal bandwidth is 7MHz
    Bandwidth_8M,           // Signal bandwidth is 8MHz
    Bandwidth_5M            // Signal bandwidth is 5MHz
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
    Interval_1_OVER_32 = 0, // Guard interval is 1/32 of symbol length
    Interval_1_OVER_16,     // Guard interval is 1/16 of symbol length
    Interval_1_OVER_8,      // Guard interval is 1/8 of symbol length
    Interval_1_OVER_4       // Guard interval is 1/4 of symbol length
} Interval;

typedef enum {
    Priority_HIGH = 0,      // DVB-T - identifies high-priority stream
    Priority_LOW            // DVB-T - identifies low-priority stream
} Priority;                 // High Priority or Low Priority

typedef enum {
    CodeRate_1_OVER_2 = 0,  // Signal uses FEC coding ratio of 1/2
    CodeRate_2_OVER_3,      // Signal uses FEC coding ratio of 2/3
    CodeRate_3_OVER_4,      // Signal uses FEC coding ratio of 3/4
    CodeRate_5_OVER_6,      // Signal uses FEC coding ratio of 5/6
    CodeRate_7_OVER_8,      // Signal uses FEC coding ratio of 7/8
    CodeRate_NONE           // None, NXT doesn't have this one
} CodeRate;

typedef enum {
    Hierarchy_NONE = 0,     // Signal is non-hierarchical
    Hierarchy_ALPHA_1,      // Signalling format uses alpha of 1
    Hierarchy_ALPHA_2,      // Signalling format uses alpha of 2
    Hierarchy_ALPHA_4       // Signalling format uses alpha of 4
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
    Byte transmissionMode;   // transmissionMode = 1, 2, 3, 4
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
    Byte serviceType;    // Service Type(P/D): 0x00: Program, 0x80: Data
    Dword serviceId;
    Dword frequency;
    Label serviceLabel;
    Byte totalComponents;
} Service;

typedef struct {
    Byte serviceType;        // Service Type(P/D): 0x00: Program, 0x80: Data
    Dword serviceId;         // Service ID
    Word componentId;        // Stream audio/data is subchid, packet mode is SCId
    Byte componentIdService; // Component ID within Service
    Label componentLabel;
    Byte language;           // Language code
    Byte primary;            // Primary/Secondary
    Byte conditionalAccess;  // Conditional Access flag
    Byte componentType;      // Component Type (A/D)
    Byte transmissionId;     // Transmission Mechanism ID
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

//
// In DVB-T mode, only value is valid. In DVB-H mode,
// as sectionType = SectionType_SIPSI: only value is valid.
// as sectionType = SectionType_TABLE: both value and table is valid.
// as sectionType = SectionType_MPE: except table all other fields is valid.
//
typedef struct {
    Byte table;                 // The table ID. Which is used to filter specific SI/PSI table.
    Byte duration;              // The maximum burst duration. It can be specify to 0xFF if user don't know the exact value.
    FrameRow frameRow;          // The frame row of MPE-FEC. It means the exact number of rows for each column in MPE-FEC frame.
    SectionType sectionType;    // The section type of pid. See the defination of SectionType.
    Priority priority;          // The priority of MPE data. Only valid when sectionType is set to SectionType_MPE.
    IpVersion version;          // The IP version of MPE data. Only valid when sectionType is set to SectionType_MPE.
    Bool cache;                 // True: MPE data will be cached in device's buffer. Fasle: MPE will be transfer to host.
    Word value;                 // The 13 bits Packet ID.
} Pid;

typedef struct {
    Dword address;      // The address of target register
    Byte value;         // The value of target register
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

typedef struct {
    Byte highCodeRate;
    Byte lowCodeRate;
    Byte transmissionMode;
    Byte constellation;
    Byte interval;
    Word cellid;
} TPS;

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

typedef enum {
    Constellation_QPSK = 0,  // Signal uses QPSK constellation
    Constellation_16QAM,     // Signal uses 16QAM constellation
    Constellation_64QAM      // Signal uses 64QAM constellation
} Constellation;

typedef enum {
    ARIB_STD_B31 = 0,   // System based on this specification
    ISDB_TSB            // System for ISDB-TSB
} SystemIdentification;

typedef struct {
    Constellation constellation; // Constellation scheme (FFT mode) in use
    CodeRate      codeRate;      // FEC coding ratio of high-priority stream
} TMCC;

typedef struct _TMCCINFO{
    TMCC                 layerA;
    TMCC                 layerB;
    Bool                 isPartialReception;
    SystemIdentification systemIdentification;
} TMCCINFO;

typedef enum {
   filter  = 0,
   LayerB  = 1,
   LayerA  = 2,
   LayerAB = 3
} TransportLayer;

typedef enum {
    DownSampleRate_21_OVER_1 = 0,  // Signal uses FEC coding ratio of 21/1
    DownSampleRate_21_OVER_2,      // Signal uses FEC coding ratio of 21/2
    DownSampleRate_21_OVER_3,      // Signal uses FEC coding ratio of 21/3
    DownSampleRate_21_OVER_4,      // Signal uses FEC coding ratio of 21/4
    DownSampleRate_21_OVER_5,      // Signal uses FEC coding ratio of 21/5
    DownSampleRate_21_OVER_6,      // Signal uses FEC coding ratio of 21/6
} DownSampleRate;

typedef enum {
    TransmissionMode_2K = 0,    // OFDM frame consists of 2048 different carriers (2K FFT mode)
    TransmissionMode_8K = 1,    // OFDM frame consists of 8192 different carriers (8K FFT mode)
    TransmissionMode_4K = 2     // OFDM frame consists of 4096 different carriers (4K FFT mode)
} TransmissionModes;

typedef enum {
    PcrModeDisable = 0,
    PcrMode1 = 1,
    PcrMode2,
    PcrMode3
} PcrMode;

typedef struct {
    Byte     chip;
    Word     bandwidth;
    uint32_t frequency;
    uint32_t error;
    Byte     reserved[16];
} AcquireChannelRequest;

typedef struct {
    Byte     chip;
    Byte     transmissionMode;
    Byte     constellation;
    Byte     interval;
    Byte     highCodeRate;
    uint32_t error;
    Byte     reserved[16];
} TxSetModuleRequest;

typedef struct {
    Byte     chip;
    Word     bandwidth;
    uint32_t frequency;
    uint32_t error;
    Byte     reserved[16];
} TxAcquireChannelRequest;

typedef struct {
    Byte     OnOff;
    uint32_t error;
    Byte     reserved[16];
} TxModeRequest;

typedef struct {
    Byte     DeviceType;
    uint32_t error;
    Byte     reserved[16];
} TxSetDeviceTypeRequest;

typedef struct {
    Byte     DeviceType;
    uint32_t error;
    Byte     reserved[16];
} TxGetDeviceTypeRequest;

typedef struct {
    int      GainValue;
    uint32_t error;
} TxSetGainRequest;

typedef struct {
    Byte  chip;
    Bool  locked;
    Dword error;
    Byte  reserved[16];
} IsLockedRequest;

typedef struct {
    Byte*     platformLength;
    Platform* platforms;
    Dword     error;
    Byte      reserved[16];
} AcquirePlatformRequest;

typedef struct {
    Byte     chip;
    Byte     index;
    Pid      pid;
    uint32_t error;
    Byte     reserved[16];
} AddPidAtRequest;

typedef struct {
    Byte     chip;
    Byte     index;
    Pid      pid;
    uint32_t error;
    Byte     reserved[16];
} TxAddPidAtRequest;

typedef struct {
    Byte     chip;
    uint32_t error;
    Byte     reserved[16];
} ResetPidRequest;

typedef struct {
    Byte     chip;
    uint32_t error;
    Byte     reserved[16];
} TxResetPidRequest;

typedef struct {
    Byte     chip;
    uint32_t channelStatisticAddr;  // ChannelStatistic*
    uint32_t error;
    Byte     reserved[16];
} GetChannelStatisticRequest;

typedef struct {
    Byte      chip;
    Statistic statistic;
    uint32_t  error;
    Byte      reserved[16];
} GetStatisticRequest;

typedef struct {
    Byte     chip;
    Byte     control;
    uint32_t error;
    Byte     reserved[16];
} ControlPidFilterRequest;

typedef struct {
    Byte     control;
    Byte     enable;
    uint32_t error;
    Byte     reserved[16];
} TxControlPidFilterRequest;

typedef struct {
    Byte     chip;
    Byte     control;
    uint32_t error;
    Byte     reserved[16];
} ControlPowerSavingRequest;

typedef struct {
    Byte     chip;
    Byte     control;
    uint32_t error;
    Byte     reserved[16];
} TxControlPowerSavingRequest;

typedef struct {
    Byte     DriverVerion[16];   // XX.XX.XX.XX Ex., 1.2.3.4
    Byte     APIVerion[32];      // XX.XX.XXXXXXXX.XX Ex., 1.2.3.4
    Byte     FWVerionLink[16];   // XX.XX.XX.XX Ex., 1.2.3.4
    Byte     FWVerionOFDM[16];   // XX.XX.XX.XX Ex., 1.2.3.4
    Byte     DateTime[24];       // Ex.,"2004-12-20 18:30:00" or "DEC 20 2004 10:22:10" with compiler __DATE__ and __TIME__
    Byte     Company[8];         // Ex.,"ITEtech"
    Byte     SupportHWInfo[32];  // Ex.,"Jupiter DVBT/DVBH"
    uint32_t error;
    Byte     reserved[128];
} DemodDriverInfo;

typedef struct {
    Byte     DriverVerion[16];   // XX.XX.XX.XX Ex., 1.2.3.4
    Byte     APIVerion[32];      // XX.XX.XXXXXXXX.XX Ex., 1.2.3.4
    Byte     FWVerionLink[16];   // XX.XX.XX.XX Ex., 1.2.3.4
    Byte     FWVerionOFDM[16];   // XX.XX.XX.XX Ex., 1.2.3.4
    Byte     DateTime[24];       // Ex.,"2004-12-20 18:30:00" or "DEC 20 2004 10:22:10" with compiler __DATE__ and __TIME__
    Byte     Company[8];         // Ex.,"ITEtech"
    Byte     SupportHWInfo[32];  // Ex.,"Jupiter DVBT/DVBH"
    uint32_t error;
    Byte     reserved[128];
} TxModDriverInfo;

// Demodulator Stream control API commands
typedef struct {
    Byte  chip;
    Dword error;
    Byte  reserved[16];
} StartCaptureRequest;

typedef struct {
    Byte  chip;
    Dword error;
    Byte  reserved[16];
} TxStartTransferRequest;

typedef struct {
    Byte  chip;
    Dword error;
    Byte  reserved[16];
} TxStopTransferRequest;

typedef struct {
    Byte  chip;
    Dword error;
    Byte  reserved[16];
} StopCaptureRequest;

typedef struct {
    uint32_t len;
    uint32_t cmdAddr;        // Byte*
    uint32_t error;
    Byte     reserved[16];
} TxCmdRequest;

typedef struct {
    uint32_t error;
    uint32_t frequency;
    Word     bandwidth;
    int      maxGain;
    int      minGain;
    Byte     reserved[16];
} TxGetGainRangeRequest;

typedef struct {
    TPS      tps;
    uint32_t error;
    Byte     reserved[16];
} TxGetTPSRequest;

typedef struct {
    TPS      tps;
    Bool     actualInfo;
    uint32_t error;
    Byte     reserved[16];
} TxSetTPSRequest;

typedef struct {
    int      gain;
    uint32_t error;
    Byte     reserved[16];
} TxGetOutputGainRequest;

typedef struct {
    uint32_t error;
    uint32_t pbufferAddr;
    Byte     reserved[16];
} TxSendHwPSITableRequest;

typedef struct {
    Byte     psiTableIndex;
    uint32_t pbufferAddr;
    uint32_t error;
    Byte     reserved[16];
} TxAccessFwPSITableRequest;

typedef struct {
    Byte     psiTableIndex;
    Word     timer;
    uint32_t error;
    Byte     reserved[16];
} TxSetFwPSITableTimerRequest;

typedef struct {
    uint32_t pBufferAddr;            // Byte*
    uint32_t pdwBufferLength;
    uint32_t error;
    Byte     reserved[16];
} TxSetLowBitRateTransferRequest;

typedef struct {
    uint32_t pIQtableAddr;       // Byte*
    Word     IQtableSize;
    uint32_t error;
    Byte     reserved[16];
} TxSetIQTableRequest;

typedef struct {
    int      dc_i;
    int      dc_q;
    uint32_t error;
    Byte     reserved[16];
} TxSetDCCalibrationValueRequest;

typedef struct {
    Word     chipType;
    uint32_t error;
    Byte     reserved[16];
} TxGetChipTypeRequest;

typedef struct {
    uint32_t isdbtModulationAddr;    //  ISDBTModulation
    uint32_t error;
    Byte     reserved[16];
} TXSetISDBTChannelModulationRequest;

typedef struct {
    TMCCINFO TmccInfo;
    Bool     actualInfo;
    uint32_t error;
    Byte     reserved[16];
} TXSetTMCCInfoRequest;

typedef struct {
    TMCCINFO TmccInfo;
    uint32_t error;
    Byte     reserved[16];
} TXGetTMCCInfoRequest;

typedef struct {
    Word     BitRate_Kbps;
    uint32_t error;
    Byte     reserved[16];
} TXGetTSinputBitRateRequest;

typedef struct {
    Byte           index;
    Pid            pid;
    TransportLayer layer;
    uint32_t       error;
    Byte           reserved[16];
} TXAddPidToISDBTPidFilterRequest;

typedef struct {
    PcrMode  mode;
    uint32_t error;
    Byte     reserved[16];
} TxSetPcrModeRequest;

typedef struct {
    uint32_t DCInfoAddr; //DCInfo*
    uint32_t error;
    Byte     reserved[16];
} TxSetDCTableRequest;

typedef struct {
    Byte     frequencyindex;
    uint32_t error;
    Byte     reserved[16];
} TxGetFrequencyIndexRequest;

typedef struct {
    Byte     Mode;
    uint32_t error;
    Byte     reserved[16];
} TxGetDTVModeRequest;

typedef struct {
    uint32_t key;
    uint32_t error;
    Byte     reserved[16];
} TxEnableTpsEncryptionRequest;

typedef struct {
    uint32_t error;
    Byte     reserved[16];
} TxDisableTpsEncryptionRequest;

typedef struct {
    uint32_t decryptKey;
    Byte     decryptEnable;
    uint32_t error;
    Byte     reserved[16];
} TxSetDecryptRequest;

typedef struct {
    Bool     isInversion;
    uint32_t error;
    Byte     reserved[16];
} TxSetSpectralInversionRequest;

//
// Modulator & Demodulator API commands.
//
#define IOCTRL_ITE_GROUP_STANDARD  0x000
#define IOCTRL_ITE_GROUP_DVBT      0x100
#define IOCTRL_ITE_GROUP_DVBH      0x200
#define IOCTRL_ITE_GROUP_FM        0x300
#define IOCTRL_ITE_GROUP_TDMB      0x400
#define IOCTRL_ITE_GROUP_OTHER     0x500
#define IOCTRL_ITE_GROUP_ISDBT     0x600
#define IOCTRL_ITE_GROUP_SECURITY  0x700
//
// == STANDARD ==
//
// Write a sequence of bytes to the contiguous registers in demodulator.
// Parameters: WriteRegistersRequest struct
//
#define IOCTL_ITE_DEMOD_WRITEREGISTERS _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x00, WriteRegistersRequest)
//
// Read a sequence of bytes from the contiguous registers in demodulator.
// Parameters: ReadRegistersRequest struct
//
#define IOCTL_ITE_DEMOD_READREGISTERS _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x06, ReadRegistersRequest)
//
// Specify the bandwidth of channel and tune the channel to the specific
// frequency. Afterwards, host could use output parameter dvbH to determine
// if there is a DVB-H signal.
// In DVB-T mode, after calling this function output parameter dvbH should
// be False and host could use output parameter "locked" to indicate if the
// TS is correct.
// In DVB-H mode, after calling this function output parameter dvbH should
// be True and host could use Jupiter_acquirePlatorm to get platform.
// Parameters: AcquireChannelRequest struct
//
#define IOCTL_ITE_DEMOD_ACQUIRECHANNEL _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x14, AcquireChannelRequest)
//
// Get all the platforms found in current frequency.
// Parameters: IsLockedRequest struct
//
#define IOCTL_ITE_DEMOD_ISLOCKED _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x15, IsLockedRequest)
//
// Get the statistic values of demodulator, it includes Pre-Viterbi BER,
// Post-Viterbi BER, Abort Count, Signal Presented Flag, Signal Locked Flag,
// Signal Quality, Signal Strength, Delta-T for DVB-H time slicing.
// Parameters: GetStatisticRequest struct
//
#define IOCTL_ITE_DEMOD_GETSTATISTIC _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x18, GetStatisticRequest)
//
// Get the statistic values of demodulator, it includes Pre-Viterbi BER,
// Post-Viterbi BER, Abort Count, Signal Presented Flag, Signal Locked Flag,
// Signal Quality, Signal Strength, Delta-T for DVB-H time slicing.
// Parameters: GetChannelStatisticRequest struct
//
#define IOCTL_ITE_DEMOD_GETCHANNELSTATISTIC _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x19, GetChannelStatisticRequest)
//
// Parameters: ControlPowerSavingRequest struct
//
#define IOCTL_ITE_DEMOD_CONTROLPOWERSAVING _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x1E, ControlPowerSavingRequest)
//
// Modulator Set Modulation.
// Parameters: TxSetModuleRequest struct
//
#define IOCTL_ITE_MOD_SETMODULE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x21, TxSetModuleRequest)
//
// Modulator Acquire Channel.
// Parameters: TxAcquireChannelRequest struct
//
#define IOCTL_ITE_MOD_ACQUIRECHANNEL _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x22, TxAcquireChannelRequest)
//
// Modulator Null Packet Enable.
// Parameters: TxModeRequest struct
//
#define IOCTL_ITE_MOD_ENABLETXMODE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x23, TxModeRequest)
//
// Read a sequence of bytes from the contiguous registers in demodulator.
// Parameters: ReadRegistersRequest struct
//
#define IOCTL_ITE_MOD_READREGISTERS _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x24, TxReadRegistersRequest)
//
// Write a sequence of bytes to the contiguous registers in demodulator.
// Parameters: TxWriteRegistersRequest struct
//
#define IOCTL_ITE_MOD_WRITEREGISTERS _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x27, TxWriteRegistersRequest)
//
// Modulator Device Type Setting.
// Parameters: TxSetDeviceTypeRequest struct
//
#define IOCTL_ITE_MOD_SETDEVICETYPE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x28, TxSetDeviceTypeRequest)
//
// Modulator Device Type Getting.
// Parameters: TxGetDeviceTypeRequest struct
//
#define IOCTL_ITE_MOD_GETDEVICETYPE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x29, TxGetDeviceTypeRequest)
//
// Modulator Set Gain Range.
// Parameters: TxSetGainRequest struct
//
#define IOCTL_ITE_MOD_ADJUSTOUTPUTGAIN _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2B, TxSetGainRequest)
//
// Modulator Get Gain Range.
// Parameters: TxGetGainRangeRequest struct
//
#define IOCTL_ITE_MOD_GETGAINRANGE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2C, TxGetGainRangeRequest)
//
// Modulator Get Output Gain Range.
// Parameters: TxGetOutputGainRangeRequest struct
//
#define IOCTL_ITE_MOD_GETOUTPUTGAIN _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2D, TxGetOutputGainRequest)
//
// Parameters: TxControlPowerSavingRequest struct
//
#define IOCTL_ITE_MOD_CONTROLPOWERSAVING _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2F, TxControlPowerSavingRequest)
//
// Write a sequence of bytes to the contiguous cells in the EEPROM.
// Parameters: WriteEepromValuesRequest struct
//
#define IOCTL_ITE_MOD_WRITEEEPROMVALUES _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x31, TxWriteEepromValuesRequest)
//
// Read a sequence of bytes from the contiguous cells in the EEPROM.
// Parameters: ReadEepromValuesRequest struct
//
#define IOCTL_ITE_MOD_READEEPROMVALUES _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x32, TxReadEepromValuesRequest)
//
// Get Chip Type IT9507/IT9503 in modulator.
// Parameters: TxGetChipTypeRequest struct
//
#define IOCTL_ITE_MOD_GETCHIPTYPE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x3B, TxGetChipTypeRequest)
//
// Get Chip Type IT9507/IT9503 in modulator.
// Parameters: TxSetSpectralInversion struct
//
#define IOCTL_ITE_MOD_SETSPECTRALINVERSION _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x3C, TxSetSpectralInversionRequest)
//
// == DVB-T ==
//
// Reset PID from PID filter.
// Parameters: ResetPidRequest struct
//
#define IOCTL_ITE_DEMOD_RESETPID _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x02, ResetPidRequest)
//
// Enable PID filter.
// Parameters: ControlPidFilterRequest struct
//
#define IOCTL_ITE_DEMOD_CONTROLPIDFILTER _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x03, ControlPidFilterRequest)
//
// Add PID to PID filter.
// Parameters: AddPidAtRequest struct
//
#define IOCTL_ITE_DEMOD_ADDPIDAT _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x04, AddPidAtRequest)
//
// Add PID to PID filter.
// Parameters: AddPidAtRequest struct
//
#define IOCTL_ITE_MOD_ADDPIDAT _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x08, TxAddPidAtRequest)
//
// Reset PID from PID filter.
// Parameters: ResetPidRequest struct
//
#define IOCTL_ITE_MOD_RESETPID _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x10, TxResetPidRequest)
//
// Enable PID filter.
// Parameters: TxControlPidFilterRequest struct
//
#define IOCTL_ITE_MOD_CONTROLPIDFILTER _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x11, TxControlPidFilterRequest)
//
// Enable Set IQTable From File.
// Parameters: TxSetIQTableRequest struct
//
#define IOCTL_ITE_MOD_SETIQTABLE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x12, TxSetIQTableRequest)
//
// Enable Set DC Calibration Value From File.
// Parameters: TxSetDCCalibrationValueRequest struct
//
#define IOCTL_ITE_MOD_SETDCCALIBRATIONVALUE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x13, TxSetDCCalibrationValueRequest)
//
// == OTHER ==
//
// Get driver information.
// Parameters: DemodDriverInfo struct
//
#define IOCTL_ITE_DEMOD_GETDRIVERINFO _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x00, DemodDriverInfo)
//
// Start capture data stream
// Parameters: StartCaptureRequest struct
//
#define IOCTL_ITE_DEMOD_STARTCAPTURE _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x01, StartCaptureRequest)
//
// Stop capture data stream
// Parameters: StopCaptureRequest struct
//
#define IOCTL_ITE_DEMOD_STOPCAPTURE _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x02, StopCaptureRequest)
//
// Start Transfer data stream
// Parameters: StartTransferRequest struct
//
#define IOCTL_ITE_MOD_STARTTRANSFER _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x07, TxStartTransferRequest)
//
// Stop capture data stream
// Parameters: StopTransferRequest struct
//
#define IOCTL_ITE_MOD_STOPTRANSFER _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x08, TxStopTransferRequest)
//
// Modulator: Get Driver information.
// Parameters: TxModDriverInfo struct
//
#define IOCTL_ITE_MOD_GETDRIVERINFO _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x09, TxModDriverInfo)
//
// Modulator: Set Start Transfer data Streaming.
// Parameters: StopTransferRequest struct
//
#define IOCTL_ITE_MOD_STARTTRANSFER_CMD _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0A, TxStartTransferRequest)
//
// Modulator: Set Stop Transfer data Streaming.
// Parameters: TxStopTransferRequest struct
//
#define IOCTL_ITE_MOD_STOPTRANSFER_CMD _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0B, TxStopTransferRequest)
//
// Modulator: Set Command.
// Parameters: TxCmdRequest struct
//
#define IOCTL_ITE_MOD_WRITE_CMD _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0C, TxCmdRequest)
//
// Modulator: Get TPS.
// Parameters: TxGetTPSRequest struct
//
#define IOCTL_ITE_MOD_GETTPS _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0D, TxGetTPSRequest)
//
// Modulator: Set TPS.
// Parameters: TxSetTPSRequest struct
//
#define IOCTL_ITE_MOD_SETTPS _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0E, TxSetTPSRequest)
//
// Modulator: Send PSI Table to Hardware.
// Parameters: TxSetTPSRequest struct
//
#define IOCTL_ITE_MOD_SENDHWPSITABLE _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0F, TxSendHwPSITableRequest)
//
// Modulator: Access PSI Table to firmware.
// Parameters: TxSetTPSRequest struct
//
#define IOCTL_ITE_MOD_ACCESSFWPSITABLE _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x10, TxAccessFwPSITableRequest)
//
// Modulator: Access PSI Table to firmware.
// Parameters: TxSetTPSRequest struct
//
#define IOCTL_ITE_MOD_SETFWPSITABLETIMER _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x11, TxSetFwPSITableTimerRequest)
//
// Modulator: Write Low Bit Rate Date.
// Parameters: TxSetLowBitRateTransferRequest struct
//
#define IOCTL_ITE_MOD_WRITE_LOWBITRATEDATA _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x12, TxSetLowBitRateTransferRequest)
//
// Modulator: Set PCR Mode.
// Parameters: TxSetPcrModeRequest struct
//
#define IOCTL_ITE_MOD_SETPCRMODE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x13, TxSetPcrModeRequest)
//
// Modulator: Set DC Table.
// Parameters: TxSetPcrModeRequest struct
//
#define IOCTL_ITE_MOD_SETDCTABLE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x14, TxSetDCTableRequest)
//
// Enable Get Frequency Index Value From API.
// Parameters: GetFrequencyIndexRequest struct
//
#define IOCTL_ITE_MOD_GETFREQUENCYINDEX _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x15, TxGetFrequencyIndexRequest)
//
// == ISDB-T ==
//
// Set ISDB-T Channel Modulation.
// Parameters: TXSetISDBTChannelModulationRequest struct
//
#define IOCTL_ITE_MOD_SETISDBTCHANNELMODULATION _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x00, TXSetISDBTChannelModulationRequest)
//
// Set TMCC Information.
// Parameters: TXSetTMCCInfoRequest struct
//
#define IOCTL_ITE_MOD_SETTMCCINFO _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x01, TXSetTMCCInfoRequest)
//
// Get TMCC Information.
// Parameters: TXGetTMCCInfoRequest struct
//
#define IOCTL_ITE_MOD_GETTMCCINFO _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x02, TXGetTMCCInfoRequest)
//
// Get TS Input Bit Rate.
// Parameters: TXGetTSinputBitRate struct
//
#define IOCTL_ITE_MOD_GETTSINPUTBITRATE _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x03, TXGetTSinputBitRateRequest)
//
// Get Add Pid To ISDBT Pid Filter.
// Parameters: TXGetTSinputBitRate struct
//
#define IOCTL_ITE_MOD_ADDPIDTOISDBTPIDFILTER _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x04, TXAddPidToISDBTPidFilterRequest)
//
// Get DTV Mode.
// Parameters: TxGetDTVModeRequest struct
//
#define IOCTL_ITE_MOD_GETDTVMODE _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x05, TxGetDTVModeRequest)
//
// == SECURITY ==
//
// Enable TPS Encryption.
// Parameters: TxEnableTpsEncryptionRequest struct
//
#define IOCTL_ITE_MOD_ENABLETPSENCRYPTION _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_SECURITY + 0x01, TxEnableTpsEncryptionRequest)
//
// Disable TPS Encryption.
// Parameters: TxDisableTpsEncryptionRequest struct
//
#define IOCTL_ITE_MOD_DISABLETPSENCRYPTION _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_SECURITY + 0x02, TxDisableTpsEncryptionRequest)
//
// Set TPS Decryption.
// Parameters: TxSetDecryptRequest struct
//
#define IOCTL_ITE_DEMOD_SETDECRYPT _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_SECURITY + 0x03, TxSetDecryptRequest)


//----------------------------------------------------------------------------
// Class internals, the "guts" internal class.
//----------------------------------------------------------------------------

class ts::HiDesDevice::Guts
{
public:
    int             fd;            // File descriptor.
    bool            transmitting;  // Transmission in progress.
    BitRate         bitrate;       // Nominal bitrate from last tune operation.
    Monotonic       due_time;      // Expected time of buffer availability.
    PacketCounter   pkt_sent;      // Total packets sent.
    uint64_t        all_write;     // Statistics: total number of write(2) operations.
    uint64_t        fail_write;    // Statistics: number of failed write(2) operations.
    HiDesDeviceInfo info;          // Portable device information.

    // Constructor, destructor.
    Guts();
    ~Guts();

    // Open a device. Index is optional.
    bool open(int index, const UString& name, Report& report);

    // Redirected services for enclosing class.
    void close();
    bool startTransmission(Report& report);
    bool stopTransmission(Report& report);
    bool send(const TSPacket* data, size_t packet_count, Report& report);

    // Get all HiDes device names.
    static void GetAllDeviceNames(UStringVector& names);

    // Get HiDes error message.
    static UString HiDesErrorMessage(ssize_t driver_status, int errno_status);
};


//----------------------------------------------------------------------------
// Guts, constructor and destructor.
//----------------------------------------------------------------------------

ts::HiDesDevice::Guts::Guts() :
    fd(-1),
    transmitting(false),
    bitrate(0),
    due_time(),
    pkt_sent(0),
    all_write(0),
    fail_write(0),
    info()
{
}

ts::HiDesDevice::Guts::~Guts()
{
    close();
}


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
// Get HiDes error message.
//----------------------------------------------------------------------------

ts::UString ts::HiDesDevice::Guts::HiDesErrorMessage(ssize_t driver_status, int errno_status)
{
    UString msg;

    // HiDes status can be a negative value. Zero means no error.
    if (driver_status != 0) {
        msg = DVBNameFromSection(u"HiDesError", std::abs(driver_status), names::HEXA_FIRST);
    }

    // In case errno was also set.
    if (errno_status != 0 && errno_status != driver_status) {
        if (!msg.empty()) {
            msg.append(u", ");
        }
        msg.append(ErrorCodeMessage(errno_status));
    }

    return msg;
}


//----------------------------------------------------------------------------
// Get all HiDes device names.
//----------------------------------------------------------------------------

void ts::HiDesDevice::Guts::GetAllDeviceNames(UStringVector& names)
{
    // First, get all /dev/usb-it95?x* devices.
    ExpandWildcard(names, u"/dev/usb-it95?x*");

    // Then, filter out receiver devices (we keep only transmitters / modulators).
    for (auto it = names.begin(); it != names.end(); ) {
        if (it->contain(u"-rx")) {
            it = names.erase(it);
        }
        else {
            ++it;
        }
    }
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
    errno = 0;

    if (::ioctl(fd, IOCTL_ITE_MOD_GETCHIPTYPE, &chipTypeRequest) < 0 || chipTypeRequest.error != 0) {
        const int err = errno;
        report.error(u"error getting chip type on %s: %s", {info.path, HiDesErrorMessage(chipTypeRequest.error, err)});
        status = false;
    }
    else {
        info.chip_type = uint16_t(chipTypeRequest.chipType);
    }

    // Get device type
    TxGetDeviceTypeRequest devTypeRequest;
    TS_ZERO(devTypeRequest);
    errno = 0;

    if (::ioctl(fd, IOCTL_ITE_MOD_GETDEVICETYPE, &devTypeRequest) < 0 || devTypeRequest.error != 0) {
        const int err = errno;
        report.error(u"error getting device type on %s: %s", {info.path, HiDesErrorMessage(devTypeRequest.error, err)});
        status = false;
    }
    else {
        info.device_type = int(devTypeRequest.DeviceType);
    }

    // Get driver information.
    TxModDriverInfo driverRequest;
    TS_ZERO(driverRequest);
    errno = 0;

    if (::ioctl(fd, IOCTL_ITE_MOD_GETDRIVERINFO, &driverRequest) < 0 || driverRequest.error != 0) {
        const int err = errno;
        report.error(u"error getting driver info on %s: %s", {info.path, HiDesErrorMessage(driverRequest.error, err)});
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
// Get information about the device.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::getInfo(HiDesDeviceInfo& info, Report& report) const
{
    if (_is_open) {
        info = _guts->info;
        return true;
    }
    else {
        report.error(u"HiDes device not open");
        return false;
    }
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

void ts::HiDesDevice::Guts::close()
{
    if (fd >= 0) {
        if (transmitting) {
            stopTransmission(NULLREP);
        }
        ::close(fd);
    }
    transmitting = false;
    fd = -1;
}


//----------------------------------------------------------------------------
// Set or get the output gain in dB.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::setGain(int& gain, Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    TxSetGainRequest request;
    TS_ZERO(request);
    request.GainValue = gain;
    errno = 0;

    if (::ioctl(_guts->fd, IOCTL_ITE_MOD_ADJUSTOUTPUTGAIN, &request) < 0 || request.error != 0) {
        const int err = errno;
        report.error(u"error setting gain on %s: %s", {_guts->info.path, Guts::HiDesErrorMessage(request.error, err)});
        return false;
    }

    // Updated value.
    gain = request.GainValue;
    return true;
}

bool ts::HiDesDevice::getGain(int& gain, Report& report)
{
    gain = 0;

    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    TxGetOutputGainRequest request;
    TS_ZERO(request);
    errno = 0;

    if (::ioctl(_guts->fd, IOCTL_ITE_MOD_GETOUTPUTGAIN, &request) < 0 || request.error != 0) {
        const int err = errno;
        report.error(u"error getting gain on %s: %s", {_guts->info.path, Guts::HiDesErrorMessage(request.error, err)});
        return false;
    }

    // Updated value.
    gain = request.gain;
    return true;
}


//----------------------------------------------------------------------------
// Get the allowed range of output gain in dB.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::getGainRange(int& minGain, int& maxGain, uint64_t frequency, BandWidth bandwidth, Report& report)
{
    minGain = maxGain = 0;

    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    // Frequency and bandwidth are in kHz
    TxGetGainRangeRequest request;
    TS_ZERO(request);
    request.frequency = uint32_t(frequency / 1000);
    request.bandwidth = BandWidthValueHz(bandwidth) / 1000;
    errno = 0;

    if (request.bandwidth == 0) {
        report.error(u"unsupported bandwidth");
        return false;
    }

    if (::ioctl(_guts->fd, IOCTL_ITE_MOD_GETGAINRANGE, &request) < 0 || request.error != 0) {
        const int err = errno;
        report.error(u"error getting gain range on %s: %s", {_guts->info.path, Guts::HiDesErrorMessage(request.error, err)});
        return false;
    }

    maxGain = request.maxGain;
    minGain = request.minGain;
    return true;
}


//----------------------------------------------------------------------------
// Tune the modulator with DVB-T modulation parameters.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::tune(const TunerParametersDVBT& params, Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    // Build frequency + bandwidth parameters.
    TxAcquireChannelRequest acqRequest;
    TS_ZERO(acqRequest);

    // Frequency is in kHz.
    acqRequest.frequency = uint32_t(params.frequency / 1000);

    // Bandwidth is in kHz
    acqRequest.bandwidth = BandWidthValueHz(params.bandwidth) / 1000;
    if (acqRequest.bandwidth == 0) {
        report.error(u"unsupported bandwidth");
        return false;
    }

    // Build modulation parameters.
    // Translate TSDuck enums into HiDes codes.
    TxSetModuleRequest modRequest;
    TS_ZERO(modRequest);

    switch (params.modulation) {
        case QPSK:
            modRequest.constellation = Byte(Mode_QPSK);
            break;
        case QAM_16:
            modRequest.constellation = Byte(Mode_16QAM);
            break;
        case QAM_64:
            modRequest.constellation = Byte(Mode_64QAM);
            break;
        default:
            report.error(u"unsupported constellation");
            return false;
    }

    switch (params.fec_hp) {
        case FEC_1_2:
            modRequest.highCodeRate = Byte(CodeRate_1_OVER_2);
            break;
        case FEC_2_3:
            modRequest.highCodeRate = Byte(CodeRate_2_OVER_3);
            break;
        case FEC_3_4:
            modRequest.highCodeRate = Byte(CodeRate_3_OVER_4);
            break;
        case FEC_5_6:
            modRequest.highCodeRate = Byte(CodeRate_5_OVER_6);
            break;
        case FEC_7_8:
            modRequest.highCodeRate = Byte(CodeRate_7_OVER_8);
            break;
        default:
            report.error(u"unsupported high priority code rate");
            return false;
    }

    switch (params.guard_interval) {
        case GUARD_1_32:
            modRequest.interval = Byte(Interval_1_OVER_32);
            break;
        case GUARD_1_16:
            modRequest.interval = Byte(Interval_1_OVER_16);
            break;
        case GUARD_1_8:
            modRequest.interval = Byte(Interval_1_OVER_8);
            break;
        case GUARD_1_4:
            modRequest.interval = Byte(Interval_1_OVER_4);
            break;
        default:
            report.error(u"unsupported guard interval");
            return false;
    }

    switch (params.transmission_mode) {
        case TM_2K:
            modRequest.transmissionMode = Byte(TransmissionMode_2K);
            break;
        case TM_4K:
            modRequest.transmissionMode = Byte(TransmissionMode_4K);
            break;
        case TM_8K:
            modRequest.transmissionMode = Byte(TransmissionMode_8K);
            break;
        default:
            report.error(u"unsupported transmission mode");
            return false;
    }

    // Build spectral inversion parameters.
    TxSetSpectralInversionRequest invRequest;
    TS_ZERO(invRequest);
    bool setInversion = true;

    switch (params.inversion) {
        case SPINV_OFF:
            invRequest.isInversion = False;
            break;
        case SPINV_ON:
            invRequest.isInversion = True;
            break;
        case SPINV_AUTO:
            setInversion = false;
            break;
        default:
            report.error(u"unsupported spectral inversion");
            return false;
    }

    // Now all parameters are validated, call the driver.
    errno = 0;
    if (::ioctl(_guts->fd, IOCTL_ITE_MOD_ACQUIRECHANNEL, &acqRequest) < 0 || acqRequest.error != 0) {
        const int err = errno;
        report.error(u"error setting frequency & bandwidth: %s", {Guts::HiDesErrorMessage(acqRequest.error, err)});
        return false;
    }

    errno = 0;
    if (::ioctl(_guts->fd, IOCTL_ITE_MOD_SETMODULE, &modRequest) < 0 || modRequest.error != 0) {
        const int err = errno;
        report.error(u"error setting modulation parameters: %s", {Guts::HiDesErrorMessage(modRequest.error, err)});
        return false;
    }

    errno = 0;
    if (setInversion && (::ioctl(_guts->fd, IOCTL_ITE_MOD_SETSPECTRALINVERSION, &invRequest) < 0 || invRequest.error != 0)) {
        const int err = errno;
        report.error(u"error setting spectral inversion: %s", {Guts::HiDesErrorMessage(invRequest.error, err)});
        return false;
    }

    // Keep nominal bitrate.
    _guts->bitrate = params.theoreticalBitrate();
    return true;
}


//----------------------------------------------------------------------------
// Start transmission (after having set tuning parameters).
//----------------------------------------------------------------------------

bool ts::HiDesDevice::startTransmission(Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }
    else {
        return _guts->startTransmission(report);
    }
}

bool ts::HiDesDevice::Guts::startTransmission(Report& report)
{
    // Request of a clock precision of 1 millisecond if possible.
    const NanoSecond prec = Monotonic::SetPrecision(NanoSecPerMilliSec);
    report.log(2, u"HiDesDevice: get system precision of %'d non-second", {prec});

    TxModeRequest modeRequest;
    TS_ZERO(modeRequest);
    modeRequest.OnOff = 1;
    errno = 0;

    if (::ioctl(fd, IOCTL_ITE_MOD_ENABLETXMODE, &modeRequest) < 0 || modeRequest.error != 0) {
        const int err = errno;
        report.error(u"error enabling transmission: %s", {Guts::HiDesErrorMessage(modeRequest.error, err)});
        return false;
    }

    TxStartTransferRequest startRequest;
    TS_ZERO(startRequest);
    errno = 0;

    if (::ioctl(fd, IOCTL_ITE_MOD_STARTTRANSFER, &startRequest) < 0 || startRequest.error != 0) {
        const int err = errno;
        report.error(u"error starting transmission: %s", {Guts::HiDesErrorMessage(startRequest.error, err)});
        return false;
    }

    transmitting = true;
    pkt_sent = 0;
    all_write = 0;
    fail_write = 0;
    return true;
}


//----------------------------------------------------------------------------
// Stop transmission.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::stopTransmission(Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }
    else {
        return _guts->stopTransmission(report);
    }
}

bool ts::HiDesDevice::Guts::stopTransmission(Report& report)
{
    TxStopTransferRequest stopRequest;
    TS_ZERO(stopRequest);
    errno = 0;

    if (::ioctl(fd, IOCTL_ITE_MOD_STOPTRANSFER, &stopRequest) < 0 || stopRequest.error != 0) {
        const int err = errno;
        report.error(u"error stopping transmission: %s", {Guts::HiDesErrorMessage(stopRequest.error, err)});
        return false;
    }

    TxModeRequest modeRequest;
    TS_ZERO(modeRequest);
    modeRequest.OnOff = 0;
    errno = 0;

    if (::ioctl(fd, IOCTL_ITE_MOD_ENABLETXMODE, &modeRequest) < 0 || modeRequest.error != 0) {
        const int err = errno;
        report.error(u"error disabling transmission: %s", {Guts::HiDesErrorMessage(modeRequest.error, err)});
        return false;
    }

    transmitting = false;
    return true;
}


//----------------------------------------------------------------------------
// Send TS packets.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::send(const TSPacket* packets, size_t packet_count, Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }
    else {
        return _guts->send(packets, packet_count, report);
    }
}

bool ts::HiDesDevice::Guts::send(const TSPacket* packets, size_t packet_count, Report& report)
{
    if (!transmitting) {
        report.error(u"transmission not started");
        return false;
    }

    if (bitrate != 0) {
        if (pkt_sent == 0) {
            // This is the first send operation, initialize timer.
            due_time.getSystemTime();
        }
        else {
            // Check if due time of all previous packets is in the past. In that case, the application
            // was late, we have lost synchronization and we should reset the timer.
            Monotonic now;
            now.getSystemTime();
            if (due_time < now) {
                report.log(2, u"HiDesDevice: late by %'d nano-seconds", {now - due_time});
                due_time = now;
                pkt_sent = 0;
            }
        }
    }

    // Retry several write operations until everything is gone.
    report.log(2, u"HiDesDevice: send %d packets, bitrate = %'d b/s", {packet_count, bitrate});
    const char* data = reinterpret_cast<const char*>(packets);
    size_t remain = packet_count * PKT_SIZE;

    // Normally, we wait before each write operation to be right on time.
    // But, in case we wake up just before the buffer is emptied, we allow
    // a number of short wait timers. These values are arbitrary and may
    // require some tuing in the future.
    const ::useconds_t error_delay = 100;
    const size_t max_retry = 100;
    size_t retry_count = 0;

    while (remain > 0) {

        // Send one burst. Get max burst size.
        const size_t burst = std::min<size_t>(remain, ITE_MAX_SEND_BYTES);

        // If this is the first attempt for this chunk, wait until due time.
        if (retry_count == 0 && bitrate != 0) {
            due_time.wait();
        }

        // Send the chunk.
        // WARNING: write returns an error code, not a size, see comments at beginning of file.
        errno = 0;
        const ssize_t status = ::write(fd, data, burst);
        const int err = errno;

        // Keep statistics on all write operations.
        all_write++;
        if (status != 0) {
            fail_write++;
        }
        report.log(2, u"HiDesDevice:: write = %d, errno = %d, after %d fail (total write: %'d, failed: %'d))", {status, err, retry_count, all_write, fail_write});

        if (status == 0) {
            // Success, assume that the complete burst was sent (ie. written in the buffer in the driver).
            data += burst;
            remain -= burst;
            pkt_sent += burst;
            // Add expected transmission time to our monotonic timer.
            if (bitrate != 0) {
                due_time += (burst * 8 * PKT_SIZE * NanoSecPerSec) / NanoSecond(bitrate);
            }
            // Reset retry count if there are errors in subsequent chunks.
            retry_count = 0;
        }
        else if (errno == EINTR) {
            // Ignore signal, retry
            report.debug(u"HiDesDevice::send: interrupted by signal, retrying");
        }
        else if (retry_count < max_retry) {
            // Short wait and retry same I/O.
            ::usleep(error_delay);
            retry_count++;
        }
        else {
            // Error and no more retry allowed.
            report.error(u"error sending data: %s", {HiDesErrorMessage(status, err)});
            return false;
        }
    }

    return true;
}
