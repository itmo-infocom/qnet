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
	}	config;
	
//---------------------------------------
//Прототипы функций
	#include "common.cpp"
	
	#define PRINT(text) cout << "bob_main: " << text << endl;

  //Сохраняет текущую конфигурацию в энергонезависимую память
  void save_conf( configuration &config)
  {
    ofstream file("comfiguration.bin", ios::binary);
    file.write( (char*)&config, sizeof(config) );
    file.close();
  }
//---------------------------------------
//Точка входа
int main( void )
{
	//Конфигурация по умолчанию для сокета
  {
    //Попытаемся прочитать конфигурацию из файла
    ifstream file( "configuration.bin", ios::binary );
    if (file)
    {
      file.read( (char*)&config, sizeof(config) );
      file.close();
    } else
		{
      cout << "bob: Using default settings" << endl;
			config.addr.sin_family = AF_INET;
			char default_hostname[] = "localhost4";
			struct hostent *server = gethostbyname( default_hostname );
			if (server == NULL )
			{
				cerr 	<< "bob: No such hostname " << default_hostname << endl;
				return EXIT_FAILURE;
			}
				bcopy(
				(char *) server->h_addr,
				(char *) &config.addr.sin_addr.s_addr,
				server->h_length );
				config.addr.sin_port = htons( 50000 );
  			config.addr_len = sizeof( config.addr );
		}
	}

	while (true)//start внешний цикл
	{
		//start Инициализация сокета
		{		
			//Дескриптор сокета - далее нам надо будет его прослушивать,
			//чтобы установить связь с Бобом и/или GUI
			int sock = socket( AF_INET, SOCK_STREAM, 0 );

			if ( sock < 0 )
			{
				//В случае неудачи при создании сокета - выбить ошибку и
				//полностью прекратить работу программы, т.к. это некая
				//пермаментная проблема
				cerr << "bob: Cannot create socket" << endl;
				return EXIT_FAILURE;
			}
			
			while (true)//start цикл socket connect
			{
				int Alice_sock, GUI_sock = -1; 
				//Дескрипторы соединений
				//По умолчанию GUI к нам не подключён. А вот без Алисы нам никак
				cout << "bob: Connecting to Alice..." << endl;
				Alice_sock = connect (
						sock,
						(struct sockaddr *) &config.addr,
						config.addr_len );
					
				if (Alice_sock < 0) 
				{
					cerr << "bob: Cannot connect to Alice" << endl;
					sleep(1);
					continue;
				}
			
				cout << "bob: Successfully connected to Alice" << endl;
			
				//В этой точке у нас точно налажена связь с Алисой.
				//Теперь нам надо перед ней представиться
	      send( sock, &device::bob, sizeof( device::bob ), 0 );
        //Также удостоверимся, что мы подключились именно к Алисе
        {
          int server;
          recv( sock, &server, sizeof( device::alice ), 0 );
          if ( server != device::alice )
          {
            cerr << "bob: Connected not to Alice" << endl;
            close( sock );
            break;
          }
        }
		
				//Теперь создадим цикл, в котором будем крутить основной алгоритм работы
				while (true)//рабочий цикл
				{
				
					//Внутри этого цикла необходимо размещать весь интеллект,
					//связанный с работой платы и прочим-прочим
					
					return EXIT_SUCCESS;
					break;
				}//end рабочий цикл
		
				close( Alice ) 
				if ( GUI != - 1 ) close( GUI );
			}//end цикл socket connect
			
			close( sock );
			
		}//end Инициализация сокета
		
	}//end Внешний цикл
	
}//end main()

//--------------------------------------------------
//Определения функций Боба
