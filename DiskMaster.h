#ifndef _DISK_MASTER
#define _DISK_MASTER

#include <windows.h>
#include <assert.h>
#include "ata8.h"
#include "DMDisk.h"
#include "abstract.h"
#include "Utilities.h"

namespace DM 
{
#define DM_DESCRIPTION_MAX_LEN				(DWORD)64
#define DM_DESCRIPTION_STR					"USB DiskMaster"

	enum DM_Event {
		kNewDiskDetected,			// param = (DMDisk *)
		kDiskRemoved,				// param = (DMDisk *)
		kDetectError,				// param = (WORD), DM_DetectError
		kTaskInProgress,			// param = (DM_TASK_INFO *)
		kTaskComplete,				// param = (DM_TASK_INFO *)
		kTaskBreak,					// param = (DM_TASK_INFO *)
		kTaskError,					// param = (DM_TASK_INFO *)
		kBadBlock					// param = (ULONGLONG *)
	};

	enum  DM_Port{
		kUsb1,
		kUsb2,
		kSata1,
		kPortsCount
	};

	enum DM_DetectError {
		kDetectErrorUsb1 = 1,
		kDetectErrorUsb2,
		kDetectErrorSata1,
		kDetectErrorSata1Lock
	};

#pragma pack(push)
#pragma pack(1)

	typedef struct _DM_CMD_MSG_HEADER {
		WORD code;
		WORD param_size;
		_DM_CMD_MSG_HEADER() : code(0), param_size(0) {}
		_DM_CMD_MSG_HEADER(WORD _code, WORD _param_size) : code(_code), param_size(_param_size) {}
		DWORD Size() {return (sizeof(DM_CMD_MSG_HEADER) + param_size);}
	} DM_CMD_MSG_HEADER, *PDM_CMD_MSG_HEADER;

	template<DWORD cmd_code, typename T>
	struct DM_COMMAND : DM_CMD_MSG_HEADER {
		T param;
		DM_COMMAND() : DM_CMD_MSG_HEADER(cmd_code, sizeof(T)) {memset(&param, 0x00, sizeof(param));}
	};

	template<DWORD msg_code, typename T>
	struct DM_MESSAGE : DM_CMD_MSG_HEADER {
		T param;
		DM_MESSAGE() : DM_CMD_MSG_HEADER(msg_code, sizeof(T)) {memset(&param, 0x00, sizeof(param));}
	};

	typedef struct _DM_LBA {
		BYTE lba[6];
		_DM_LBA &operator=(const ULONGLONG &right)
		{
			memcpy(lba, (void *)&right, sizeof(lba));
			return *this;
		}
	} DM_LBA, *PDM_LBA;

	typedef struct _DM_DETECT_INFO {
		INQUIRY_DATA usb1_id;
		INQUIRY_DATA usb2_id;
		IDENTIFY_DISK_ATA sata1_id;
		DM_LBA usb1_num_lba;
		DM_LBA usb2_num_lba;
		DWORD usb1_block_size;
		DWORD usb2_block_size;
		DM_LBA sata1_native_max;
	} DM_DETECT_INFO, *PDM_DETECT_INFO;

	typedef struct _DM_TASK_INFO {
		WORD End_code;
		DM_LBA Lba;
		WORD RepCount;
		DWORD BadCount;
		WORD TimeoutCount;
		WORD TimeCnt_1s;
		WORD TimeCnt_1m;
		WORD TimeCnt_1h;
		WORD TimeCnt_1d;
		WORD TMax;
		DWORD Slow_30_cnt;
		DWORD Slow_100_cnt;
		DWORD Slow_300_cnt;
	} DM_TASK_INFO, *PDM_TASK_INFO;

	typedef struct _SEND_OPTION {
		WORD NumRepeatRd;
		WORD RdTimeLimit;
		WORD ShkPwrLimit;
		WORD Chirp;
		WORD CrcBeep;
		WORD EndBeep;
	} SEND_OPTION, *PSEND_OPTION;

