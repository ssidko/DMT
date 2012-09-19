#ifndef _D2XX
#define _D2XX

#include <windows.h>
#include <dbt.h>
#include <vector>
#include <stdio.h>
#include <assert.h>
#include "ftd2xx.h"
#include "FTChipID.h"
#include "Utilities.h"

#pragma comment(lib, "ftd2xx.lib")
#pragma comment(lib, "FTChipID.lib")

namespace DM 
{
	#define FT_OPEN_BY_INDEX							8

	#define FT_READ_TIMEOUT								1000
	#define FT_WRITE_TIMEOUT							1000
	#define FT_BAUD_RATE								FT_BAUD_115200
	#define FT_WORD_LENGTH								FT_BITS_8
	#define FT_STOP_BITS								FT_STOP_BITS_1
	#define FT_PARITY									FT_PARITY_NONE

	#define D2XX_MANAGER_RESCAN_TIMEOUT					(DWORD)15000
	#define D2XX_MANAGER_RESCAN_SLEEP_TIMEOUT			(DWORD)500

	#define HARDWARE_MONITOR_CLASS_NAME					"Hardware PnP Monitor CLASS"
	#define HARDWARE_MONITOR_WND_NAME					"Hardware PnP Monitor WND"

	class D2XXManager;
	class HardwareMonitor;

	D2XXManager *GetD2XXManager(void);
	HardwareMonitor *GetHardwareMonitor(void);

	class HardwareMonitor : public Subject
	{
	private:
		HWND wnd;
		static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			PDEV_BROADCAST_HDR header = (PDEV_BROADCAST_HDR)lParam;
			if (uMsg == WM_DEVICECHANGE) {
				D2XXManager *ftdi_mgr = GetD2XXManager();
				switch (wParam) {
					case DBT_DEVICEARRIVAL :
						if (header->dbch_devicetype == DBT_DEVTYP_PORT)
							DM::GetHardwareMonitor()->Notify();
						break;
					case DBT_DEVICEREMOVECOMPLETE :
						if (header->dbch_devicetype == DBT_DEVTYP_PORT)
							DM::GetHardwareMonitor()->Notify();
						break;
				}
				return TRUE;
			}
			else return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}

		void RegisterWindowClass(void)
		{
			HINSTANCE hinst = ::GetModuleHandle(NULL);
			WNDCLASSEXA wce;
			memset(&wce, 0x00, sizeof(WNDCLASSEX));

			wce.cbSize = sizeof(wce);
			wce.lpfnWndProc = HardwareMonitor::StaticWindowProc;
			wce.hInstance = hinst;
			wce.lpszClassName = HARDWARE_MONITOR_CLASS_NAME;

			if (!::RegisterClassExA(&wce)) {
				if ( ERROR_CLASS_ALREADY_EXISTS != ::GetLastError())
					throw DMException();
			}			
		}

		void UnregisterWindowClass(void)
		{
			DestroyWindow();
			::UnregisterClassA(HARDWARE_MONITOR_CLASS_NAME, ::GetModuleHandle(NULL));
		}

		void CraeteWindow(void)
		{
			HINSTANCE hinst = ::GetModuleHandle(NULL);
			wnd = ::CreateWindowA(HARDWARE_MONITOR_CLASS_NAME, HARDWARE_MONITOR_WND_NAME, 0, 
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hinst, NULL);
			if (!wnd) throw DMException();
		}

		void DestroyWindow(void)
		{
			if (wnd) {
				::DestroyWindow(wnd);
				wnd = NULL;
			}
		}

		void Initialise(void)
		{
			RegisterWindowClass();
			CraeteWindow();
		}

	public:
		HardwareMonitor(void) : wnd(NULL)
		{
			Initialise();
		}

