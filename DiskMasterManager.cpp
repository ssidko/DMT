#include "DiskMasterManager.h"

namespace DM
{
	DiskMasterManager theDiskMasterManager;

	DiskMasterManager *GetDiskMasterManager(void)
	{
		return &theDiskMasterManager;
	}
}



