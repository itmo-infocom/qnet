//Точка входа консольного приложения Алисы
using namespace std;//Чтобы не указывать это явно перед каждой функцией.

#include "common_header.h"
//Файл с функциями и компонентами, одинаковыми и для Алисы, и для Боба

#include <netdb.h>//Для работы gethostbyname() - чтобы можно было пользоваться DNS
#include <strings.h>//Для работы bcopy()
#include <vector>//Для работы с векторами
#include "FFT_CODE/complex.h"
#include "FFT_CODE/fft.cpp"

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
	
	#define PRINT(text) cout << "bob_main: " << text << endl;

vector<bool> key_sift(detections &alice, detections &bob); //Просеивание ключа
float calib_max(int* arr); //Выдает положение максимума в радианах

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
				PRINT("Connecting to Alice...")
				Alice = connect (
						sock,
						(struct sockaddr *) &config.addr,
						config.addr_len );
					
				if (Alice < 0) 
				{
					cerr << "bob_main: Cannot connect to Alice" << endl;
					sleep(1);
					continue;
				}
			
				PRINT("Successfully connected to Alice")
			
				//В этой точке у нас точно налажена связь с Алисой.
				//Теперь нам надо перед ней представиться, чтобы она не подумала, что мы являемся GUI
				char Im_Bob = device::bob;
				int n = send( sock, &Im_Bob, sizeof( Im_Bob ), 0 );
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
					
					return EXIT_SUCCESS;
					break;
					
				}//end рабочий цикл
		
				//Не забываем закрыть все соединения
				if (close( Alice ) < 0) 
				{
					cerr << "bob_main: Cannot close connection with alice" << endl;
					return EXIT_FAILURE;
				}
				if ( GUI != -1 ) 
					if (close( GUI ) < 0)//Может быть и так, что GUI не подключён
					{
						cerr << "bob_main: Cannot close connection with GUI" << endl;
						return EXIT_FAILURE;
					}
				
			}//end цикл socket connect
			
			close( sock );
			
		}//end Инициализация сокета
		
	}//end Внешний цикл
	
}//end main()

//Функция просеивания ключа. Возвращает <vector> с уже просеянным ключем.
vector<bool> key_sift(detections &alice, detections &bob) //Просеивание ключа
{
   vector<bool> temp; //Сюда собирается ключ
   for (unsigned int i=0; i<alice.basis.size(); i++) 
      {
         if (alice.basis[i]==bob.basis[i]) //Если базисы совпадают, то добавляем бит из ключа 
            { 
               temp.push_back(bob.key[i]);
            }
      }
return temp;
}

//Нахождение калибровочного максимума в радианах. На вход целочисленный массив
float calib_max(int* arr)
{
   complex cplx_arr[128];
   for (int i=0; i<128; i++)   //Конвертирование целосчисленного входного массива в массив типа complex (ТАК НАДО)
      {
         cplx_arr[i]=arr[i];
      }
   CFFT::Forward(cplx_arr, 128); //Быстрое преобразование Фурье. Массив типа complex на вход, 128 элементов в массиве
   return M_PI_2+atan(cplx_arr[1].im()/cplx_arr[1].re());
}

//--------------------------------------------------
//Определения функций Боба
