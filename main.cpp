// dm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>
#include <tchar.h>
#include "windows.h"
#include "d2xx.h"
#include "DiskMaster.h"
#include "DiskMasterManager.h"

#include "abstract.h"

// Working with  cd-rom
#include <winioctl.h>
#include "ntddcdrm.h"

using namespace DM;

int _tmain(int argc, _TCHAR* argv[])
{
	//HANDLE hFile = CreateFile (_T("\\\\.\\CDROM0"), GENERIC_READ,
	//								FILE_SHARE_READ/*|FILE_SHARE_WRITE*/,
	//								NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
	//								NULL);

	//if (hFile != INVALID_HANDLE_VALUE) {
	//	DWORD rw = 0;
	//	BYTE sec[2048] = {0};
	//	::SetFilePointer(hFile, 16*2048, NULL, FILE_BEGIN);
	//	::ReadFile(hFile, sec, 2048, &rw, NULL);
	//	int x = 0;
	//}
	//else {
	//	DWORD err = ::GetLastError();
	//	int x = 0;
	//}

	BOOL ret = FALSE;
	DiskMaster *dm = NULL;
	DiskMasterManager *dm_mgr = DM::GetDiskMasterManager();

	const Disk *disk = NULL;
	if (dm_mgr->Rescan()) {
		dm = dm_mgr->GetDiskMaster(0);
		if (dm->Open()) {
			//DWORD dsk_count = dm->Rescan();
			//disk = dm->Rescan(kSata1);
			//disk = dm->Rescan(kUsb1);

			ULONGLONG src_offs = 2000000;
			ULONGLONG dst_offs = 1000000;
			ULONGLONG count = 2000000;

			//dm->Copy(kUsb1, kUsb2);
			//dm->CopyEx(kUsb1, kSata1, src_offs, dst_offs, count, NULL);
			//dm->Erase(kSata1, kErasePattern_FF);
			//dm->EraseEx(kSata1, src_offs, count, kErasePattern_FF);
			//dm->Test(kSata1, NULL);
			//dm->TestEx(kSata1, src_offs, count, NULL);

			ret = dm->Close();
		}
	}

	_tprintf(_T("\nPress any key for exit ..."));
	_getch();

	////////////////////////////////////////////////////////
	// Message loop
	//
	//MSG msg;
	//memset(&msg, 0x00, sizeof(MSG));
	//while (ret = ::GetMessage(&msg, NULL, 0, 0)) {
	//	if (ret == -1) {
	//		return ::GetLastError();
	//	}
	//	else {
	//		TranslateMessage(&msg); 
	//		DispatchMessage(&msg); 
	//	}
	//}

	return 0;
}

