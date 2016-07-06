//Точка входа консольного приложения Алисы
  #include "header.h"	//Файл с функциями и константами,
				//одинаковыми и для Алисы, и для Боба
//---------------------------------------
//Глобальные переменные и константы

//---------------------------------------
//Прототипы функций
	#include "common.cpp"
	void generation(AnB::AnBDriverAPI &board);

//---------------------------------------
//Точка входа
int main( void )
{
	using namespace std;
	using namespace AnB;

	//Инициализация платы
	
	//Здесь хранятся указатели на все обнаруженные устройства
	struct device_list boards;
	AnBDriverAPI *board;
	while (true)
	{
		try{
			if (!AnBDriverAPI::GetDevicesList(boards)) throw errno;
			board = boards.array[0].dev_ref;
			if (!board->DMADisable())
			{
				if (errno != EINVAL)
				{
					cerr << board->LastError() << endl;
					throw errno;
				} else errno = 0;
			};	
			if (!board->SetDev(Alice)) 
			{
				cerr << board->LastError() << endl;
				board->~AnBDriverAPI();
				continue;
			} else break;
		} catch( int e)
		{
			cerr << boards.array[0].dev_ref->LastError() << endl;
			cerr << strerror(e) << endl;
		}
	};

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
			generation(*board);//, bob_sock);
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

//Алгоритм генерации ключа
void generation(AnB::AnBDriverAPI &board)
{
	AnBRegInfo reg;
	reg.address = RegTable;
	board.RegRawRead(reg);
	reg.value.table.size = 32;
	reg.value.table.mode = Mode2;
	board.RegRawWrite(reg);

	//Сформируем таблицу RNG
	uint8_t val[8];
	for (uint8_t i = 0; i < 8; i++) val[i] = i;
	board.WriteTable((char*)val, reg.value.table.size, TableRNG);

	//std::cout << board.GetDump(BAR0); 
	//board.DMADisable();
	//std::cout << board.GetDump(BAR0);
	//std::cout << "TableRNG" << std::endl;
	//std::cout << board.GetDump(BAR1_RNG);
	//board.DMADisable();
	std::cout << "DMAEnable = " << board.DMAEnable() << std::endl;
	char *buf;
	board.DMARead(buf);

	while (true)
	{
		if (board.DMAIsReady()) board.DMARead(buf);
		else 
		{	
			if (!board.DMADisable()) std::cerr << board.LastError() << std::endl;
			std::cout << "DMA is not ready" << std::endl;
			if (!board.DMAEnable()) std::cerr << board.LastError() << std::endl;
		}
	}	

	for (int i = 0; i < AnBBufferMinSize; i++)
	{
		std::cout << *(buf+i);
	} 
};