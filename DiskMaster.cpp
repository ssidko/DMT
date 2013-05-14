#include "DiskMaster.h"

#pragma pack(push)
#pragma pack(1)

#define DM_CHECK_CODE								(WORD)0x4343
#define DM_SEND_COMMAND_DELAY						(DWORD)100				// Задержка перед пересылкой параметра
#define DM_SEND_TASK_DELAY							(DWORD)10*1000			// Задержка перед получением DETECT_INFO
#define DM_RESPONSE_TIMEOUT							(DWORD)2*60*1000		// Время в течении которого должен ответить "USB DiskMaster"


enum DM_TaskCode {
	kTaskNone,
	kTaskUsb1Sata1Copy = 8,
	kTaskUsb1Usb2Copy,			
	kTaskUsb1Read = 22,
	kTaskUsb2Read,
	kTaskSata1Read,
	kTaskSata1Verify,
	kTaskUsb2_00_Erase = 31,
	kTaskUsb2_FF_Erase,
	kTaskUsb2_Random_Erase,
	kTaskSata1_00_Erase,
	kTaskSata1_FF_Erase,
	kTaskSata1_Random_Erase,
	kTaskLastTaskCode
};

enum DM_Command {
	kCmdUnknownCommand,			// 0x00
	kCmdCheckReady,				// 0x01
	kCmdBoardOn,				// 0x02
	kCmdBoardOff,				// 0x03
	kCmdSendOption,				// 0x04
	kCmdSendTask,				// 0x05
	kCmdSetSataSize,			// 0x06
	kCmdSetCopyOffset,			// 0x07
	kCmdCopyBlock,				// 0x08
	kCmdTestBlock,				// 0x09
	kCmdEraseBlock,				// 0x0A
	kCmdReadLbaHdd1,			// 0x0B
	kCmdReadLbaHdd2,			// 0x0C
	kCmdWriteLbaHdd2,			// 0x0D
	kCmdLastCommandCode
};

enum DM_CommandComplete {
	kReadWriteEnd,
	kReadWriteError,
	kTimeOut,
	kWriteError,
	kNotReady,
	kLastCommandCompleteCode
};

enum DM_Message {
	kMsgUnknown,
	kMsgDetectInfo,				// 0x01
	kMsgDetectError,			// 0x02
	kMsgTaskInfo,				// 0x03
	kMsgBadLba,					// 0x04
	kMsgReadLbaData,			// 0x05
	kMsgEndWriteLba,			// 0x06
	kMsgGetCommand,				// 0x07
	kMsgLastMessageCode
};

enum DM_DetectError {
	kDetectErrorUsb1 = 1,
	kDetectErrorUsb2,
	kDetectErrorSata1,
	kDetectErrorSata1Lock
};

enum DM_EraseParam {
	kErasePattern_00,
	kErasePattern_FF,
	kErasePattern_Random
};

enum DM_OffsetDirection {
	kBackOffset,
	kForwardOffset
};

typedef DM::DM_COMMAND<kCmdCheckReady, WORD>					CMD_CHECK_READY;
typedef DM::DM_COMMAND<kCmdBoardOn, DWORD>						CMD_BOARD_ON;
typedef DM::DM_COMMAND<kCmdSendOption, DM::DM_OPTION>			CMD_SEND_OPTION;
typedef DM::DM_COMMAND<kCmdSendTask, WORD>						CMD_SEND_TASK;
typedef DM::DM_COMMAND<kCmdSetSataSize, DM::DM_LBA>				CMD_SET_SATA_SIZE;
typedef DM::DM_COMMAND<kCmdSetCopyOffset, DM::DM_COPY_OFFSET>	CMD_SET_COPY_OFFSET;
typedef DM::DM_COMMAND<kCmdCopyBlock, DM::DM_LBA_RANGE>			CMD_COPY_BLOCK;
typedef DM::DM_COMMAND<kCmdTestBlock, DM::DM_LBA_RANGE>			CMD_TEST_BLOCK;
typedef DM::DM_COMMAND<kCmdEraseBlock, DM::DM_LBA_RANGE>		CMD_ERASE_BLOCK;
typedef DM::DM_COMMAND<kCmdReadLbaHdd1, DM::DM_LBA>				CMD_READ_LBA_HDD1;
typedef DM::DM_COMMAND<kCmdReadLbaHdd2, DM::DM_LBA>				CMD_READ_LBA_HDD2;

typedef DM::DM_MESSAGE<kMsgDetectInfo, DM::DM_DETECT_INFO>		MSG_DETECT_INFO;
typedef DM::DM_MESSAGE<kMsgDetectError, WORD>					MSG_DETECT_ERROR;
typedef DM::DM_MESSAGE<kMsgTaskInfo, DM::DM_TASK_INFO>			MSG_TASK_INFO;
typedef DM::DM_MESSAGE<kMsgBadLba, DM::DM_LBA>					MSG_BAD_LBA;
typedef DM::DM_MESSAGE<kMsgEndWriteLba, WORD>					MSG_END_WRITE_LBA;
typedef DM::DM_MESSAGE<kMsgGetCommand, WORD>					MSG_GET_COMMAND;

