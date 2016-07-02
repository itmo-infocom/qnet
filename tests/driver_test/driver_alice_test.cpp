//Данный тест написан для экспериментов работы с написанным драйвером
using namespace std;

#include <iostream>

#include "../../kks_console/driverAnB/devel/AnBDefs.h"
#include "../../kks_console/driverAnB/devel/driver.h"

using namespace AnB;

int main(void)
{
	//Инициализируем устройства
	device_list devices;
  
	if (!AnBDriverAPI::GetDevicesList(devices))
	{
		cerr << "Cannot initialize device_lsit" << endl;
		return EXIT_FAILURE;
	}
  AnBDriverAPI *alice = devices.array[0].dev_ref;

  cout << "Devices initialized: " << devices.count << endl;

	cout << "Дамп до установки режима платы \"Алиса\"" << endl;
	cout << alice->GetDump(DumpSources::BAR0) << endl;

	if (!alice->SetDev(AnBDevices::Alice))
	{
		cerr << "Cannot set device mode" << endl;
		cerr << alice->LastError() << endl;
		return EXIT_FAILURE;
	}
	
	cout << "Дамп после установки режима платы \"Алиса\"" << endl;
	cout << alice->GetDump(DumpSources::BAR0) << endl;
}
