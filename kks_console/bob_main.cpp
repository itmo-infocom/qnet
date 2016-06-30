//Точка входа консольного приложения Алисы
using namespace std;//Чтобы не указывать это явно перед каждой функцией.

#include "common_header.h"
//Файл с функциями и компонентами, одинаковыми и для Алисы, и для Боба

#include <netdb.h>//Для работы gethostbyname() - чтобы можно было пользоваться DNS
#include <strings.h>//Для работы bcopy()

//---------------------------------------
//Глобальные переменные и константы
	//Структура configuration хранит в себе всю конфигурацию, которой подчинается программа
	//Она синхронизирована между Алисой и Бобом
	struct configuration
	{
		struct sockaddr_in addr;
		socklen_t addr_len;
	}
	config, config_default;//Глобальная переменная
	
//---------------------------------------
//Прототипы функций
	#include "common.cpp"
	
	#define PRINT(text) cout << "bob_main: text" << endl;

//---------------------------------------
//Точка входа
int main( void )
{
	//Конфигурация по умолчанию для сокета
			{
				config_default.addr.sin_family = AF_INET;
					char default_hostname[] = "localhost4";
					struct hostent *server = gethostbyname( default_hostname );
					if (server == NULL )
					{
						cerr 	<< "bob_main: No such hostname "
							<< default_hostname << endl;
						return EXIT_FAILURE;
					}

				bcopy(
					(char *) server->h_addr,
					(char *) &config_default.addr.sin_addr.s_addr,
					server->h_length );

				config_default.addr.sin_port = htons( 50000 );
				config_default.addr_len = sizeof( config_default.addr );
			}

	//Чтение конфигурации из файла
	read_config();
	
	while (true)//start внешний цикл
	{
		//Самый внешний цикл - заставляет повториться всю программу,
		//если что-то пошло не так
		
		//Инициализация сокета
		{		
			//Дескриптор сокета - далее нам надо будет его прослушивать,
			//чтобы установить связь с Бобом и/или GUI
			int sock = socket( AF_INET, SOCK_STREAM, 0 );

			if ( sock < 0 )
			{
				//В случае неудачи при создании сокета - выбить ошибку и
				//полностью прекратить работу программы, т.к. это некая
				//пермаментная проблема
				cerr << "bob_main: Cannot create socket" << endl;
				return EXIT_FAILURE;
			}
			while (true)//start цикл socket connect
			{
				int Alice, GUI = -1; 
				//Дескрипторы соединений
				//По умолчанию GUI к нам не подключён. А вот без Алисы нам никак
				PRINT(Connecting to Alice...)
				Alice = connect (
						sock,
						(struct sockaddr *) &config.addr,
						config.addr_len );
					
				if (Alice < 0) 
				{
					cerr << "bob_main: Cannot connect to Alice" << endl;
					continue;
				}
			
				PRINT(Successfully connected to Alice)
			
				//В этой точке у нас точно налажена связь с Алисой.
				//Теперь нам надо перед ней представиться, чтобы она не подумала, что мы являемся GUI
				char Im_Bob = bob;
				int n = send( Alice, &Im_Bob, sizeof( Im_Bob ), 0 );
				if ( n < 0 )
				{
					cerr << "bob_main: Cannot send a packet to Alice" << endl;
					continue;
				}
		
				//Теперь создадим цикл, в котором будем крутить основной алгоритм работы
				while (true)//рабочий цикл
				{
				
					//Внутри этого цикла необходимо размещать весь интеллект,
					//связанный с работой платы и прочим-прочим
					break;
					
				}//end рабочий цикл
		
				//Не забываем закрыть все соединения
				close( Alice );
				if ( GUI != -1 ) close( GUI );//Может быть и так, что GUI не подключён
				
			}//end цикл socket connect
			
			close( sock );
			
		}//end Инициализация сокета
		
	}//end Внешний цикл
	
}//end main()

//--------------------------------------------------
//Определения функций Боба
