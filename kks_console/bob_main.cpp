//Точка входа консольного приложения Алисы
using namespace std;//Чтобы не указывать это явно перед каждой функцией.

#include "common_header.h"
//Файл с функциями и компонентами, одинаковыми и для Алисы, и для Боба

#include <netdb.h>//Для работы gethostbyname() - чтобы можно было пользоваться DNS
#include <strings.h>//Для работы bcopy()

//---------------------------------------
//Глобальные переменные и константы
	
	
//---------------------------------------
//Прототипы функций
	#include "common.cpp"
	
	#define PRINT(text) cout << "bob_main: " << text << endl;

//---------------------------------------
//Точка входа
int main( int argc, char **argv )
{
	while (true)//start Инициализация сокета
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
    
    //Конфигурация по умолчанию для сокета
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    socklen_t addr_len;
    struct hostent *server = gethostbyname( argv[1] );
    if (server == NULL )
    {
      cerr 	<< "bob: No such hostname " <<  argv[1]<< endl;
      return EXIT_FAILURE;
    }
    bcopy(
      (char *) server->h_addr,
      (char *) &addr.sin_addr.s_addr,
      server->h_length );
    addr.sin_port = htons( 50000 );
    addr_len = sizeof( addr );
      		
		while (true)//start цикл socket connect
		{
			int Alice_sock, GUI_sock = -1; 
			//Дескрипторы соединений
			//По умолчанию GUI к нам не подключён. А вот без Алисы нам никак
			cout << "bob: Connecting to Alice..." << endl;
			Alice_sock = connect ( sock, (struct sockaddr *) &addr,	addr_len );
				
			if (Alice_sock < 0) 
			{
				cerr << "bob: Cannot connect to Alice" << endl;
				sleep(1);
				continue;
			}
		
			cout << "bob: Successfully connected to Alice" << endl;
		
			//В этой точке у нас точно налажена связь с Алисой.
			//Теперь нам надо перед ней представиться
      int im_bob = device::bob;
      send( sock, &im_bob, sizeof( im_bob ), 0 );
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
				//Внутри этого цикла будет размещён весь интеллект,
				//связанный с работой платы и прочим-прочим
				
				return EXIT_SUCCESS;
				break;
			}//end рабочий цикл

			close( Alice_sock );
			if ( GUI != - 1 ) close( GUI );

		}//end цикл socket connect
	
	  close( sock );
			
  }//end Инициализация сокета
		
}//end main()

//--------------------------------------------------
//Определения функций Боба