	typedef struct _DM_LBA_RANGE {
		DM_LBA start;
		DM_LBA end;
	} DM_LBA_RANGE, *PDM_LBA_RANGE;

	typedef struct _DM_COPY_OFFSET {
		DM_LBA offset;
		WORD direction;
	} DM_COPY_OFFSET, *PDM_COPY_OFFSET;

#pragma pack(pop)

	class DiskMaster : public DiskController
	{
	private:
		DWORD id;
		DWORD unique_id;
		IO *io;
		char name[DM_DESCRIPTION_MAX_LEN];
		PORT ports[kPortsCount];
		DMDisk *disks[kPortsCount];
		BOOL opened;
		BOOL task_in_progress;
		WORD dm_current_task;
		WORD dm_last_cmd;
		DM_TASK_INFO dm_task_info;
		ULONGLONG last_bad;

		void Initialize(void);
		BOOL IsValidCommand(WORD code);
		BOOL SendCommand(DM_CMD_MSG_HEADER &cmd);
		void AddDisk(DMDisk *disk, BYTE port_num);
		void RemoveDisk(BYTE port_num);
		void RemoveAllDisks(void);
		void ProcessDetectInfo(DM_DETECT_INFO &di, BYTE task_code);

		BOOL CmdCheckReady(void);
		BOOL CmdBoardOn();
		BOOL CmdBoardOff();
		BOOL CmdSendOption(SEND_OPTION &option);
		BOOL CmdSendTask(BYTE task_code);
		BOOL CmdSetSataSize(ULONGLONG &new_size);
		BOOL CmdSetCopyOffset(DM_COPY_OFFSET &copy_offset);
		BOOL CmdCopyBlock(DM_LBA_RANGE &range);
		BOOL CmdTestBlock(DM_LBA_RANGE &range);
		BOOL CmdEraseBlock(DM_LBA_RANGE &range);
		BOOL CmdReadLbaHdd1(BYTE *buff, ULONGLONG lba, DWORD lba_size);
		BOOL CmdReadLbaHdd2(BYTE *buff, ULONGLONG lba, DWORD lba_size);
		BOOL CmdWriteLbaHdd2(BYTE *buff, ULONGLONG lba, DWORD lba_size);
		void CmdTaskBreak(void);

		BOOL WaitForTaskEnd();
		WORD Task();
		DWORD Command(void);

	public:
		DiskMaster(DWORD dm_id, DWORD dm_unique_id, IO *dm_io);

		~DiskMaster();

		IO *GetIO(void);

		virtual DWORD GetID(void);
		virtual DWORD GetUniqueID(void);
		virtual const char *GetName(void);

		virtual BOOL Open();
		virtual BOOL Close();
		virtual BOOL IsOpen(void);

		const DM_TASK_INFO *TaskInfo();

		virtual DWORD PortsCount(void);
		virtual const PORT *Port(DWORD index);
		virtual DWORD Rescan(void);
		virtual Disk *Rescan(DWORD port_number);
		virtual Disk *GetDisk(DWORD port_number);

		virtual BOOL Copy(DWORD src_port, DWORD dst_port, DWORD param);
		virtual BOOL CopyEx(DWORD src_port, DWORD dst_port, ULONGLONG &src_offset, ULONGLONG &dst_offset, ULONGLONG &count, DWORD param);
		virtual BOOL Erase(DWORD port, DWORD param);
		virtual BOOL EraseEx(DWORD port, ULONGLONG &offset, ULONGLONG &count, DWORD param);
		virtual BOOL Test(DWORD port, DWORD param);
		virtual BOOL TestEx(DWORD port, ULONGLONG &offset, ULONGLONG &count, DWORD param);

		virtual void Break(void);

		BOOL ReadBlock(DWORD port, ULONGLONG &offset,  BYTE *buff, DWORD block_size) /*!!! NOT IMPLEMENTED !!!*/;
		BOOL WriteBlock(DWORD port, ULONGLONG &offset, BYTE *buff, DWORD block_size) /*!!! NOT IMPLEMENTED !!!*/;
	};
}

#endif // _DISK_MASTER
