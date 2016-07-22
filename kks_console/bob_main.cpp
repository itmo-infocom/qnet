//Точка входа консольного приложения Алисы
#include "header.h"

//Файл с функциями и компонентами, одинаковыми и для Алисы, и для Боба

#include "FFT_CODE/complex.h"
#include "FFT_CODE/fft.cpp"
//---------------------------------------
//Глобальные переменные и константы

//---------------------------------------
//Прототипы функций
#include "common.cpp"

	vector<bool> key_sift(detections &alice, detections &bob); //Просеивание ключа
	float calib_max(int* arr); //Выдает положение максимума в радианах

//---------------------------------------
//Точка входа
int main( void )
{
	while ( true )
	//Самый внешний цикл - заставляет повториться всю программу,
	//если что-то пошло не так
	{
		try
		{
      char port[] = "50000";
			NetWork server( 3, port );
			
			while (!server.IsReady(peers::alice)) sleep(1);
			
			//Теперь создадим цикл, в котором будем крутить основной алгоритм работы
			bool work_flag = true;
			while ( work_flag )//рабочий цикл
			{
				std::cout << "bob: Successfully connected with Alice" << std::endl;
				break;
			}//end рабочий цикл
			
			//Не забываем закрыть все соединения
			break;
		} catch (int errno)
		{
			std::cerr << strerror(errno) << std::endl;
		}

	}//end Внешний цикл
	
}//end main()

//--------------------------------------------------
//Определения функций Боба

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