typedef WORD													CMD_COMPLETE_CODE;

#pragma pack(pop)

DM::DiskMaster::DiskMaster( DWORD dm_id, DWORD dm_unique_id, IO *dm_io ) :
	id(dm_id),
	unique_id(dm_unique_id),
	io(dm_io),
	opened(FALSE),
	current_task(kTaskNone),
	last_cmd(kCmdUnknownCommand),
	last_bad(0)
{
	assert(dm_id);
	assert(dm_io);

	memset(name, 0x00, sizeof(name));
	strcpy_s(name, DM_NAME_MAX_LEN, DM_DESCRIPTION_STR);

	memset(disks, 0x00, sizeof(disks));
	memset(&task_info, 0x00, sizeof(DM_TASK_INFO));
	last_cmd = kCmdUnknownCommand;

	memset(ports, 0x00, sizeof(ports));

	ports[kUsb1].number = kUsb1;
	ports[kUsb1].bus_type = kUsb;
	ports[kUsb1].features = kReadOnly;
	ports[kUsb1].dc = this;
	strcpy_s(ports[kUsb1].name, PORT_NAME_MAX_LENGTH, "USB1");

	ports[kUsb2].number = kUsb2;
	ports[kUsb2].bus_type = kUsb;
	ports[kUsb2].features = kReadWrite;
	ports[kUsb2].dc = this;
	strcpy_s(ports[kUsb2].name, PORT_NAME_MAX_LENGTH, "USB2");

	ports[kSata1].number = kSata1;
	ports[kSata1].bus_type = kSata;
	ports[kSata1].features = kReadWrite | kSetMaxAddress;
	ports[kSata1].dc = this;
	strcpy_s(ports[kSata1].name, PORT_NAME_MAX_LENGTH, "SATA1");

	task_in_progress = FALSE;

	InitializeOptionWithDefaultValues(&option);
}

DM::DiskMaster::~DiskMaster()
{
	Close();
}

void DM::DiskMaster::Initialize( void )
{
	current_task = kTaskNone;
	memset(&task_info, 0x00, sizeof(DM_TASK_INFO));
}

BOOL DM::DiskMaster::IsValidCommand( WORD code )
{
	return ((code > kCmdUnknownCommand) && (code < kCmdLastCommandCode));
}

BOOL DM::DiskMaster::SendCommand( DM_CMD_MSG_HEADER &cmd )
{
	DMT_TRACE("   Send command: code %d, size %d\n", cmd.code, cmd.param_size);
	assert(IsValidCommand(cmd.code));

	BYTE *param = NULL;
	if (sizeof(DM_CMD_MSG_HEADER) == io->Write(&cmd, sizeof(DM_CMD_MSG_HEADER))) {
		if (!cmd.param_size) {
			last_cmd = cmd.code;
			return TRUE;
		}
		else ::Sleep(DM_SEND_COMMAND_DELAY);

		param = &((BYTE *)&cmd)[sizeof(DM_CMD_MSG_HEADER)];
		if (cmd.param_size == io->Write(param, cmd.param_size)) {
			MSG_GET_COMMAND get_cmd;
			WORD check_code = 0;
			switch(cmd.code) {
						case kCmdCheckReady:
							if (sizeof(check_code) == io->Read(&check_code, sizeof(check_code))) {
								assert(check_code == *(&((WORD *)&cmd)[2]));
								last_cmd = cmd.code;
								DMT_TRACE("   OK\n");
								return TRUE;
							}
							break;
						case kCmdBoardOff:
							last_cmd = cmd.code;
							return TRUE;
						case kCmdBoardOn:
						case kCmdSendOption:
						case kCmdSendTask:
						case kCmdSetSataSize:
						case kCmdSetCopyOffset:
						case kCmdCopyBlock:
						case kCmdTestBlock:
						case kCmdEraseBlock:
						case kCmdReadLbaHdd1:
						case kCmdReadLbaHdd2:
						case kCmdWriteLbaHdd2:
							if (sizeof(MSG_GET_COMMAND) == io->Read(&get_cmd, sizeof(MSG_GET_COMMAND))) {
								assert(get_cmd.code == kMsgGetCommand);
								assert(get_cmd.param == cmd.code);
								last_cmd = cmd.code;
								DMT_TRACE("   OK\n");
								return TRUE;
							}
							break;
						default :
							DMT_TRACE("   ERROR\n");
							return FALSE;
			}
		}
	}
	DMT_TRACE("   ERROR\n");
	return FALSE;
}

BOOL DM::DiskMaster::CmdCheckReady( void )
{
	DMT_TRACE("\n CheckReady\n");
	WORD check_code = 0;
	CMD_CHECK_READY cmd;
	cmd.param = DM_CHECK_CODE;
	return SendCommand(cmd);
}

BOOL DM::DiskMaster::CmdBoardOn()
{
	DMT_TRACE("\n BoardOn\n");
	CMD_BOARD_ON cmd;
	cmd.param = id;
	return (opened = SendCommand(cmd));
}

BOOL DM::DiskMaster::CmdBoardOff()
{
	DMT_TRACE("\n BoardOff\n");
	assert(opened);

	DM_CMD_MSG_HEADER cmd(kCmdBoardOff, 0);
	return SendCommand(cmd);
}

