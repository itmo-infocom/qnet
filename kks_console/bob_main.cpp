//Точка входа консольного приложения Алисы
using namespace std;//Чтобы не указывать это явно перед каждой функцией.

#include "common_header.h"
//Файл с функциями и компонентами, одинаковыми и для Алисы, и для Боба

#include <netdb.h>//Для работы gethostbyname() - чтобы можно было пользоваться DNS
#include <strings.h>//Для работы bcopy()

//---------------------------------------
//Глобальные переменные и константы
	struct configuration
	{
		struct sockaddr_in addr;
		socklen_t addr_len;
	}
	config, //Рабочая конфигурация, с которой работаем почти всегда
	config_default;	//Эта конфигурация "по умолчанию" - задействуем её только
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
		
	while ( true )
	//Самый внешний цикл - заставляет повториться всю программу,
	//если что-то пошло не так
	{
		read_config();
		
		int sock = socket( AF_INET, SOCK_STREAM, 0 );
		//Дескриптор сокета - далее нам надо будет его прослушивать,
		//чтобы установить связь с Бобом и/или GUI

		if ( sock < 0 )
		{
			//В случае неудачи при создании сокета - выбить ошибку и
			//полностью прекратить работу программы, т.к. это некая
			//пермаментная проблема
			cerr << "bob_main: Cannot create socket" << endl;
			return EXIT_FAILURE;
		}
		
		int Alice, GUI = -1; 
		//Дескрипторы соединений
		//По умолчанию GUI к нам не подключён. А вот без Алисы нам никак
		
		Alice = connect (
				sock,
				(struct sockaddr *) &config.addr,
				config.addr_len );
		if ( Alice < 0 )
		{
			cerr << "bob_main: Cannot connect to Alice" << endl;
			return EXIT_FAILURE;
		}
		
		//В этой точке у нас точно налажена связь с Алисой.
		//Теперь нам надо перед ней представиться
		char Im_Bob = bob;
		int n = send( sock, &Im_Bob, sizeof( Im_Bob ), 0 );
		if ( n < 0 )
		{
			cerr << "bob_main: Cannot send a packet to Alice" << endl;
			return EXIT_FAILURE;
		}
		
		//Теперь создадим цикл, в котором будем крутить основной алгоритм работы
		bool work_flag = true;
		while ( work_flag )//рабочий цикл
		{
			cout	<< "bob_main: Successfully connected with Alice"
				<< endl;
			break;
		}//end рабочий цикл
		
		//Не забываем закрыть все соединения
		close( Alice );
		if ( GUI != -1 ) close( GUI );//Может быть и так, что GUI не подключён
		close( sock );
		break;
	}//end Внешний цикл
}//end main()

//--------------------------------------------------
//Определения функций Боба
