#ifndef _DISK_MASTER_MANAGER
#define _DISK_MASTER_MANAGER

#include <vector>
#include "D2xx.h"
#include "DiskMaster.h"

namespace DM 
{
	#define DISK_MASTER_PORTABLE_USB_VIDPID				(DWORD)0x04036001

	typedef std::vector<DiskMaster *> DiskMastersList;

	class DiskMasterManager : public ManagerNotifier, public ManagerObserver
	{
	private:
		DiskMastersList dm_list;
		DWORD next_id;

		DWORD GenerateNewID(void)
		{
			return ++next_id;
		}

		BOOL Add(DiskMaster *dm)
		{
			if (dm) {
				dm_list.push_back(dm);
				Notify(dm, kPlugged);
			}
			return FALSE;
		}

		BOOL Remove(IO *ftdi)
		{
			if (ftdi) {
				DiskMastersList::iterator it = dm_list.begin();
				while (it != dm_list.end()) {
					if (ftdi == (*it)->GetIO()) {
						Notify(*it, kUnplugged);
						delete *it;
						dm_list.erase(it);
						return TRUE;
					}
					++it;
				}
			}
			return FALSE;
		}

		void CleanList(DiskMastersList *list)
		{
			DiskMastersList::iterator it;
			for (it = list->begin(); it != list->end(); it++)
				delete *it; 
			list->clear();
		}

	public:
		DiskMasterManager() : next_id(0)
		{
			DM::GetD2XXManager()->Attach(this);
		}

		~DiskMasterManager()
		{
			DM::GetD2XXManager()->Detach(this);
			CleanList(&dm_list);
		}

		virtual void Update(ManagerNotifier *manager, void *device, DWORD event_code)
		{
			if (manager == (ManagerNotifier *)GetD2XXManager()) {
				DWORD dm_id = 0;
				DWORD dm_unique_id = 0;
				D2XXDevice *ft_dev = dynamic_cast<D2XXDevice *>((D2XXDevice *)device);				
				if (ft_dev) {
					BOOL ret = TRUE;
					DiskMaster *dm = NULL;
					switch(event_code) {
						case kPlugged:
							if (ft_dev->Open()) {
								if (ft_dev->GetVIDPID() == DISK_MASTER_PORTABLE_USB_VIDPID) {
									ret &= ft_dev->SetTimeouts(FT_READ_TIMEOUT, FT_WRITE_TIMEOUT);
									ret &= ft_dev->SetBaudRate(FT_BAUD_RATE);
									ret &= ft_dev->SetDataCharacteristics(FT_WORD_LENGTH, FT_STOP_BITS_1, FT_PARITY_NONE);
									dm_id = GenerateNewID();
									ft_dev->GetUniqueChipID(&dm_unique_id);
									if (ret && dm_id && dm_unique_id) {
										dm = new DiskMaster(dm_id, dm_unique_id, ft_dev);
										Add(dm);
									}
								}
							}
							break;
						case kUnplugged:
							Remove((D2XXDevice *)ft_dev);
							break;
					}				
				}
			}
		}

		DWORD Count()
		{
			return dm_list.size();
		}

		DWORD Rescan(void)
		{
			GetD2XXManager()->Rescan();
			return Count();
		}

		DiskMaster *GetDiskMaster(DWORD index)
		{
			return dm_list.at(index);
		}

	};

	DiskMasterManager *GetDiskMasterManager(void);

}

#endif // _DISK_MASTER_MANAGER