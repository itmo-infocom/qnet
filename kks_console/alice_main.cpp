//Точка входа консольного приложения Алисы
using namespace std;//Чтобы не указывать это явно перед каждой функцией.

#include "common_header.h"	//Файл с функциями и константами,
				//одинаковыми и для Алисы, и для Боба
//---------------------------------------
//Глобальные переменные и константы
	struct configuration
	{
		struct sockaddr_in addr;
		socklen_t addr_len;
	}
	config, //Рабочая конфигурация, с которой работаем
	config_default; //Эта конфигурация "по умолчанию" - задействуем её только
			//когда у нас отсутствует файл конфигурации

//---------------------------------------
//Прототипы функций
	#include "common.cpp"

//---------------------------------------
//Точка входа
int main( void )
{
	//В этом блоке можно определять различные константы
	{
		config_default.addr.sin_family = AF_INET;
		config_default.addr.sin_addr.s_addr = INADDR_ANY;
		config_default.addr.sin_port = htons( 50000 );
		config_default.addr_len = sizeof( config.addr );
	}	
		
	while ( true )	//Самый внешний цикл - заставляет повториться всю программу,
			//если что-то пошло не так
	{
		read_config();
		
		//int socket = socket_init( config.addr );
		int sock = socket( AF_INET, SOCK_STREAM, 0 );
		//Дескриптор сокета - далее нам надо будет его прослушивать,
		//чтобы установить связь с Бобом и/или GUI

		if ( sock < 0 )
		{
			//В случае неудачи при создании сокета - выбить ошибку и
			//полностью прекратить работу программы, т.к. это некая
			//пермаментная проблема
			cerr << "alice_main: Cannot create socket" << endl;
			return EXIT_FAILURE;
		}

		if ( bind(
			sock,
			(struct sockaddr *) &config.addr,
			config.addr_len ) < 0 )
		{
			cerr << "alice_main: Cannot bind socket" << endl;
			return EXIT_FAILURE;
		}
		
		//Теперь будем ждать пока к нам кто-нибудь подключится.
		//Подключиться могут только двое - Боб или GUI:
		if ( listen( sock, 2 ) < 0 )
		{
			//Если даже слушать не можем - то тоже падаем в обморок,
			//т.к. не знаю допустимо ли такое вообще и при каких условиях
			//эта ошибка возникает.
			cerr << "alice_main: Cannot listen socket" << endl;
		}
		
		int Bob = -1, GUI = -1; 
		//Дескрипторы клиентов
		//По умолчанию GUI к нам не подключён. А вот без Боба нам никак
		
		while ( Bob == -1 )//Установка соединения с Бобом
			//Будет крутить этот кусок до тех пор, пока не установим
			//связь с Бобом
		{
			
			int temp_client = accept(
				sock,
				(struct sockaddr *) &config.addr,
				&config.addr_len );
			//К нам кто-то подключился - теперь определим кто это и дадим
			//соответствующее имя.
			//Для этого установим связь connect() и спросим кто это
			if ( temp_client < 0 )
			{
				cerr << "alice_main: Cannot connect to Bob" << endl;
				return EXIT_FAILURE;
			}
			
			char who_are_you;
			int recv_bytes = recv( 
				temp_client,
				&who_are_you,
				sizeof( who_are_you ), 0 );
				//Подождём пока собеседник представится

			if ( recv_bytes < 0 )
			{
				cerr << "alice_main: Couldn't recieve packet" << endl;
				return EXIT_FAILURE;
			}

			if ( who_are_you == BOB ) Bob = temp_client;
			else GUI = temp_client;
		}//end Установка соединения с Бобом
		
		//В этой точке у нас точно налажена связь с Бобом

		//Теперь создадим цикл, в котором будем крутить основной алгоритм
		//работы
		bool work_flag = true;
		while ( work_flag )//рабочий цикл
		{
			cout << "alice_main: Successfully connected with Bob" << endl;
			break;
		}//end рабочий цикл
		
		//Не забываем закрыть все соединения
		if ( close( Bob ) < 0 )
		{
			cerr << "alice_main: Cannot close connection with Bob" << endl;
			return EXIT_FAILURE;
		};
		if ( GUI != -1 ) close( GUI );
		//Может быть и так, что GUI не подключён

		close( sock );
		break;
	}//end Внешний цикл
	return EXIT_SUCCESS;
}//end main()

//--------------------------------------------------
//Определения функций Алисы
