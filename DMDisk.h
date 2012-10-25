#ifndef _DMDISK
#define _DMDISK

#include <windows.h>
#include <assert.h>
#include "abstract.h"
#include "ata8.h" 

namespace DM
{
#define DMDISK_MAX_STRING_LENGTH					64

	class DiskMaster;

	class DMDisk : public Disk
	{

		friend DiskMaster;

	private:
		PORT *port;
		INQUIRY_DATA inquiry_data;
		IDENTIFY_DISK_ATA identify_data;
		ULONGLONG size;
		ULONGLONG native_size;					// NATIVE MAX ADDRESS + 1
		DWORD block_size;
		char model[DMDISK_MAX_STRING_LENGTH];
		char serial_number[DMDISK_MAX_STRING_LENGTH];

		DMDisk(PORT *_port, INQUIRY_DATA *_inquiry, ULONGLONG _size, DWORD _block_size) :
			port(_port),
			size(_size),
			native_size(_size),
			block_size(_block_size)
		{
			assert(port);
			assert(_inquiry);
			assert(size);
			assert(native_size);
			assert(block_size);
			assert(port->bus_type == kUsb);

			memcpy(&inquiry_data, _inquiry, sizeof(inquiry_data));
			memset(&identify_data, 0x00, sizeof(identify_data));

			memset(model, 0x00, sizeof(model));
			memset(serial_number, 0x00, sizeof(serial_number));

			memcpy(model, (const char *)_inquiry->vendor_id, 24);
		}

		DMDisk(PORT *_port, IDENTIFY_DISK_ATA *_identify, ULONGLONG _native_size) : 
			port(_port),
			native_size(_native_size)
		{
			assert(port);
			assert(_identify);
			assert(native_size);
			assert(port->bus_type == kSata);

			memset(&inquiry_data, 0x00, sizeof(inquiry_data));
			memcpy(&identify_data, _identify, sizeof(identify_data));

			memset(model, 0x00, sizeof(model));
			memset(serial_number, 0x00, sizeof(serial_number));

			memcpy(model, (const char *)_identify->ModelNumber, 40);
			memcpy(serial_number, (const char *)_identify->SerialNumber, 20);

			native_size++;
			size = DetermineDiskSize(*_identify);
			assert(size);
			block_size = DetermineBlockSize(*_identify);
			assert(block_size);
		}

		DWORD DetermineBlockSize(IDENTIFY_DISK_ATA &id_data)
		{
			// If bit 14 of word 106 is set to one and bit 15 of word 106 is cleared to zero,
			// the contents of word 106 contain valid information.
			if ((id_data.PhysicalAndLogicalSectorSize & (WORD)(1 << 14)) &&
				!(id_data.PhysicalAndLogicalSectorSize & (WORD)(1 << 15))) {
					// Bit 12 of word 106 shall be cleared to 0 to indicate that words 117-118
					// are invalid and that the logical sector size is 256 words.
					if ((id_data.PhysicalAndLogicalSectorSize & (WORD)(1 << 12)) == 0x01) {
						return 2*id_data.LogicalSectorSize;
					}
			}
			return 512;
		}

		ULONGLONG DetermineDiskSize(IDENTIFY_DISK_ATA &id_data)
		{
			WORD feature_set2 = 0;
			DWORD size28 = 0;
			ULONGLONG size48 = 0;
			SwapBytesInWords((const WORD *)&id_data.FeatureSetSupported2, &feature_set2);
			if ((feature_set2 & (WORD)(1 << 10))){
				SwapBytesInWords(&id_data.TotalNumberLBA48, &size48);
				return size48;
			}
			else {
				SwapBytesInWords((const DWORD *)&id_data.TotalNumberLBA28, &size28);				
				return size28;
			}
		}

		void SwapBytesInWords(const WORD *in, WORD *out)
		{
			((BYTE *)out)[0] = ((const BYTE *)in)[1];
			((BYTE *)out)[1] = ((const BYTE *)in)[0];
		}

		void SwapBytesInWords(const DWORD *in, DWORD *out)
		{
			((BYTE *)out)[0] = ((const BYTE *)in)[1];
			((BYTE *)out)[1] = ((const BYTE *)in)[0];
			((BYTE *)out)[2] = ((const BYTE *)in)[3];
			((BYTE *)out)[3] = ((const BYTE *)in)[2];
		}

		void SwapBytesInWords(const ULONGLONG *in, ULONGLONG *out)
		{
			((BYTE *)out)[0] = ((const BYTE *)in)[1];
			((BYTE *)out)[1] = ((const BYTE *)in)[0];
			((BYTE *)out)[2] = ((const BYTE *)in)[3];
			((BYTE *)out)[3] = ((const BYTE *)in)[2];
			((BYTE *)out)[4] = ((const BYTE *)in)[5];
			((BYTE *)out)[5] = ((const BYTE *)in)[4];
			((BYTE *)out)[6] = ((const BYTE *)in)[7];
			((BYTE *)out)[7] = ((const BYTE *)in)[6];
		}

	public:

		~DMDisk(void) {}

		virtual const PORT *Port(void)
		{
			return port;
		}

		virtual const char *Model(void)
		{
			return model;
		}

		virtual const char *SerialNumber(void)
		{
			return serial_number; 
		}

		virtual BOOL Open(void)																/*!!! NOT IMPLEMENTED !!!*/
		{
			return FALSE;
		}

		virtual BOOL Close(void)															/*!!! NOT IMPLEMENTED !!!*/
		{
			return TRUE;
		}

		virtual DWORD ReadBlock(ULONGLONG block_offset, BYTE *buff, DWORD block_count)		/*!!! NOT IMPLEMENTED !!!*/
		{
			return 0;
		}

		virtual DWORD WriteBlock(ULONGLONG block_offset,BYTE *buff, DWORD block_count)		/*!!! NOT IMPLEMENTED !!!*/
		{
			return 0;
		}

		virtual ULONGLONG Size()
		{
			return size;
		}

		virtual ULONGLONG NativeSize(void)
		{
			return native_size;
		}

		virtual DWORD BlockSize()
		{
			return block_size;
		}

		int operator==(const DMDisk &right) const
		{
			if (port->number == right.port->number) {
				if ((size == right.size) && (block_size == right.block_size)) {
					if (port->bus_type == kUsb) {
						if (!memcmp(&inquiry_data, &right.inquiry_data, sizeof(inquiry_data)))
							return TRUE;
					}
					else if (port->bus_type == kSata) {
						if (!memcmp(&identify_data, &right.identify_data, sizeof(identify_data)))
							return TRUE;
					}
				}
			}				
			return FALSE;
		}
	};
}

#endif // _DMDISK
