//----------------------------------------------------------------------------
// Definition for drivers of ITE Tech. 950x chips (used in HiDes devices).
// The Windows and Linux drivers have completely different interfaces.
//----------------------------------------------------------------------------

#pragma once
#include <stdint.h>

#if defined(__llvm__) || defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wpadded" // Many structures are poorly aligned and need padding.
#endif


//----------------------------------------------------------------------------
// Windows driver interface
//----------------------------------------------------------------------------

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)

// KS property sets for it950x devices.
// Main property set. Control device operation and send TS data blocks.
#define ITE_STATIC_KSPROPSETID_IT9500Properties 0xf23fac2d,0xe1af,0x48e0,{0x8b,0xbe,0xa1,0x40,0x29,0xc9,0x2f,0x11}

// Auxiliary property set. Query USB mode and device IDs.
// This value is actually KSPROPERTYSET_Wd3KsproxySample, an example GUID
// used by some vendors where engineers don't run guidgen.exe.
#define ITE_STATIC_KSPROPSETID_IT9500PropertiesAux 0xc6efe5eb,0x855a,0x4f1b,{0xb7,0xaa,0x87,0xb5,0xe1,0xdc,0x41,0x13}

// Maximum number of TS packets to send to modulator.
#define IT95X_TX_BLOCK_PKTS 348

// For get chip type.
#define IT95X_REG_CHIP_VERSION 0x1222

namespace ite {

    // FEC code rate
    enum {
        IT95X_CODERATE_UNKNOWN = -1,
        IT95X_CODERATE_1_2     = 0,
        IT95X_CODERATE_2_3     = 1,
        IT95X_CODERATE_3_4     = 2,
        IT95X_CODERATE_5_6     = 3,
        IT95X_CODERATE_7_8     = 4,
    };

    // Constellation.
    enum {
        IT95X_CONSTELLATION_UNKNOWN = -1,
        IT95X_CONSTELLATION_QPSK    = 0,
        IT95X_CONSTELLATION_16QAM   = 1,
        IT95X_CONSTELLATION_64QAM   = 2,
    };

    // Transmission mode.
    enum {
        IT95X_TX_MODE_UNKNOWN = -1,
        IT95X_TX_MODE_2K      = 0,
        IT95X_TX_MODE_8K      = 1,
        IT95X_TX_MODE_4K      = 2,
    };

    // Guard interval.
    enum {
        IT95X_GUARD_UNKNOWN = -1,
        IT95X_GUARD_1_32    = 0,
        IT95X_GUARD_1_16    = 1,
        IT95X_GUARD_1_8     = 2,
        IT95X_GUARD_1_4     = 3,
    };

    // Properties
    enum {
        KSPROPERTY_IT95X_DRV_INFO = 0,  // in KSPROPSETID_IT9500Properties
        KSPROPERTY_IT95X_IOCTL    = 1,  // in KSPROPSETID_IT9500Properties
        KSPROPERTY_IT95X_BUS_INFO = 5,  // in KSPROPSETID_IT9500PropertiesAux
    };

    // KS property list indexes for DeviceIoControl
    enum {
        KSLIST_DRV_INFO_GET = 0,
        KSLIST_DRV_INFO_SET = 1,
        KSLIST_IOCTL_GET    = 2,
        KSLIST_IOCTL_SET    = 3,
        KSLIST_BUS_INFO_GET = 4,
        KSLIST_MAX          = 5,
    };