BOOL DM::DiskMaster::CmdSendOption( DM_OPTION *dm_option )
{
	DMT_TRACE("\n SendOption\n");
	assert(dm_option);

	CMD_SEND_OPTION cmd;
	memcpy(&cmd.param, dm_option, sizeof(DM_OPTION));
	return SendCommand(cmd);
}

void DM::DiskMaster::AddDisk( DMDisk *disk, BYTE port_num )
{
	assert(disk);
	assert(port_num < kPortsCount);
	disks[port_num] = disk;
	this->Notify(kNewDiskDetected, disks[port_num]);
}

void DM::DiskMaster::RemoveDisk( BYTE port_num )
{
	assert(port_num < kPortsCount);
	if (disks[port_num]) {
		this->Notify(kDiskRemoved, disks[port_num]);
		delete disks[port_num];
		disks[port_num] = NULL;
	}
}

void DM::DiskMaster::RemoveAllDisks( void )
{
	for (BYTE port = 0; port < kPortsCount; port++)
		RemoveDisk(port);
}

void DM::DiskMaster::ProcessDetectInfo( DM_DETECT_INFO &di, BYTE task_code )
{
	DMDisk *disk = NULL;
	ULONGLONG size = 0;
	if ((task_code == kTaskUsb1Sata1Copy) || (task_code == kTaskUsb1Usb2Copy) || (task_code == kTaskUsb1Read)) {
		memcpy(&size, &di.usb1_num_lba, sizeof(di.usb1_num_lba));
		if (size && di.usb1_block_size) {
			disk = new DMDisk(&ports[kUsb1], &di.usb1_id, size, di.usb1_block_size);
			if (disks[kUsb1] == NULL)
				AddDisk(disk, kUsb1);
			else {
				if (*disks[kUsb1] == *disk)
					delete disk;
				else {
					RemoveDisk(kUsb1);
					AddDisk(disk, kUsb1);
				}
			}
		}
	}
	if ((task_code == kTaskUsb1Usb2Copy) || (task_code == kTaskUsb2Read) || (task_code == kTaskUsb2_00_Erase) ||
		(task_code == kTaskUsb2_FF_Erase) || (task_code == kTaskUsb2_Random_Erase)) {
			size = 0;
			memcpy(&size, &di.usb2_num_lba, sizeof(di.usb2_num_lba));
			if (size && di.usb2_block_size) {
				disk = new DMDisk(&ports[kUsb2], &di.usb2_id, size, di.usb2_block_size);
				if (disks[kUsb2] == NULL)
					AddDisk(disk, kUsb2);
				else {
					if (*disks[kUsb2] == *disk)
						delete disk;
					else {
						RemoveDisk(kUsb2);
						AddDisk(disk, kUsb2);
					}
				}
			}
	}
	if ((task_code == kTaskUsb1Sata1Copy) || (task_code == kTaskSata1Read) || (task_code == kTaskSata1Verify) ||
		(task_code == kTaskSata1_00_Erase) || (task_code == kTaskSata1_FF_Erase) || (task_code == kTaskSata1_Random_Erase)) {
		size = 0;
		memcpy(&size, &di.sata1_native_max, sizeof(di.sata1_native_max));
		disk = new DMDisk(&ports[kSata1], &di.sata1_id, size);
		if (disk->Size() && disk->BlockSize()) {
			if (disks[kSata1] == NULL)
				AddDisk(disk, kSata1);
			else {
				if (*disks[kSata1] == *disk)
					delete disk;
				else {
					RemoveDisk(kSata1);
					AddDisk(disk, kSata1);
				}
			}					
		}
		else delete disk;
	}
}

BOOL DM::DiskMaster::CmdSendTask( BYTE task_code )
{
	DMT_TRACE("\n SendTask: %d\n", task_code);
	assert(opened);

	DM_DETECT_INFO info;
	DM_CMD_MSG_HEADER hdr;
	WORD error_code = 0;
	CMD_SEND_TASK cmd;
	cmd.param = task_code;
	memset(&info, 0x00, sizeof(info));
	if (SendCommand(cmd)) {
		DWORD tick = ::GetTickCount();
		while (TRUE) {
			if (sizeof(DM_CMD_MSG_HEADER) == io->Read((void *)&hdr, sizeof(DM_CMD_MSG_HEADER))) {
				switch (hdr.code) {
							case kMsgDetectInfo:
								if (sizeof(DM_DETECT_INFO) == io->Read(&info, sizeof(DM_DETECT_INFO))) {
									current_task = task_code;
									ProcessDetectInfo(info, task_code);
									DMT_TRACE("\n");
									DMT_TRACE(" DETECT INFO:\n");
									DMT_TRACE("  USB1: %s\n", info.usb1_id.vendor_id);
									DMT_TRACE("  USB2: %s\n", info.usb2_id.vendor_id);
									DMT_TRACE("  SATA1: %s\n", info.sata1_id.ModelNumber);
									DMT_TRACE("\n");
									return TRUE;
								}
								break;
							case kMsgDetectError:
								current_task = kTaskNone;
								io->Read(&error_code, sizeof(error_code));
								DMT_TRACE("  Detect Error: %d\n", error_code);
								this->Notify(kDetectError, (void *)error_code);
								return FALSE;
							default :
								throw DMException();
				}
			}
			if ((::GetTickCount() - tick) > DM_RESPONSE_TIMEOUT) {
				DMT_TRACE("Disk Master not response");
				throw DMException();
			}
		}
	}			
	return FALSE;
}

