//Коды ошибок
	#define CANNOT_CREATE_SOCKET
	#define CANNOT_CONNECT
	#define CANNOT_SEND

//-------------------------------------------------------------------
//Общие константы
	enum Err
	{
		sock_create,
		sock_send_recv,
		sock_bind
	};
	
	enum peers
	{
		alice,
		bob,
		gui,
		telnet,
		
		peers_size
	};
//-------------------------------------------------------------------
//Общие типы данных и структуры
	#include "detections.cpp"
	#include "DMAFrame.cpp"
	#include "sockets.cpp"

//-------------------------------------------------------------------
//Общие функции
	//Чтение конфигурации из бинарного файла
		void read_config()
		{
			ifstream file( "configuration.bin", ios::binary );
			if (file) 
			{
				//read( file, (void *) &config, sizeof( config ));
				file.read( (char *) &config, sizeof( config ));
				file.close();
			}
			{
				file.close() ;
				//Раз в нём ничего нет - то он нам больше не нужен
				//Если файла не существует - то установим значения
				//по умолчанию

				cout << "Using default settings" << endl;

				config = config_default;
				//Если конфиг. файл отсутствует, то его записывать
				//не будем - для отладки будет лучше, если он будет
				//инициализироваться по данным из исходного кода
			}
		}

	//Сохраняет текущую конфигурацию config в энергонезависимую память
		void save_config()
		{
			ofstream file( "configuration.bin", ios::binary );
			file.write( (char *) &config, sizeof( config ));
			file.close();
		}

	//Процедура инициализации сокета
	//Возвращает дескриптор сокета в случае успеха. Иначе "-1"
		int socket_init( struct sockaddr_in addr )
		{
			//Алиса является сервером-сокета - т.о. она прослушивает порт
			int sock = socket( AF_INET, SOCK_STREAM, 0);
			
			socklen_t addr_len = sizeof( addr );
			if ( bind( 
				sock,
				(struct sockaddr *) &addr,
				addr_len ) < 0)
					return -1;
			
			return sock;
		}
