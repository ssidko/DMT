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
//#include <winioctl.h>
//#include "ntddcdrm.h"

using namespace DM;

int _tmain(int argc, _TCHAR* argv[])
{
	BOOL ret = FALSE;
	DiskMaster *dm = NULL;
	DiskMasterManager *dm_mgr = DM::GetDiskMasterManager();

	int x = 0;
	ULONGLONG size = 0;
	Disk *disk = NULL;
	if (dm_mgr->Rescan()) {
		dm = dm_mgr->GetDiskMaster(0);
		if (dm->Open()) {
			disk = dm->Rescan(kSata1);
			size = disk->Size();
			size = disk->NativeSize();
			disk->SetSize(size);
			size = disk->Size();
			
			x++;
			dm->Close();
		}		
		x--;
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