BOOL DM::DiskMaster::CmdSetSataSize( ULONGLONG &new_size )
{
	DMT_TRACE("\n SetSataSize\n");
	assert(opened);
	assert( (current_task == kTaskUsb1Sata1Copy) ||
		(current_task == kTaskSata1Read) || 
		(current_task == kTaskSata1Verify) );

	CMD_SET_SATA_SIZE cmd;
	cmd.param = new_size;
	return SendCommand(cmd);
}

BOOL DM::DiskMaster::CmdSetCopyOffset( DM_COPY_OFFSET &copy_offset )
{
	DMT_TRACE("\n SetCopyOffset\n");
	assert(opened);
	assert((current_task == kTaskUsb1Sata1Copy) || (current_task == kTaskUsb1Usb2Copy));

	CMD_SET_COPY_OFFSET cmd;
	memcpy(&cmd.param, &copy_offset, sizeof(DM_COPY_OFFSET));
	return SendCommand(cmd);
}

BOOL DM::DiskMaster::CmdCopyBlock( DM_LBA_RANGE &range )
{
	DMT_TRACE("\n CopyBlock\n");
	assert(opened);
	assert((current_task == kTaskUsb1Sata1Copy) || (current_task == kTaskUsb1Usb2Copy));

	CMD_COPY_BLOCK cmd;
	memset(&task_info, 0x00, sizeof(DM_TASK_INFO));
	memcpy(&cmd.param, &range, sizeof(range));
	if (SendCommand(cmd)) {
		task_in_progress = TRUE;
		return WaitForTaskEnd();
	}
	else return FALSE;
}

BOOL DM::DiskMaster::CmdTestBlock( DM_LBA_RANGE &range )
{
	DMT_TRACE("\n TestBlock\n");
	assert(opened);
	assert( (current_task == kTaskUsb1Read) ||
		(current_task == kTaskUsb2Read) ||
		(current_task == kTaskSata1Read) ||
		(current_task == kTaskSata1Verify) );

	CMD_TEST_BLOCK cmd;
	memset(&task_info, 0x00, sizeof(DM_TASK_INFO));
	memcpy(&cmd.param, &range, sizeof(range));
	if (SendCommand(cmd)) {
		task_in_progress = TRUE;
		return WaitForTaskEnd();
	}
	else return FALSE;
}

BOOL DM::DiskMaster::CmdEraseBlock( DM_LBA_RANGE &range )
{
	DMT_TRACE("\n EraseBlock\n");
	assert(opened);
	assert( (current_task == kTaskUsb2_00_Erase) ||
		(current_task == kTaskUsb2_FF_Erase) ||
		(current_task == kTaskUsb2_Random_Erase) ||
		(current_task == kTaskSata1_00_Erase) ||
		(current_task == kTaskSata1_FF_Erase) ||
		(current_task == kTaskSata1_Random_Erase) );

	CMD_ERASE_BLOCK cmd;
	memset(&task_info, 0x00, sizeof(DM_TASK_INFO));
	memcpy(&cmd.param, &range, sizeof(range));
	if (SendCommand(cmd)) {
		task_in_progress = TRUE;
		return WaitForTaskEnd();
	}
	else return FALSE;
}

