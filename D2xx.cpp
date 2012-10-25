#include "D2xx.h"

namespace DM
{
	HardwareMonitor theHardwareMonitor;
	D2XXManager theD2XXManager;

	D2XXManager *GetD2XXManager(void) 
	{
		return &theD2XXManager;
	}

	HardwareMonitor *GetHardwareMonitor(void)
	{
		return &theHardwareMonitor;
	}
}
