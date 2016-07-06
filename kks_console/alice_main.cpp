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
	using namespace board_if;

	while ( true )	//Самый внешний цикл - заставляет повториться всю программу,
					//если что-то пошло не так
	try
	{
		char hostname[] = "localhost4";
		char port[] = "50000";
		//NetWork bob_sock( hostname, port );
		//Теперь создадим цикл, в котором будем крутить основной алгоритм
		//работы
		while (true)//рабочий цикл
		{
			std::cout << "alice: Successfully connected with bob_sock" << std::endl;
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