BOOL DM::DiskMaster::WaitForTaskEnd()
{
	DM_LBA bad;
	ULONGLONG curr_lba = 0;
	DM_CMD_MSG_HEADER hdr;
	DWORD tick = ::GetTickCount();
	DMT_TRACE("\n Wait for TASK INFO: \n");
	while (TRUE) {
		if (sizeof(DM_CMD_MSG_HEADER) == io->Read(&hdr, sizeof(DM_CMD_MSG_HEADER))) {
			tick = ::GetTickCount();
			switch(hdr.code){
				case kMsgTaskInfo:
					if (sizeof(DM_TASK_INFO) == io->Read(&task_info, sizeof(DM_TASK_INFO))) {
						memcpy(&curr_lba, &task_info.Lba, sizeof(DM_LBA));
						switch (task_info.End_code) {
							case kEndCodeTaskNotEnd:
								this->Notify(kTaskInProgress, &task_info);
								DMT_TRACE("  LBA: %lld %dd:%dh:%dm:%ds\n", curr_lba, task_info.TimeCnt_1d, task_info.TimeCnt_1h, task_info.TimeCnt_1m, task_info.TimeCnt_1s);
								break;

							case kEndCodeTaskEnd:
								// После получения этого сообщения можно продолжить посылать
								// блочные команды в соответствии с кодом задачи
								this->Notify(kTaskComplete, &task_info);
								task_in_progress = FALSE;
								DMT_TRACE("  TASK END -- LBA: %lld %dd:%dh:%dm:%ds\n", curr_lba, task_info.TimeCnt_1d, task_info.TimeCnt_1h, task_info.TimeCnt_1m, task_info.TimeCnt_1s);
								return TRUE;

							case kEndCodeTaskBreak:
								// После получения этого сообщения можно продолжить посылать
								// блочные команды в соответствии с кодом задачи
								this->Notify(kTaskBreak, &task_info);
								task_in_progress = FALSE;
								DMT_TRACE("  Task BREAK LBA: %lld %dd:%dh:%dm:%ds\n", curr_lba, task_info.TimeCnt_1d, task_info.TimeCnt_1h, task_info.TimeCnt_1m, task_info.TimeCnt_1s);
								return FALSE;

							case kEndCodeTaskCrash:
							case kEndCodeTaskShkPwrEnd:
							case kEndCodeTaskCrcErrorEnd:
								//
								// В данном месте нада уведомить всех подписчиков о завершении таска,
								// и ОБЯЗАТЕЛЬНО ДО УСТАНОВКИ current_task = kTaskNone,  
								// т.к. подписчики обязательно захотят узнать какой именно таск закончился.
								//
								this->Notify(kTaskError, &task_info);
								current_task = kTaskNone;
								task_in_progress = FALSE;
								DMT_TRACE(" Task ERROR: %d\n", task_info.End_code);
								return FALSE;
							default:
								return FALSE;
						}
					}
					break;
				case kMsgBadLba:
					io->Read(&bad, sizeof(DM_LBA));
					memcpy(&last_bad, &bad, sizeof(DM_LBA));
					this->Notify(kBadBlock, &last_bad);
					DMT_TRACE("  BAD: %lld\n", last_bad);
					break;
				default:
					DMException();
			}
		}
		if ((::GetTickCount() - tick) > (DWORD)DM_RESPONSE_TIMEOUT)
			throw DMException();
	}
	return TRUE;
}

BOOL DM::DiskMaster::CmdReadLbaHdd1(BYTE *buff, ULONGLONG &lba, DWORD lba_size)
{
	DMT_TRACE("\n ReadLbaHdd1\n");
	assert(opened);
	assert( (current_task == kTaskUsb1Sata1Copy) ||
			(current_task == kTaskUsb1Usb2Copy) ||
			(current_task == kTaskUsb1Read) );

	CMD_COMPLETE_CODE complete_code = 0;
	CMD_READ_LBA_HDD1 cmd;
	cmd.param = lba;
	if (SendCommand(cmd)) {
		DM_CMD_MSG_HEADER hdr;
		if (sizeof(DM_CMD_MSG_HEADER) == io->Read(&hdr, sizeof(DM_CMD_MSG_HEADER))) {
			assert(hdr.code == kMsgReadLbaData);
			assert(hdr.param_size == (lba_size + sizeof(CMD_COMPLETE_CODE)));
			if ((hdr.param_size - sizeof(CMD_COMPLETE_CODE)) == io->Read(buff, hdr.param_size - sizeof(CMD_COMPLETE_CODE)))
				if (sizeof(CMD_COMPLETE_CODE) == io->Read(&complete_code, sizeof(CMD_COMPLETE_CODE)))
					if (complete_code == kReadWriteEnd)
						return TRUE;
		}
	}
	return FALSE;
}

BOOL DM::DiskMaster::CmdReadLbaHdd2(BYTE *buff, ULONGLONG &lba, DWORD lba_size)
{
	DMT_TRACE("\n ReadLbaHdd2\n");
	assert(opened);
	assert( (current_task == kTaskUsb1Sata1Copy) ||
			(current_task == kTaskUsb1Usb2Copy) ||
			(current_task == kTaskUsb2Read) ||
			(current_task == kTaskSata1Read) ||
			(current_task == kTaskSata1Verify) );

	CMD_COMPLETE_CODE complete_code = 0;
	CMD_READ_LBA_HDD2 cmd;
	cmd.param = lba;
	if (SendCommand(cmd)) {
		DM_CMD_MSG_HEADER hdr;
		if (sizeof(DM_CMD_MSG_HEADER) == io->Read(&hdr, sizeof(DM_CMD_MSG_HEADER))) {
			assert(hdr.code == kMsgReadLbaData);
			assert(hdr.param_size == (lba_size + sizeof(CMD_COMPLETE_CODE)));
			if ((hdr.param_size - sizeof(CMD_COMPLETE_CODE)) == io->Read(buff, hdr.param_size - sizeof(CMD_COMPLETE_CODE)))
				if (sizeof(CMD_COMPLETE_CODE) == io->Read(&complete_code, sizeof(CMD_COMPLETE_CODE)))
					if (complete_code == kReadWriteEnd)
						return TRUE;
		}
	}
	return FALSE;
}