    // IOCTL codes for modulator
    enum {
        IOCTL_IT95X_GET_DRV_INFO             =  1,
        IOCTL_IT95X_SET_POWER                =  4,
        IOCTL_IT95X_SET_DVBT_MODULATION      =  8,
        IOCTL_IT95X_SET_RF_OUTPUT            =  9,
        IOCTL_IT95X_SEND_TS_DATA             = 30,
        IOCTL_IT95X_SET_CHANNEL              = 31,
        IOCTL_IT95X_SET_DEVICE_TYPE          = 32,
        IOCTL_IT95X_GET_DEVICE_TYPE          = 33,
        IOCTL_IT95X_SET_GAIN                 = 34,
        IOCTL_IT95X_RD_REG_OFDM              = 35,
        IOCTL_IT95X_WR_REG_OFDM              = 36,
        IOCTL_IT95X_RD_REG_LINK              = 37,
        IOCTL_IT95X_WR_REG_LINK              = 38,
        IOCTL_IT95X_SEND_PSI_ONCE            = 39,
        IOCTL_IT95X_SET_PSI_PACKET           = 40,
        IOCTL_IT95X_SET_PSI_TIMER            = 41,
        IOCTL_IT95X_GET_GAIN_RANGE           = 42,
        IOCTL_IT95X_SET_TPS                  = 43,
        IOCTL_IT95X_GET_TPS                  = 44,
        IOCTL_IT95X_GET_GAIN                 = 45,
        IOCTL_IT95X_SET_IQ_TABLE             = 46,
        IOCTL_IT95X_SET_DC_CAL               = 47,
        IOCTL_IT95X_SET_ISDBT_MODULATION     = 60,
        IOCTL_IT95X_ADD_ISDBT_PID_FILTER     = 61,
        IOCTL_IT95X_SET_TMCC                 = 62,
        IOCTL_IT95X_SET_TMCC2                = 63,
        IOCTL_IT95X_GET_TMCC                 = 64,
        IOCTL_IT95X_GET_TS_BITRATE           = 65,
        IOCTL_IT95X_CONTROL_ISDBT_PID_FILTER = 66,
        IOCTL_IT95X_SET_PCR_MODE             = 67,
        IOCTL_IT95X_SET_PCR_ENABLE           = 68,
        IOCTL_IT95X_RESET_ISDBT_PID_FILTER   = 69,
        IOCTL_IT95X_SET_OFS_CAL              = 70,
        IOCTL_IT95X_ENABLE_TPS_CRYPT         = 71,
        IOCTL_IT95X_DISABLE_TPS_CRYPT        = 72,
    };

    // Gain sign, when stored in unsigned type.
    enum {
        GAIN_POSITIVE = 1,
        GAIN_NEGATIVE = 2,
    };

    // Parameter structure for generic DeviceIoControl.
    struct IoctlGeneric
    {
        uint32_t code;
        uint32_t param1;
        uint32_t param2;

        // Constructor.
        IoctlGeneric(uint32_t c = 0, uint32_t p1 = 0, uint32_t p2 = 0) : code(c), param1(p1), param2(p2) {}
    };

    // Parameter structure for DVB-T DeviceIoControl.
    struct IoctlDVBT
    {
        uint32_t code;
        uint8_t  code_rate;
        uint8_t  tx_mode;
        uint8_t  constellation;
        uint8_t  guard_interval;

        // Constructor.
        IoctlDVBT(uint32_t c = 0) : code(c), code_rate(0), tx_mode(0), constellation(0), guard_interval(0) {}
    };

    // Parameter structure for gain range DeviceIoControl.
    struct IoctlGainRange
    {
        uint32_t code;
        uint32_t frequency;  // in kHz
        uint32_t bandwidth;  // in kHz
        int32_t  max_gain;
        int32_t  min_gain;

        // Constructor.
        IoctlGainRange(uint32_t c = 0) : code(c), frequency(0), bandwidth(0), max_gain(0), min_gain(0) {}
    };

    // Parameter structure for DC calibration DeviceIoControl.
    struct IoctlDCCalibration
    {
        uint32_t code;
        int32_t  dc_i;
        int32_t  dc_q;
        uint8_t  reserved[8];

        // Constructor.
        IoctlDCCalibration(uint32_t c = 0) : code(c), dc_i(0), dc_q(0), reserved() {}
    };

