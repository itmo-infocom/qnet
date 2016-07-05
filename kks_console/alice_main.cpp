//Точка входа консольного приложения Алисы
  #include "header.h"	//Файл с функциями и константами,
				//одинаковыми и для Алисы, и для Боба
//---------------------------------------
//Глобальные переменные и константы

//---------------------------------------
//Прототипы функций
	#include "common.cpp"

//---------------------------------------
//Точка входа
int main( void )
{
	using namespace std;
	using namespace AnB;

	//Инициализация платы

	struct device_list boards;//Здесь хранятся указателя на все обнаруженные устройства
	try{
		if (!AnBDriverAPI::GetDevicesList(boards)) 
		{
			cerr << boards.array[0].dev_ref->LastError() << endl;
		}
	} catch(...)
	{

	}
	AnBDriverAPI &board = *boards.array[0].dev_ref;
	

	while ( true )	//Самый внешний цикл - заставляет повториться всю программу,
					//если что-то пошло не так
	try
	{
    char hostname[] = "localhost4";
    char port[] = "50000";
		NetWork bob( hostname, port );
		//Теперь создадим цикл, в котором будем крутить основной алгоритм
		//работы
		while (true)//рабочий цикл
		{
			std::cout << "alice: Successfully connected with Bob" << std::endl;
			
			break;
		}//end рабочий цикл

		break;
	}//end Внешний цикл
	catch (int errno)
	{
		std::cerr << strerror(errno) << std::endl;
	};
	return EXIT_SUCCESS;
}//end main()

//--------------------------------------------------
//Определения функций Алисы