BOOL DM::DiskMaster::CmdWriteLbaHdd2(BYTE *buff, ULONGLONG &lba, DWORD lba_size)
{
	DMT_TRACE("\n WriteLbaHdd2\n");
	assert(opened);
	assert( (current_task == kTaskUsb1Sata1Copy) ||
			(current_task == kTaskUsb1Usb2Copy) ||
			(current_task == kTaskUsb2Read) ||
			(current_task == kTaskSata1Read) ||
			(current_task == kTaskSata1Verify) );

	BYTE *write_cmd = new BYTE[sizeof(DM_CMD_MSG_HEADER) + sizeof(DM_LBA) + lba_size];
	((PDM_CMD_MSG_HEADER)write_cmd)->code = kCmdWriteLbaHdd2;
	((PDM_CMD_MSG_HEADER)write_cmd)->param_size = (WORD)(sizeof(DM_LBA) + lba_size);
	memcpy(&write_cmd[sizeof(DM_CMD_MSG_HEADER)], &lba, sizeof(DM_LBA));
	memcpy(&write_cmd[sizeof(DM_CMD_MSG_HEADER) + sizeof(DM_LBA)], buff, lba_size);
	if (SendCommand(*(PDM_CMD_MSG_HEADER)write_cmd)) {
		MSG_END_WRITE_LBA msg;
		if (sizeof(MSG_END_WRITE_LBA) == io->Read(&msg, sizeof(MSG_END_WRITE_LBA))) {
			assert(msg.code == kMsgEndWriteLba);
			assert(msg.param_size == sizeof(CMD_COMPLETE_CODE));
			if (msg.param == kReadWriteEnd)
				return TRUE;
		}
	}
	return FALSE;
}

void DM::DiskMaster::CmdTaskBreak( void )
{
	DMT_TRACE("\n TaskBreak\n");
	assert(opened);
	assert((last_cmd == kCmdCopyBlock) || (last_cmd == kCmdTestBlock) || (last_cmd == kCmdEraseBlock));

	BYTE break_code = 0xFF;
	DM_CMD_MSG_HEADER hdr;
	if (sizeof(break_code) == io->Write(&break_code, sizeof(break_code))) {
		task_in_progress = FALSE;
		::Sleep(500);
	}
}

WORD DM::DiskMaster::Task()
{
	return current_task;
}

DWORD DM::DiskMaster::Command( void )
{
	return last_cmd;
}

void DM::DiskMaster::InitializeOptionWithDefaultValues(DM::DM_OPTION *dm_option)
{
	assert(dm_option);

	dm_option->NumRepeatRd = DM_DEFAULT_OPT_READ_COUNT;
	dm_option->RdTimeLimit = DM_DEFAULT_OPT_READ_TIMEOUT;
	dm_option->ShkPwrLimit = DM_DEFAULT_OPT_SHAKE_POWER_LIMIT;
	dm_option->Chirp = DM_DEFAULT_OPT_CHIRP;
	dm_option->CrcBeep = DM_DEFAULT_OPT_CRC_BEEP;
	dm_option->EndBeep = DM_DEFAULT_OPT_END_BEEP;
	dm_option->BadMarker = DM_DEFAULT_OPT_BAD_MARKER;
}

BOOL DM::DiskMaster::IsValidOption(DM::DM_OPTION *dm_option)
{
	assert(dm_option);
	if ((dm_option->NumRepeatRd < DM_OPTION_READ_COUNT_MIN) && (dm_option->NumRepeatRd > DM_OPTION_READ_COUNT_MAX))
		return FALSE;
	if((dm_option->RdTimeLimit < DM_OPTION_READ_TIMEOUT_MIN) && (dm_option->RdTimeLimit > DM_OPTION_READ_TIMEOUT_MAX))
		return FALSE;
	if((dm_option->ShkPwrLimit < DM_OPTION_SHAKE_POWER_LIMIT_MIN) && (dm_option->ShkPwrLimit > DM_OPTION_SHAKE_POWER_LIMIT_MAX))
		return FALSE;
	if (!((dm_option->Chirp == DM_ENABLED) || (dm_option->Chirp == DM_DISABLED)))
		return FALSE;
	if (!((dm_option->CrcBeep == DM_ENABLED) || (dm_option->CrcBeep == DM_DISABLED)))
		return FALSE;
	if (!((dm_option->EndBeep == DM_ENABLED) || (dm_option->EndBeep == DM_DISABLED)))
		return FALSE;
	if (!((dm_option->BadMarker == DM_BAD_MARKER_0000) || (dm_option->BadMarker == DM_BAD_MARKER_BAD)))
		return FALSE;
	return TRUE;
}

BOOL DM::DiskMaster::SetDefaultOption(DM::DM_OPTION *dm_option)
{
	assert(dm_option);
	assert(opened);
	InitializeOptionWithDefaultValues(dm_option);
	return SetOption(dm_option);
}

DM::IO *DM::DiskMaster::GetIO( void )
{
	return io;
}

DWORD DM::DiskMaster::GetID( void )
{
	return id;
}

DWORD DM::DiskMaster::GetUniqueID( void )
{
	return unique_id;
}

const char *DM::DiskMaster::GetName(void)
{
	return name;
}

BOOL DM::DiskMaster::Open()
{
	Close();
	Initialize();
	if (CmdCheckReady())
		if (opened = CmdBoardOn())
			if (SetOption(&option))
				return opened;
	return (opened = FALSE);
}

