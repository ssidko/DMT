#ifndef _ATA8
#define _ATA8

#pragma pack(push)
#pragma pack(1)
// ATA8 specification
typedef struct _IDENTIFY_DISK_ATA					// Word(dec)
{
	USHORT		GeneralConfiguration;				// 000
	USHORT		CylindersCHS;// Obsolete			// 001
	USHORT		SpecificConfiguration;				// 002
	USHORT		HeadsCHS;// Obsolete				// 003
	USHORT		Retired1[2];						// 004-005
	USHORT		SectorsCHS;// Obsolete				// 006
	USHORT		ReservedCFA1[2];					// 007-008
	USHORT		Retired2;							// 009
	UCHAR		SerialNumber[20];					// 010-019
	USHORT		Retired3[2];						// 020-021
	USHORT		NumberBytesLong;// Obsolete			// 022
	UCHAR		FirmwareRevision[8];				// 023-026
	UCHAR		ModelNumber[40];					// 027-046
	USHORT		MaxNumSectorsForMultiple;			// 047
	USHORT		TrustedComputingSupported;			// 048
	ULONG		Capabilities;						// 049-050
	USHORT		SettingsTimePIO;// Obsolete			// 051
	USHORT		SettingsTimeDMA;// Obsolete			// 052
	USHORT		SupportedModes;						// 053
	USHORT		CylindersCurrent;// Obsolete		// 054
	USHORT		HeadsCurrent;// Obsolete			// 055
	USHORT		SectorsCurrent;// Obsolete			// 056
	ULONG		CapacityCHS;// Obsolete				// 057-058
	USHORT		NumSectorsForMultiple;				// 059
	ULONG		TotalNumberLBA28;					// 060-061
	USHORT		ModeDMA;// Obsolete					// 062
	USHORT		ModeMultiwordDMA;					// 063
	USHORT		ModePIO;							// 064
	USHORT		MinCycleTimeMultiwordDMA;			// 065
	USHORT		VendorCycleTimeMultiwordDMA;		// 066
	USHORT		MinCycleTimePIOwithoutIORDY;		// 067
	USHORT		MinCycleTimePIOwithIORDY;			// 068
	USHORT		Reserved1[2];						// 069-070
	USHORT		ReservedIdentifyPacketDevice[4];	// 071-074
	USHORT		QueueDepth;							// 075
	USHORT		CapabilitiesSATA;					// 076
	USHORT		ReservedSATA;						// 077
	USHORT		FeaturesSupportedSATA;				// 078
	USHORT		FeaturesEnabledSATA;				// 079
	USHORT		MajorVersionNumber;					// 080
	USHORT		MinorVersionNumber;					// 081
	USHORT		FeatureSetSupported1;				// 082
	USHORT		FeatureSetSupported2;				// 083
	USHORT		FeatureSetSupported3;				// 084
	USHORT		FeatureSetSupportedOrEnabled1;		// 085
	USHORT		FeatureSetSupportedOrEnabled2;		// 086
	USHORT		FeatureSetSupportedOrEnabled3;		// 087
	USHORT		ModeUltraDMA;						// 088
	USHORT		TimeSecuriteEraseNormal;			// 089
	USHORT		TimeSecuriteEraseEnhanced;			// 090
	USHORT		CurrentAdvancedPowerManagementLevel;// 091
	USHORT		MasterPasswordIdentifier;			// 092
	USHORT		HardwareResetResults;				// 093
	USHORT		FeatureSetAAM;						// 094
	USHORT		StreamMinimumRequestSize;			// 095
	USHORT		StreamTransferTimeDMA;				// 096
	USHORT		StreamAccessLatency;				// 097
	ULONG		StreamingPerformanceGranularity;	// 098-099
	ULONGLONG	TotalNumberLBA48;					// 100-103
	USHORT		StreamTransferTimePIO;				// 104
	USHORT		Reserved2;							// 105
	USHORT		PhysicalAndLogicalSectorSize;		// 106
	USHORT		InterSeekDelay;						// 107
	USHORT		WorldWideName[4];					// 108-111
	USHORT		ReservedWorldWideName[4];			// 112-115
	USHORT		ReservedTLC;						// 116
	ULONG		LogicalSectorSize;					// 117-118
	USHORT		FeatureSetSupported4;				// 119
	USHORT		FeatureSetSupportedOrEnabled4;		// 120
	USHORT		ReservedSupportedAndEnabledExp[6];	// 121-126
	USHORT		RemovableMediaSupported;// Obsolete	// 127
	USHORT		SecurityStatus;						// 128
	USHORT		VendorSpecific[31];					// 129-159
	USHORT		PowerModeCFA;						// 160
	USHORT		ReservedCFA2[7];					// 161-167
	USHORT		DeviceNominalFormFactor;			// 168
	USHORT		Reserved3[7];						// 169-175
	UCHAR		MediaSerialNumber[40];				// 176-195
	UCHAR		MediaManufacturer[20];				// 196-205
	USHORT		CommandTransportSCT;				// 206
	USHORT		ReservedCE_ATA1[2];					// 207-208
	USHORT		AlignmentLogicalToPhysical;			// 209
	ULONG		WriteReadVerifySectorCountMode3;	// 210-211
	ULONG		WriteReadVerifySectorCountMode2;	// 212-213
	USHORT		NonVolatileCacheCapabilities;		// 214
	ULONG		NonVolatileCacheSize;				// 215-216
	USHORT		NominalMediaRotationRate;			// 217
	USHORT		Reserved4;							// 218
	USHORT		NonVolatileCacheOptions;			// 219
	USHORT		WriteReadVerifyCurrentMode;			// 220
	USHORT		Reserved5;							// 221
	USHORT		TransportMajorVersion;				// 222
	USHORT		TransportMinorVersion;				// 223
	USHORT		ReservedCE_ATA2[10];				// 224-233
	USHORT		MinNumberSectorsDownloadMicrocode;	// 234
	USHORT		MaxNumberSectorsDownloadMicrocode;	// 235
	USHORT		Reserved6[19];						// 236-254
	USHORT		IntegrityWord;						// 255
} IDENTIFY_DISK_ATA, *PIDENTIFY_DISK_ATA;

typedef struct _INQUIRY_DATA {
	// Byte 0
	BYTE peripheral_device_type:5;
	BYTE peripheral_qualifier:3;
	// Byte 1
	BYTE reserved_1:7;
	BYTE removable:1;
	// Byte 2
	BYTE version;
	// Byte 3
	BYTE response_data_format:4;
	BYTE hierarchical_support:1;
	BYTE normal_aca:1;
	BYTE obsolete_1:1;
	BYTE aerc:1;
	// Byte 4
	BYTE additional_length;
	// Byte 5
	BYTE reserved:7;
	BYTE sccs:1;
	// Byte 6
	BYTE addr:1;
	BYTE obsolete_2:1;
	BYTE obsolete_3:1;
	BYTE medium_changer:1;
	BYTE multi_port:1;
	BYTE vs_1:1;
	BYTE enclosure_services:1;
	BYTE basic_queuing:1;
	// Byte 7
	BYTE vs_2:1;
	BYTE command_queue:1;
	BYTE obsolete_5:1;
	BYTE linked_command:1;
	BYTE sync:1;
	BYTE w_bus:1;
	BYTE obsolete_4:1;
	BYTE relative_addressing:1;
	// Byte 8-15
	BYTE vendor_id[8];
	// Byte 16-31
	BYTE product_id[16];
	// Byte 32-35
	BYTE revision[4];
} INQUIRY_DATA, *PINQUIRY_DATA;
#pragma pack (pop)

#endif //_ATA8