    // Parameter structure for transmission of TS data.
    struct IoctlTransmission
    {
        uint32_t code;
        uint32_t size;
        uint8_t  data[IT95X_TX_BLOCK_PKTS * 188];

        // Constructor.
        IoctlTransmission(uint32_t c = 0) : code(c), size(0) {}
    };
}


//----------------------------------------------------------------------------
// Linux driver interface
//----------------------------------------------------------------------------

#elif defined(__gnu_linux__) || defined(__linux__) || defined(linux)

namespace ite {

    // The driver interface defines its own integer type and there are
    // INCONSISTENCIES between the int types and the associated comment.
    // Typically, the size of a 'long' depends on the platform (32 vs. 64 bits).
    // And a 'long long' is often 64-bit on 32-bit platforms despite the comment
    // (32 bits). So, there is a bug somewhere:
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
        Byte      chip;
        Processor processor;
        uint32_t  registerAddress;
        Byte      bufferLength;
        Byte      buffer[256];
        uint32_t  error;
        Byte      reserved[16];
    } WriteRegistersRequest;

    typedef struct {
        Byte      chip;
        Processor processor;
        uint32_t  registerAddress;
        Byte      bufferLength;
        Byte      buffer[256];
        uint32_t  error;
        Byte      reserved[16];
    } TxWriteRegistersRequest;

    typedef struct {
        Byte     chip;
        Word     registerAddress;
        Byte     bufferLength;
        Byte     buffer[256];
        uint32_t error;
        Byte     reserved[16];
    } TxWriteEepromValuesRequest;

    typedef struct {
        Byte      chip;
        Processor processor;
        uint32_t  registerAddress;
        Byte      bufferLength;
        Byte      buffer[256];
        uint32_t  error;
        Byte      reserved[16];
    } ReadRegistersRequest;

    typedef struct {
        Byte      chip;
        Processor processor;
        uint32_t  registerAddress;
        Byte      bufferLength;
        Byte      buffer[256];
        uint32_t  error;
        Byte      reserved[16];
    } TxReadRegistersRequest;

    typedef struct {
        Byte     chip;
        Word     registerAddress;
        Byte     bufferLength;
        Byte     buffer[256];
        uint32_t error;
        Byte     reserved[16];
    } TxReadEepromValuesRequest;

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
}