BOOL DM::DiskMaster::Close()
{
	if (opened) {
		RemoveAllDisks();
		if (task_in_progress) CmdTaskBreak();
		opened = !CmdBoardOff();
		return !opened;
	}
	return FALSE;
}

BOOL DM::DiskMaster::IsOpen( void )
{
	return opened;
}

const DM::DM_TASK_INFO *DM::DiskMaster::TaskInfo()
{
	return &task_info;
}

DWORD DM::DiskMaster::PortsCount( void )
{
	return kPortsCount;
}

const DM::PORT *DM::DiskMaster::Port( DWORD index )
{
	return &ports[index];
}

DWORD DM::DiskMaster::Rescan( void )
{
	if (opened) {
		DWORD counter = 0;
		if (CmdSendTask(kTaskSata1Read)) counter++;
		if (CmdSendTask(kTaskUsb1Read)) counter++;
		if (CmdSendTask(kTaskUsb2Read)) counter++;
		return counter;
	}
	return 0;
}

DM::Disk *DM::DiskMaster::Rescan( DWORD port_number )
{
	assert(port_number < kPortsCount);
	if (opened) {
		switch (port_number) {
			case kUsb1:
				if (CmdSendTask(kTaskUsb1Read))
					return disks[kUsb1];
				break;
			case kUsb2:
				if (CmdSendTask(kTaskUsb2Read))
					return disks[kUsb2];
				break;
			case kSata1:
				if (CmdSendTask(kTaskSata1Read))
					return disks[kSata1];
				break;
		}
	}
	return NULL;
}

DM::Disk *DM::DiskMaster::GetDisk( DWORD port_number )
{
	assert(port_number < kPortsCount);
	if (opened)
		return disks[port_number];
	else
		return NULL;
}

BOOL DM::DiskMaster::Copy( DWORD src_port, DWORD dst_port, DWORD param )
{
	assert(src_port == kUsb1);
	assert((dst_port == kUsb2) || (dst_port == kSata1));

	if (!opened) return FALSE;

	BYTE task_code = 0;
	if (dst_port == kUsb2)
		task_code = kTaskUsb1Usb2Copy;
	else if (dst_port == kSata1)
		task_code = kTaskUsb1Sata1Copy;
	if (CmdSendTask(task_code)) {
		if (disks[src_port]->Size() <= disks[dst_port]->Size()) {
			DM_LBA_RANGE dm_range;
			dm_range.start = 0;
			dm_range.end = disks[src_port]->Size() - 1;
			return CmdCopyBlock(dm_range);
		}
		else
			return FALSE; // Destination Disk too small
	}
	return FALSE;
}

BOOL DM::DiskMaster::CopyEx( DWORD src_port, DWORD dst_port, ULONGLONG &src_offset, ULONGLONG &dst_offset, ULONGLONG &count, DWORD param )
{
	assert(src_port == kUsb1);
	assert((dst_port == kUsb2) || (dst_port == kSata1));

	if (!opened) return FALSE;

	BYTE task_code = 0;
	if (dst_port == kUsb2)
		task_code = kTaskUsb1Usb2Copy;
	else if (dst_port == kSata1)
		task_code = kTaskUsb1Sata1Copy;

	if (current_task != task_code)
		if (!CmdSendTask(task_code))
			return FALSE;

	if (count) {
		if (((src_offset + count) <= disks[src_port]->Size()) && ((dst_offset + count) <= disks[dst_port]->Size())) {
			DM_COPY_OFFSET dm_offs;
			if (src_offset > dst_offset) {
				dm_offs.direction = kBackOffset;
				dm_offs.offset = src_offset - dst_offset;
			}
			else {
				dm_offs.direction = kForwardOffset;
				dm_offs.offset = dst_offset - src_offset;
			}

			if (CmdSetCopyOffset(dm_offs)) {
				DM_LBA_RANGE dm_range;
				dm_range.start = src_offset;
				dm_range.end = src_offset + count - 1;
				return CmdCopyBlock(dm_range);
			}
		}
	}

	return FALSE;
}

BOOL DM::DiskMaster::Erase( DWORD port, DWORD param )
{
	if (opened && ((port == kUsb2) || (port == kSata1))) {
		BYTE task_code;
		if (port == kUsb2) {
			switch (param) {
						case kErasePattern_00:
							task_code = kTaskUsb2_00_Erase;
							break;
						case kErasePattern_FF:
							task_code = kTaskUsb2_FF_Erase;
							break;
						case kErasePattern_Random:
							task_code = kTaskUsb2_Random_Erase;
							break;
						default :
							return FALSE;
			}
		}
		else if (port == kSata1) {
			switch (param) {
						case kErasePattern_00:
							task_code = kTaskSata1_00_Erase;
							break;
						case kErasePattern_FF:
							task_code = kTaskSata1_FF_Erase;
							break;
						case kErasePattern_Random:
							task_code = kTaskSata1_Random_Erase;
							break;
						default :
							return FALSE;
			}
		}
		else return FALSE;

		if (current_task != task_code)
			if (!CmdSendTask(task_code))
				return FALSE;

		DM_LBA_RANGE range;
		range.start = (ULONGLONG)0;
		range.end = disks[port]->Size() - 1;
		return CmdEraseBlock(range);
	}
	return FALSE;
}

