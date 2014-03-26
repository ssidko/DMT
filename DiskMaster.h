#ifndef _DISK_MASTER
#define _DISK_MASTER

#include <windows.h>
#include <assert.h>
#include "ata8.h"
#include "abstract.h"
#include "Utilities.h"

namespace DM 
{
#define DM_NAME_MAX_LEN								(DWORD)64
#define DM_DESCRIPTION_STR							"USB DiskMaster"

#define DM_ENABLED									1
#define DM_DISABLED									0

#define DM_BAD_MARKER_0000							0
#define DM_BAD_MARKER_BAD							1

#define DM_DEFAULT_OPT_READ_COUNT					(WORD) 1					// „исло попыток чтени€ дефектных секторов
#define DM_DEFAULT_OPT_READ_TIMEOUT					(WORD) 15000				// ƒопустимое врем€ выполнени€ команд чтени€, мсек
#define DM_DEFAULT_OPT_SHAKE_POWER_LIMIT			(WORD) 100					// ќбщее допустимое число переключений питани€ в течении одной задачи
#define DM_DEFAULT_OPT_CHIRP						(WORD) DM_ENABLED			// –азрешение/запрещение звукового сигнала при обработке ошибок чтени€/записи
#define DM_DEFAULT_OPT_CRC_BEEP						(WORD) DM_ENABLED			// –азрешение/запрещение звукового сигнала при обработке ошибок CRC
#define DM_DEFAULT_OPT_END_BEEP						(WORD) DM_ENABLED			// –азрешение/запрещение звукового сигнала при завершении (прекращении) задачи
#define DM_DEFAULT_OPT_BAD_MARKER					(WORD) DM_BAD_MARKER_BAD	// ћаркер

#define DM_OPTION_READ_COUNT_MIN					(WORD) 1
#define DM_OPTION_READ_COUNT_MAX					(WORD) 10

#define DM_OPTION_READ_TIMEOUT_MIN					(WORD) 3000
#define DM_OPTION_READ_TIMEOUT_MAX					(WORD) 15000

#define DM_OPTION_SHAKE_POWER_LIMIT_MIN				(WORD) 10
#define DM_OPTION_SHAKE_POWER_LIMIT_MAX				(WORD) 100


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

	enum  DM_Port {
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

	enum DM_TaskEndCode {
		kEndCodeTaskEnd = 0x1111,
		kEndCodeTaskBreak = 0x2222,
		kEndCodeTaskCrash = 0x3333,
		kEndCodeTaskNotEnd = 0x4444,
		kEndCodeTaskShkPwrEnd = 0x5555,
		kEndCodeTaskCrcErrorEnd = 0x6666
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
		WORD end_code;
		DM_LBA lba;
		WORD rep_count;
		DWORD bad_count;
		WORD timeout_count;
		WORD time_cnt_1s;
		WORD time_cnt_1m;
		WORD time_cnt_1h;
		WORD time_cnt_1d;
		WORD time_max;
		DWORD slow_30_cnt;
		DWORD slow_100_cnt;
		DWORD slow_300_cnt;
	} DM_TASK_INFO, *PDM_TASK_INFO;

	typedef struct _DM_OPTION {
		WORD NumRepeatRd;				// „исло попыток чтени€ дефектных секторов
		WORD RdTimeLimit;				// ƒопустимое врем€ выполнени€ команд чтени€, мсек
		WORD ShkPwrLimit;				// ќбщее допустимое число переключений питани€ в течении одной задачи
		WORD Chirp;						// –азрешение/запрещение звукового сигнала при обработке ошибок чтени€/записи
		WORD CrcBeep;					// –азрешение/запрещение звукового сигнала при обработке ошибок CRC
		WORD EndBeep;					// –азрешение/запрещение звукового сигнала при завершении (прекращении) задачи
		WORD BadMarker;					// ћаркер
	} DM_OPTION, *PDM_OPTION;

	typedef struct _DM_LBA_RANGE {
		DM_LBA start;
		DM_LBA end;
	} DM_LBA_RANGE, *PDM_LBA_RANGE;

	typedef struct _DM_COPY_OFFSET {
		DM_LBA offset;
		WORD direction;
	} DM_COPY_OFFSET, *PDM_COPY_OFFSET;

#pragma pack(pop)

	class DMDisk;

	class DiskMaster : public DiskController
	{
	private:
		DWORD id;
		DWORD unique_id;
		IO *io;
		char name[DM_NAME_MAX_LEN];
		PORT ports[kPortsCount];
		DMDisk *disks[kPortsCount];
		BOOL opened;
		BOOL task_in_progress;
		WORD current_task;
		WORD last_cmd;
		ULONGLONG last_bad;
		DM_TASK_INFO task_info;
		DM_OPTION option;

		void Initialize(void);
		BOOL IsValidCommand(WORD cmd_code);
		BOOL SendCommand(DM_CMD_MSG_HEADER &cmd);
		void AddDisk(DMDisk *disk, BYTE port_num);
		void RemoveDisk(BYTE port_num);
		void RemoveAllDisks(void);
		void ProcessDetectInfo(DM_DETECT_INFO &di, BYTE task_code);

		BOOL CmdCheckReady(void);
		BOOL CmdBoardOn();
		BOOL CmdBoardOff();
		BOOL CmdSendOption(DM_OPTION *option);
		BOOL CmdSendTask(BYTE task_code);
		BOOL CmdSetSata1Size(ULONGLONG &new_size);
		BOOL CmdSetCopyOffset(DM_COPY_OFFSET &copy_offset);
		BOOL CmdCopyBlock(DM_LBA_RANGE &range);
		BOOL CmdTestBlock(DM_LBA_RANGE &range);
		BOOL CmdEraseBlock(DM_LBA_RANGE &range);
		BOOL CmdReadLbaHdd1(BYTE *buff, ULONGLONG &lba, DWORD lba_size);
		BOOL CmdReadLbaHdd2(BYTE *buff, ULONGLONG &lba, DWORD lba_size);
		BOOL CmdWriteLbaHdd2(BYTE *buff, ULONGLONG &lba, DWORD lba_size);
		void CmdTaskBreak(void);

		BOOL WaitForTaskEnd();
		WORD Task();
		DWORD Command(void);

		void InitializeOptionWithDefaultValues(DM_OPTION *dm_option);
		BOOL IsValidOption(DM_OPTION *dm_option);
		BOOL SetDefaultOption(DM_OPTION *dm_option);

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

		BOOL SetOption(DM_OPTION *dm_option);
		void GetOption(DM_OPTION *dm_option);

		BOOL SetDiskSize(DMDisk &disk, ULONGLONG &new_max_lba);

		void Testing(void *param = NULL)
		{
			if (Open()) {
				BOOL ret = FALSE; 
				DM_OPTION option;
				option.NumRepeatRd = DM_DEFAULT_OPT_READ_COUNT;
				option.RdTimeLimit = DM_DEFAULT_OPT_READ_TIMEOUT;
				option.ShkPwrLimit = DM_DEFAULT_OPT_SHAKE_POWER_LIMIT;
				option.Chirp = DM_DEFAULT_OPT_CHIRP;
				option.CrcBeep = DM_DEFAULT_OPT_CRC_BEEP;
				option.EndBeep = DM_DEFAULT_OPT_END_BEEP;
				option.BadMarker = DM_DEFAULT_OPT_BAD_MARKER;

				ret = this->CmdSendOption(&option);

				int x = 0;	
			}		
		}
	};
}

#endif // _DISK_MASTER