// Use 'k' as magic number.
#define AFA_IOC_MAGIC  'k'

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
#define IOCTL_ITE_DEMOD_WRITEREGISTERS _IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x00, ite::WriteRegistersRequest)
//
// Read a sequence of bytes from the contiguous registers in demodulator.
// Parameters: ReadRegistersRequest struct
//
#define IOCTL_ITE_DEMOD_READREGISTERS _IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x06, ite::ReadRegistersRequest)
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
#define IOCTL_ITE_DEMOD_ACQUIRECHANNEL ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x14, ite::AcquireChannelRequest))
//
// Get all the platforms found in current frequency.
// Parameters: IsLockedRequest struct
//
#define IOCTL_ITE_DEMOD_ISLOCKED ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x15, ite::IsLockedRequest))
//
// Get the statistic values of demodulator, it includes Pre-Viterbi BER,
// Post-Viterbi BER, Abort Count, Signal Presented Flag, Signal Locked Flag,
// Signal Quality, Signal Strength, Delta-T for DVB-H time slicing.
// Parameters: GetStatisticRequest struct
//
#define IOCTL_ITE_DEMOD_GETSTATISTIC ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x18, ite::GetStatisticRequest))
//
// Get the statistic values of demodulator, it includes Pre-Viterbi BER,
// Post-Viterbi BER, Abort Count, Signal Presented Flag, Signal Locked Flag,
// Signal Quality, Signal Strength, Delta-T for DVB-H time slicing.
// Parameters: GetChannelStatisticRequest struct
//
#define IOCTL_ITE_DEMOD_GETCHANNELSTATISTIC ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x19, ite::GetChannelStatisticRequest))
//
// Parameters: ControlPowerSavingRequest struct
//
#define IOCTL_ITE_DEMOD_CONTROLPOWERSAVING ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x1E, ite::ControlPowerSavingRequest))
//
// Modulator Set Modulation.
// Parameters: TxSetModuleRequest struct
//
#define IOCTL_ITE_MOD_SETMODULE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x21, ite::TxSetModuleRequest))
//
// Modulator Acquire Channel.
// Parameters: TxAcquireChannelRequest struct
//
#define IOCTL_ITE_MOD_ACQUIRECHANNEL ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x22, ite::TxAcquireChannelRequest))
//
// Modulator Null Packet Enable.
// Parameters: TxModeRequest struct
//
#define IOCTL_ITE_MOD_ENABLETXMODE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x23, ite::TxModeRequest))
//
// Read a sequence of bytes from the contiguous registers in demodulator.
// Parameters: ReadRegistersRequest struct
//
#define IOCTL_ITE_MOD_READREGISTERS ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x24, ite::TxReadRegistersRequest))
//
// Write a sequence of bytes to the contiguous registers in demodulator.
// Parameters: TxWriteRegistersRequest struct
//
#define IOCTL_ITE_MOD_WRITEREGISTERS ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x27, ite::TxWriteRegistersRequest))
//
// Modulator Device Type Setting.
// Parameters: TxSetDeviceTypeRequest struct
//
#define IOCTL_ITE_MOD_SETDEVICETYPE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x28, ite::TxSetDeviceTypeRequest))
//
// Modulator Device Type Getting.
// Parameters: TxGetDeviceTypeRequest struct
//
#define IOCTL_ITE_MOD_GETDEVICETYPE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x29, ite::TxGetDeviceTypeRequest))
//
// Modulator Set Gain Range.
// Parameters: TxSetGainRequest struct
//
#define IOCTL_ITE_MOD_ADJUSTOUTPUTGAIN ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2B, ite::TxSetGainRequest))
//
// Modulator Get Gain Range.
// Parameters: TxGetGainRangeRequest struct
//
#define IOCTL_ITE_MOD_GETGAINRANGE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2C, ite::TxGetGainRangeRequest))
//
// Modulator Get Output Gain Range.
// Parameters: TxGetOutputGainRangeRequest struct
//
#define IOCTL_ITE_MOD_GETOUTPUTGAIN ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2D, ite::TxGetOutputGainRequest))
//
// Parameters: TxControlPowerSavingRequest struct
//
#define IOCTL_ITE_MOD_CONTROLPOWERSAVING ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x2F, ite::TxControlPowerSavingRequest))
//
// Write a sequence of bytes to the contiguous cells in the EEPROM.
// Parameters: WriteEepromValuesRequest struct
//
#define IOCTL_ITE_MOD_WRITEEEPROMVALUES ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x31, ite::TxWriteEepromValuesRequest))
//
// Read a sequence of bytes from the contiguous cells in the EEPROM.
// Parameters: ReadEepromValuesRequest struct
//
#define IOCTL_ITE_MOD_READEEPROMVALUES ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x32, ite::TxReadEepromValuesRequest))
//
// Get Chip Type IT9507/IT9503 in modulator.
// Parameters: TxGetChipTypeRequest struct
//
#define IOCTL_ITE_MOD_GETCHIPTYPE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x3B, ite::TxGetChipTypeRequest))
//
// Get Chip Type IT9507/IT9503 in modulator.
// Parameters: TxSetSpectralInversion struct
//
#define IOCTL_ITE_MOD_SETSPECTRALINVERSION ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_STANDARD + 0x3C, ite::TxSetSpectralInversionRequest))
//
// == DVB-T ==
//
// Reset PID from PID filter.
// Parameters: ResetPidRequest struct
//
#define IOCTL_ITE_DEMOD_RESETPID ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x02, ite::ResetPidRequest))
//
// Enable PID filter.
// Parameters: ControlPidFilterRequest struct
//
#define IOCTL_ITE_DEMOD_CONTROLPIDFILTER ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x03, ite::ControlPidFilterRequest))
//
// Add PID to PID filter.
// Parameters: AddPidAtRequest struct
//
#define IOCTL_ITE_DEMOD_ADDPIDAT ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x04, ite::AddPidAtRequest))
//
// Add PID to PID filter.
// Parameters: AddPidAtRequest struct
//
#define IOCTL_ITE_MOD_ADDPIDAT ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x08, ite::TxAddPidAtRequest))
//
// Reset PID from PID filter.
// Parameters: ResetPidRequest struct
//
#define IOCTL_ITE_MOD_RESETPID ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x10, ite::TxResetPidRequest))
//
// Enable PID filter.
// Parameters: TxControlPidFilterRequest struct
//
#define IOCTL_ITE_MOD_CONTROLPIDFILTER ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x11, ite::TxControlPidFilterRequest))
//
// Enable Set IQTable From File.
// Parameters: TxSetIQTableRequest struct
//
#define IOCTL_ITE_MOD_SETIQTABLE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x12, ite::TxSetIQTableRequest))
//
// Enable Set DC Calibration Value From File.
// Parameters: TxSetDCCalibrationValueRequest struct
//
#define IOCTL_ITE_MOD_SETDCCALIBRATIONVALUE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_DVBT + 0x13, ite::TxSetDCCalibrationValueRequest))
//
// == OTHER ==
//
// Get driver information.
// Parameters: DemodDriverInfo struct
//
#define IOCTL_ITE_DEMOD_GETDRIVERINFO ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x00, ite::DemodDriverInfo))
//
// Start capture data stream
// Parameters: StartCaptureRequest struct
//
#define IOCTL_ITE_DEMOD_STARTCAPTURE ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x01, ite::StartCaptureRequest))
//
// Stop capture data stream
// Parameters: StopCaptureRequest struct
//
#define IOCTL_ITE_DEMOD_STOPCAPTURE ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x02, ite::StopCaptureRequest))
//
// Start Transfer data stream
// Parameters: StartTransferRequest struct
//
#define IOCTL_ITE_MOD_STARTTRANSFER ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x07, ite::TxStartTransferRequest))
//
// Stop capture data stream
// Parameters: StopTransferRequest struct
//
#define IOCTL_ITE_MOD_STOPTRANSFER ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x08, ite::TxStopTransferRequest))
//
// Modulator: Get Driver information.
// Parameters: TxModDriverInfo struct
//
#define IOCTL_ITE_MOD_GETDRIVERINFO ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x09, ite::TxModDriverInfo))
//
// Modulator: Set Start Transfer data Streaming.
// Parameters: StopTransferRequest struct
//
#define IOCTL_ITE_MOD_STARTTRANSFER_CMD ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0A, ite::TxStartTransferRequest))
//
// Modulator: Set Stop Transfer data Streaming.
// Parameters: TxStopTransferRequest struct
//
#define IOCTL_ITE_MOD_STOPTRANSFER_CMD ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0B, ite::TxStopTransferRequest))
//
// Modulator: Set Command.
// Parameters: TxCmdRequest struct
//
#define IOCTL_ITE_MOD_WRITE_CMD ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0C, ite::TxCmdRequest))
//
// Modulator: Get TPS.
// Parameters: TxGetTPSRequest struct
//
#define IOCTL_ITE_MOD_GETTPS ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0D, ite::TxGetTPSRequest))
//
// Modulator: Set TPS.
// Parameters: TxSetTPSRequest struct
//
#define IOCTL_ITE_MOD_SETTPS ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0E, ite::TxSetTPSRequest))
//
// Modulator: Send PSI Table to Hardware.
// Parameters: TxSetTPSRequest struct
//
#define IOCTL_ITE_MOD_SENDHWPSITABLE ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x0F, ite::TxSendHwPSITableRequest))
//
// Modulator: Access PSI Table to firmware.
// Parameters: TxSetTPSRequest struct
//
#define IOCTL_ITE_MOD_ACCESSFWPSITABLE ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x10, ite::TxAccessFwPSITableRequest))
//
// Modulator: Access PSI Table to firmware.
// Parameters: TxSetTPSRequest struct
//
#define IOCTL_ITE_MOD_SETFWPSITABLETIMER ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x11, ite::TxSetFwPSITableTimerRequest))
//
// Modulator: Write Low Bit Rate Date.
// Parameters: TxSetLowBitRateTransferRequest struct
//
#define IOCTL_ITE_MOD_WRITE_LOWBITRATEDATA ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x12, ite::TxSetLowBitRateTransferRequest))
//
// Modulator: Set PCR Mode.
// Parameters: TxSetPcrModeRequest struct
//
#define IOCTL_ITE_MOD_SETPCRMODE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x13, ite::TxSetPcrModeRequest))
//
// Modulator: Set DC Table.
// Parameters: TxSetPcrModeRequest struct
//
#define IOCTL_ITE_MOD_SETDCTABLE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x14, ite::TxSetDCTableRequest))
//
// Enable Get Frequency Index Value From API.
// Parameters: GetFrequencyIndexRequest struct
//
#define IOCTL_ITE_MOD_GETFREQUENCYINDEX ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_OTHER + 0x15, ite::TxGetFrequencyIndexRequest))
//
// == ISDB-T ==
//
// Set ISDB-T Channel Modulation.
// Parameters: TXSetISDBTChannelModulationRequest struct
//
#define IOCTL_ITE_MOD_SETISDBTCHANNELMODULATION ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x00, ite::TXSetISDBTChannelModulationRequest))
//
// Set TMCC Information.
// Parameters: TXSetTMCCInfoRequest struct
//
#define IOCTL_ITE_MOD_SETTMCCINFO ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x01, ite::TXSetTMCCInfoRequest))
//
// Get TMCC Information.
// Parameters: TXGetTMCCInfoRequest struct
//
#define IOCTL_ITE_MOD_GETTMCCINFO ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x02, ite::TXGetTMCCInfoRequest))
//
// Get TS Input Bit Rate.
// Parameters: TXGetTSinputBitRate struct
//
#define IOCTL_ITE_MOD_GETTSINPUTBITRATE ts::ioctl_request_t(_IOR(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x03, ite::TXGetTSinputBitRateRequest))
//
// Get Add Pid To ISDBT Pid Filter.
// Parameters: TXGetTSinputBitRate struct
//
#define IOCTL_ITE_MOD_ADDPIDTOISDBTPIDFILTER ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x04, ite::TXAddPidToISDBTPidFilterRequest))
//
// Get DTV Mode.
// Parameters: TxGetDTVModeRequest struct
//
#define IOCTL_ITE_MOD_GETDTVMODE ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_ISDBT + 0x05, ite::TxGetDTVModeRequest))
//
// == SECURITY ==
//
// Enable TPS Encryption.
// Parameters: TxEnableTpsEncryptionRequest struct
//
#define IOCTL_ITE_MOD_ENABLETPSENCRYPTION ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_SECURITY + 0x01, ite::TxEnableTpsEncryptionRequest))
//
// Disable TPS Encryption.
// Parameters: TxDisableTpsEncryptionRequest struct
//
#define IOCTL_ITE_MOD_DISABLETPSENCRYPTION ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_SECURITY + 0x02, ite::TxDisableTpsEncryptionRequest))
//
// Set TPS Decryption.
// Parameters: TxSetDecryptRequest struct
//
#define IOCTL_ITE_DEMOD_SETDECRYPT ts::ioctl_request_t(_IOW(AFA_IOC_MAGIC, IOCTRL_ITE_GROUP_SECURITY + 0x03, ite::TxSetDecryptRequest))

#endif // Windows, Linux

#if defined(__llvm__) || defined(__clang__)
#pragma clang diagnostic pop
#endif