BOOL DM::DiskMaster::EraseEx( DWORD port, ULONGLONG &offset, ULONGLONG &count, DWORD param )
{
	if (opened && ((port == kUsb2) || (port == kSata1))) {
		BYTE task_code;
		if (port == kUsb2) {
			switch (param) {
						case kErasePattern_00:
							task_code = kTaskUsb2_00_Erase;
							break;
						case kErasePattern_FF:
							task_code = kTaskUsb2_FF_Erase;
							break;
						case kErasePattern_Random:
							task_code = kTaskUsb2_Random_Erase;
							break;
						default :
							return FALSE;
			}
		}
		else if (port == kSata1) {
			switch (param) {
						case kErasePattern_00:
							task_code = kTaskSata1_00_Erase;
							break;
						case kErasePattern_FF:
							task_code = kTaskSata1_FF_Erase;
							break;
						case kErasePattern_Random:
							task_code = kTaskSata1_Random_Erase;
							break;
						default :
							return FALSE;
			}
		}
		else return FALSE;

		if (current_task != task_code)
			if (!CmdSendTask(task_code))
				return FALSE;

		if (count) {
			if ((offset + count) <= disks[port]->Size()) {
				DM_LBA_RANGE range;
				range.start = offset;
				range.end = offset + count - 1;
				return CmdEraseBlock(range);
			}
		}
	}
	return FALSE;
}

BOOL DM::DiskMaster::Test( DWORD port, DWORD param )
{
	if (opened) {
		BYTE task_code;
		if (port == kUsb1) {
			task_code = kTaskUsb1Read;
		} else if (port == kUsb2) {
			task_code = kTaskUsb2Read;
		} else if (port == kSata1) {
			task_code = kTaskSata1Verify;
		} else return FALSE;

		if (current_task != task_code)
			if (!CmdSendTask(task_code))
				return FALSE;

		DM_LBA_RANGE range;
		range.start = (ULONGLONG)0;
		range.end = (ULONGLONG)disks[port]->Size() - 1;
		return CmdTestBlock(range);
	}
	return FALSE;
}

BOOL DM::DiskMaster::TestEx( DWORD port, ULONGLONG &offset, ULONGLONG &count, DWORD param )
{
	if (opened) {
		BYTE task_code;
		if (port == kUsb1) {
			task_code = kTaskUsb1Read;
		} else if (port == kUsb2) {
			task_code = kTaskUsb2Read;
		} else if (port == kSata1) {
			task_code = kTaskSata1Verify;
		} else return FALSE;

		if (current_task != task_code)
			if (!CmdSendTask(task_code))
				return FALSE;

		if (count) {
			if ((offset + count) <= disks[port]->Size()) {
				DM_LBA_RANGE range;
				range.start = offset;
				range.end = offset + count - 1;
				return CmdTestBlock(range);
			}
		}
	}
	return FALSE;
}

void DM::DiskMaster::Break(void)
{
	if (task_in_progress) {
		CmdTaskBreak();
	}
}

BOOL DM::DiskMaster::ReadBlock(DWORD port, ULONGLONG &offset, BYTE *buff, DWORD block_size)
{
	assert(buff);
	assert(block_size);
	assert(port < kPortsCount);

	if (opened) {
		BYTE task_code;
		if (port == kUsb1) {
			task_code = kTaskUsb1Read;
		} else if (port == kUsb2) {
			task_code = kTaskUsb2Read;
		} else if (port == kSata1) {
			task_code = kTaskSata1Read;
		} else return FALSE;

		if (current_task != task_code)
			if (!CmdSendTask(task_code))
				return FALSE;

		if (disks[port]->block_size == block_size) {
			if (port == kUsb1)
				return CmdReadLbaHdd1(buff, offset, block_size);
			else
				return CmdReadLbaHdd2(buff, offset, block_size);
		}
	}
	return FALSE;
}

BOOL DM::DiskMaster::WriteBlock(DWORD port, ULONGLONG &offset, BYTE *buff, DWORD block_size)
{
	assert(buff);
	assert(block_size);
	assert(port < kPortsCount);

	if (opened) {
		BYTE task_code;
		if (port == kUsb2) {
			task_code = kTaskUsb2Read;
		} else if (port == kSata1) {
			task_code = kTaskSata1Read;
		} else return FALSE;

		if (current_task != task_code)
			if (!CmdSendTask(task_code))
				return FALSE;

		if (disks[port]->block_size == block_size)
			return CmdWriteLbaHdd2(buff, offset, block_size);
	}
	return FALSE;
}

BOOL DM::DiskMaster::SetOption(DM::DM_OPTION *dm_option)
{
	assert(dm_option);
	assert(opened);
	if(IsValidOption(dm_option)) {
		if(CmdSendOption(dm_option)) {
			memcpy(&option, dm_option, sizeof(DM::DM_OPTION));
			return TRUE;
		}
	}
	return FALSE;
}

void DM::DiskMaster::GetOption(DM::DM_OPTION *dm_option)
{
	assert(dm_option);
	assert(opened);
	memcpy(dm_option, &option, sizeof(DM::DM_OPTION));
}
