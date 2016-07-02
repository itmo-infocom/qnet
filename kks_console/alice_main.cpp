//Точка входа консольного приложения Алисы
using namespace std;//Чтобы не указывать это явно перед каждой функцией.

#include "common_header.h"	//Файл с функциями и константами,
				//одинаковыми и для Алисы, и для Боба
//---------------------------------------
//Глобальные переменные и константы



//---------------------------------------
//Прототипы функций
	#include "common.cpp"

	#define PRINT(text) cout << "alice_main: " << text << endl;

//---------------------------------------
//Точка входа
int main( void )
{
	struct sockaddr_in addr;
	socklen_t addr_len;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons( 50000 );//Номер порта по умолчанию
	addr_len = sizeof( config.addr );
		
	while ( true )//start внешний цикл
	{
		int sock = socket( AF_INET, SOCK_STREAM, 0 );
		//Дескриптор сокета - далее нам надо будет его прослушивать,
		//чтобы установить связь с клиентом

		if ( sock < 0 )
		{
			//В случае неудачи при создании сокета - выбить ошибку и
			//полностью прекратить работу программы, т.к. это некая
			//пермаментная проблема
			cerr << "alice: Cannot create socket" << endl;
			return EXIT_FAILURE;
		}

		if ( bind(
			sock,
			(struct sockaddr *) &addr,
			addr_len ) < 0 )
		{
			cerr << "alice: Cannot bind socket - perhaps, port is busy" << endl;
			return EXIT_FAILURE;
		}
		
		//Теперь будем ждать пока к нам кто-нибудь подключится.
		//Подключиться могут только двое - Боб или GUI:
		if ( listen( sock, 1 ) < 0 )//Здесь выполнение блочится пока кто-нибудь к нам не постучится
		{
			//Если даже слушать не можем - то тоже падаем в обморок,
			//т.к. не знаю допустимо ли такое вообще и при каких условиях
			//эта ошибка возникает.
			cerr << "alice: Cannot listen socket" << endl;
			return EXIT_FAILURE;
		}
		
		int Bob_sock = -1;//Файловый дескриптор Боба 
		
		while ( Bob_sock == -1 )//start Установка соединения с Бобом
		{
      //Надо убедиться что это именно Боб
      {
  			int temp_client = accept(
	  			sock,
	  			(struct sockaddr *) &addr,
	  			&addr_len );
	  		//К нам кто-то подключился - теперь определим кто это и дадим
	  		//соответствующее имя.
	  		//Для этого установим связь connect() и спросим кто это
	  		if ( temp_client < 0 )
	  		{
	  			cerr << "alice: Cannot connect to Bob" << endl;
         close( temp_client );
	  			continue;
	  		}
	  		
	  		int who_are_you;
	  		int recv_bytes = recv( temp_client,	&who_are_you,	sizeof( who_are_you ), 0 );
	  		//Подождём пока собеседник представится
  
  			if ( recv_bytes < 0 )
  			{
  				cerr << "alice: Couldn't recieve packet" << endl;
  				continue;
  			}
  
  			if ( who_are_you == device::bob ) 
  			{
  				Bob_sock = temp_client;
          //Скажем Бобу, что я - Алиса
          send( Bob, &device::alice, sizeof(device::alice), 0 );
  				cout << "Established connection with Bob" << endl;
  			} else
        {
          cerr << "alice: This isn't Bob" << endl;
          close( temp_client );
          continue;
        }
      }
		}//end Установка соединения с Бобом
		
		//В этой точке у нас налажена связь с Бобом

		//Теперь создадим цикл, в котором будем крутить основной алгоритм
		//работы
		while (true)//рабочий цикл
		{
			//В теле этого цикла необходимо разместить весь интеллект, касающийся работы самой программы
      
      //Узнаем от Боба в каком режиме мы будем сейчас работать
      int regime;
      int recv_bytes = recv( Bob_sock, &regime, sizeof(regime), 0 );
			
      //Преположим, что соединение с Бобом абсполютно надёжно, поэтому никаких проверок на ошибки делать не будем
      switch (regime)
      {
        case regimes_list::generation:
        {
          //Алгоритм генерации ключа

        }
        default: continue;
      }
		}//end рабочий цикл
		
		//Не забываем закрыть все соединения
  	if ( close( Bob ) < 0 )
    {
      cerr << "alice: Cannot close connection with Bob" << endl;
      break;
    };
		if ( close( sock ) < 0 )
    {
      cerr << "alice: Cannot close socket" << endl;
      break;
    };
	}//end Внешний цикл

	return EXIT_FAILURE;
}//end main()

//--------------------------------------------------
//Определения функций Алисы