		~HardwareMonitor(void) 
		{
			UnregisterWindowClass();
		}
	};

	class D2XXDevice : public IO
	{
	private:
		FT_HANDLE handle;
		FT_STATUS ft_status;
		FT_DEVICE_LIST_INFO_NODE info;

		BOOL OpenEx(PVOID arg, DWORD flag)
		{
			Close();
			if (flag & FT_OPEN_BY_INDEX) ft_status = FT_Open((int)arg, &handle);
			else ft_status = FT_OpenEx(arg, flag, &handle);
			if (FT_SUCCESS(ft_status)) info.Flags |= FT_FLAGS_OPENED;
			return FT_SUCCESS(ft_status);
		}

		BOOL OpenByIndex(DWORD index)
		{
			return OpenEx((PVOID)index, FT_OPEN_BY_INDEX);
		}

		BOOL OpenBySerialNumber(const char *serial_number)
		{
			return OpenEx((PVOID)serial_number, FT_OPEN_BY_SERIAL_NUMBER);
		}

		BOOL OpenByDescription(const char *description)
		{
			return OpenEx((PVOID)description, FT_OPEN_BY_DESCRIPTION);
		}

		BOOL OpenByLocation(DWORD location)
		{
			return OpenEx((PVOID)location, FT_OPEN_BY_LOCATION);
		}

	public:
		D2XXDevice(const FT_DEVICE_LIST_INFO_NODE *device_info) : handle(NULL), ft_status(FT_DEVICE_NOT_OPENED)
		{
			assert(device_info);
			memcpy(&info, device_info, sizeof(FT_DEVICE_LIST_INFO_NODE));
		}

		~D2XXDevice() {Close();}

		virtual BOOL Open()
		{
			return OpenEx(info.SerialNumber, FT_OPEN_BY_SERIAL_NUMBER);
		}

		virtual void Close(void)
		{
			if (handle) {
				if (FT_SUCCESS(ft_status = FT_Close(handle)))
					info.Flags &= ~FT_FLAGS_OPENED;
				handle = NULL;
				ft_status = FT_DEVICE_NOT_OPENED;
			}	
		}

		virtual DWORD Read(void *buffer, DWORD count)
		{
			DWORD rw = 0;
			DMT_TRACE("\tReading %d bytes: ", count);
			if (FT_SUCCESS(ft_status = FT_Read(handle, buffer, count, &rw))) {
				DMT_TRACE(" (%d)\n", rw);
				return rw;
			}
			else {
				printf(" ERROR\n");
				return 0;
			} 
		}

		virtual DWORD Write(void *buffer, DWORD count)
		{
			DWORD rw = 0;
			DMT_TRACE("\tWriting %d bytes: ", count);
			if (FT_SUCCESS(ft_status = FT_Write(handle, buffer, count, &rw))) {
				DMT_TRACE(" (%d)\n", rw);				
				return rw;
			}
			else {
				printf(" ERROR\n");
				return 0;
			}
		}

		BOOL IsOpen()
		{
			return (handle != NULL);
		}

		BOOL Initialise(void)
		{
			BOOL ret = TRUE;
			ret &= SetTimeouts(FT_READ_TIMEOUT, FT_WRITE_TIMEOUT);
			ret &= SetBaudRate(FT_BAUD_RATE);
			ret &= SetDataCharacteristics(FT_WORD_LENGTH, FT_STOP_BITS_1, FT_PARITY_NONE);
			return ret;
		}

		BOOL SetTimeouts(DWORD read_timeout, DWORD write_timeout)
		{
			return (FT_SUCCESS(ft_status = FT_SetTimeouts(handle, read_timeout, write_timeout)));
		}

		BOOL SetBaudRate(DWORD baud_rate)
		{
			return (FT_SUCCESS(ft_status = FT_SetBaudRate(handle, baud_rate)));
		}

		BOOL SetDataCharacteristics(BYTE word_length, BYTE stop_bits, BYTE parity)
		{
			return (FT_SUCCESS(ft_status = FT_SetDataCharacteristics(handle, word_length, stop_bits, parity)));
		}

		BOOL GetQueueStatus(DWORD *rx_bytes, DWORD *tx_bytes, DWORD *event_status)
		{
			return FT_SUCCESS(ft_status = FT_GetStatus(handle, rx_bytes, tx_bytes, event_status));
		}

		BOOL GetUniqueChipID(DWORD *chip_id)
		{
			return FT_SUCCESS(ft_status = FTID_GetChipIDFromHandle(handle, chip_id));
		}

		DWORD FT_Status(void)
		{
			return ft_status;
		}

		DWORD GetType(void)
		{
			return info.Type;
		}

		DWORD GetVIDPID(void)
		{
			return info.ID;
		}

		DWORD GetLocationID(void)
		{
			return info.LocId;
		}

		const char *GetSerialNumber(void)
		{
			return info.SerialNumber;
		}

		const char *GetDescription(void)
		{
			return info.Description;
		}

		void DisplayInfo(void)
		{
			char *dev_type_str[] = {"232BM", "232AM", "100AX", "UNKNOWN", "2232C", "232R", "2232H", "4232H", "232H"};

			printf("%18s%s\n", "FT Device type: ", dev_type_str[info.Type]);
			printf("%18s%s\n", "Serial number: ", info.SerialNumber);
			printf("%18s%s\n", "Description: ", info.Description);
			printf("%18s0x%08X\n", "VID&PID: ", info.ID);
			printf("%18s%d\n", "Is opened: ", (info.Flags & FT_FLAGS_OPENED));
			printf("%18s0x%08X\n", "Handle: ", info.ftHandle);
			printf("%18s0x%08X\n", "Location ID: ", info.LocId);
		}
	};

	typedef std::vector<D2XXDevice *> D2XXDevicesList;

	class D2XXManager : public ManagerNotifier, public Observer
	{
	private:
		DWORD dev_count;
		FT_STATUS ft_status;
		D2XXDevicesList dev_list;

		BOOL IsValidDeiviceInfo(FT_DEVICE_LIST_INFO_NODE *dev_info)
		{
			assert (dev_info);
			if (strlen(dev_info->SerialNumber)) {
				if (dev_info->Flags & FT_FLAGS_OPENED)
					return TRUE;
				else if (dev_info->LocId)
					return TRUE;
			}
			return FALSE;
		}

		DWORD CreateDeviceList(D2XXDevicesList *list)
		{
			BOOL all_dev_valid = TRUE;
			DWORD deice_count = 0;
			D2XXDevice *device = NULL;
			FT_DEVICE_LIST_INFO_NODE *ft_info = NULL;
			FT_DEVICE_LIST_INFO_NODE *ft_info_list = NULL;
			DWORD tick_count = ::GetTickCount();

			do {
				CleanList(list);
				all_dev_valid = TRUE;
				if (FT_SUCCESS(ft_status = FT_CreateDeviceInfoList(&deice_count))) {
					if (deice_count) {
						ft_info_list = new FT_DEVICE_LIST_INFO_NODE[deice_count*sizeof(FT_DEVICE_LIST_INFO_NODE)];
						memset(ft_info_list, 0x00, deice_count*sizeof(FT_DEVICE_LIST_INFO_NODE));
						if (FT_SUCCESS(ft_status = FT_GetDeviceInfoList(ft_info_list, &deice_count))) {
							ft_info = ft_info_list;
							for (DWORD i = 0; i < deice_count; i++, ft_info++) {
								if (IsValidDeiviceInfo(ft_info)) {
									device = new D2XXDevice(ft_info);
									list->push_back(device);
								}
								else all_dev_valid = FALSE;
							}
						}
						delete[] ft_info_list;
					}
				}
				if (!all_dev_valid) ::Sleep(D2XX_MANAGER_RESCAN_SLEEP_TIMEOUT);
			} while (!(all_dev_valid || ((::GetTickCount() - tick_count) > D2XX_MANAGER_RESCAN_TIMEOUT)));
			return list->size();
		}

		void CleanList(D2XXDevicesList *list)
		{
			D2XXDevicesList::iterator it;
			for (it = list->begin(); it != list->end(); it++)
				delete *it;
			list->clear();
		}

	public:
		D2XXManager() : dev_count(0), ft_status(FT_OK)
		{
			DM::GetHardwareMonitor()->Attach(this);
		}

		~D2XXManager()
		{
			CleanList(&dev_list);
			DM::GetHardwareMonitor()->Detach(this);
		}

		virtual void Update(Subject *subject)
		{
			if (subject == DM::GetHardwareMonitor()) Rescan();
		}

		DWORD Count(void)
		{
			return dev_count;
		}

		DWORD Rescan(void)
		{
			BOOL find = FALSE;
			D2XXDevicesList::iterator it;
			D2XXDevicesList::iterator tmp_it;
			D2XXDevicesList tmp_list;
			DWORD tmp_count = CreateDeviceList(&tmp_list);
			for (tmp_it = tmp_list.begin(); tmp_it != tmp_list.end(); tmp_it++) {
				for (it = dev_list.begin(); it != dev_list.end(); it++) {
					if (!strcmp((*tmp_it)->GetSerialNumber(), (*it)->GetSerialNumber())) {
						find = TRUE;
						break;	
					}					
				}
				if (!find) {
					dev_list.push_back(*tmp_it);
					Notify(*tmp_it, kPlugged);
				}
				find = FALSE;
			}
			for (it = dev_list.begin(); it != dev_list.end(); it++) {
				for (tmp_it = tmp_list.begin(); tmp_it != tmp_list.end(); tmp_it++) {
					if (!strcmp((*it)->GetSerialNumber(), (*tmp_it)->GetSerialNumber())) {
						find = TRUE;
						break;
					}
				}
				if (!find) {
					Notify(*it, kUnplugged);
					delete *it;
					it = dev_list.erase(it);
					if (it == dev_list.end()) break;
				}
				find = FALSE;
			}
			return dev_count = dev_list.size();
		}

		D2XXDevice *GetDevice(DWORD index)
		{
			return dev_list.at(index);
		}

		DWORD FT_Status(void)
		{
			return ft_status;
		}

		void DisplayDevicesInfo(void)
		{
			printf("===================================\n");
			printf("Devices: %d\n", dev_count);
			printf("===================================\n");
			for (DWORD i = 0; i < dev_count; i++) {
				dev_list.at(i)->DisplayInfo();
				printf("===================================\n");
			}
		}
	};
}

#endif // _D2XX
