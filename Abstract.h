#ifndef _ABSTRACT
#define _ABSTRACT

#include <windows.h>
#include "assert.h"
#include "Utilities.h"

namespace DM
{
	class DiskController;

	typedef struct _BLOCKS_RANGE {
		ULONGLONG start;
		ULONGLONG end;
		_BLOCKS_RANGE() {}
		_BLOCKS_RANGE(ULONGLONG _start, ULONGLONG _end) : start(_start), end(_end) {assert(start < end);}
	} BLOCKS_RANGE, *PBLOCKS_RANGE;

	enum PortFeatures {
		kReadOnly = 1 << 1,
		kReadWrite = 1 << 2,
		kSetMaxAddress = 1 << 3
	};

	enum  BusType {
		kUsb = 1,
		kSata,
		kAta,
		kScsi,
		kSas
	};

	#define PORT_NAME_MAX_LENGTH				32

	typedef struct _PORT {
		DWORD number;
		DWORD bus_type;
		DWORD features;
		char name[PORT_NAME_MAX_LENGTH];
		DiskController *dc;
	} PORT, *PPORT;

	class BlockDevice
	{
	public:
		virtual ~BlockDevice() {}

		virtual BOOL Open() = 0;

		virtual BOOL Close() = 0;

		virtual DWORD ReadBlock(ULONGLONG block_offset, BYTE *buff, DWORD block_count) = 0;

		virtual DWORD WriteBlock(ULONGLONG block_offset,BYTE *buff, DWORD block_count) = 0;

		// ���������� ������ ���������� � ������.
		virtual ULONGLONG Size() = 0;

		// ���������� ������ �����.
		virtual DWORD BlockSize() = 0;
	};

	class Disk : public BlockDevice
	{
	public:
		virtual ~Disk() {}

		virtual const PORT *Port(void) = 0;

		virtual const char *Model(void) = 0;

		virtual const char *SerialNumber(void) = 0;

		// ���������� ������ ������ ����� � ��������.
		virtual ULONGLONG NativeSize(void) = 0;
	};

	//enum DC_State {
	//	kReady,
	//	kTaskInProgress,
	//};

	class DiskController : public DCNotifier
	{
	public:
		virtual ~DiskController() {};

		virtual BOOL Open() = 0;

		virtual BOOL Close() = 0;

		virtual BOOL IsOpen() = 0;

		virtual DWORD GetID(void) = 0;

		virtual DWORD GetUniqueID(void) = 0;

		virtual const char *GetName(void) = 0;

		virtual DWORD PortsCount(void) = 0;

		virtual const PORT *Port(DWORD index) = 0;

		// ����������� ����� �� ���� ������.
		// ���������� ���-�� ����������������� ������.
		virtual DWORD Rescan(void) = 0;
		
		// ����������� ���� �� ��������� �����.
		// ���� ���� �������������� ���������� ��������� �� Disk.
		virtual Disk *Rescan(DWORD port_number) = 0;

		// ���������� ���� ����������������� ������.
		virtual Disk *GetDisk(DWORD port_number) = 0;

		virtual BOOL Copy(DWORD src_port, DWORD dst_port, DWORD param) = 0;

		virtual BOOL CopyEx(DWORD src_port, DWORD dst_port, ULONGLONG &src_offset, ULONGLONG &dst_offset, ULONGLONG &count, DWORD param) = 0;

		virtual BOOL Erase(DWORD port, DWORD param) = 0;
		
		virtual BOOL EraseEx(DWORD port, ULONGLONG &offset, ULONGLONG &count, DWORD param) = 0;

		virtual BOOL Test(DWORD port, DWORD param) = 0;

		virtual BOOL TestEx(DWORD port, ULONGLONG &offset, ULONGLONG &count, DWORD param) = 0;

		virtual void Break(void) = 0;
	};
}

#endif