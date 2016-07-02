//Данный тест написан для экспериментов работы с написанным драйвером

#include <iostream>

#include "../../kks_console/driverAnB/devel/AnBDefs.h"
#include "../../kks_console/driverAnB/devel/driver.h"

int main(void)
{
namespace std
{
namespace AnB
{
	//Инициализируем устройства
	device_list devices;
	if (!AnBDriverAPI::GetDevicesList(devices))
	{
		cerr << "Cannot initialize device_lsit" << endl;
		cerr << AnBDriverAPI::LastError() << endl;
		return EXIT_FAILURE;
	}

	cout << "Дамп до установки режима платы \"Алиса\"" << endl;
	cout << AnBDriverAPI::GetDump(DumpSources::BAR0) << endl;

	if (!AnBDriverAPI::SetDev(AnBDevices::Alice))
	{
		cerr << "Cannot set device mode" << endl;
		cerr << AnBDriverAPI::LastError() << endl;
		return EXIT_FAILURE;
	}
	
	cout << "Дамп после установки режима платы \"Алиса\"" << endl;
	cout << AnBDriverAPI::GetDump(DumpSources::BAR0) << endl;

	
}
}
}
