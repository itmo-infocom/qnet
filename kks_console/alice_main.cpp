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
try{
	using namespace std;
	board_if::board_if brd;

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
			cout << "alice: Successfully connected with bob_sock" << endl;
			break;
		}//end рабочий цикл

		break;
	}//end Внешний цикл
	catch (int errno)
	{
		cerr << strerror(errno) << endl;
	};
	return EXIT_SUCCESS;
}//end main()
catch(std::string estr)
{
	std::cerr << estr << std::endl; 
	return EXIT_FAILURE;
}
//--------------------------------------------------
//Определения функций Алисы