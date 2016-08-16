//Точка входа консольного приложения Боба
#include <iostream>
#include <errno.h>//Для использования errno
#include <string.h>//Ради strerror
#include <string>
#include <vector>

//#include "board_if.h"
#include "NetWork.h"
#include "detections.cpp"

#include "common_func.cpp"

//---------------------------------------
//Точка входа
int main( void )
{
	using namespace std;
	try{
		char hostname[] = "localhost4";
		char port[] = "50000";
		NetWork::client alice(hostname, port);

		//Отправка отдельного числа
		{
			cout << "Посылаю число 7..." << endl;
			alice.Send(7);
		}

		//Отправка вектора целых чисел
		{
			cout << "Отправляю вектор целых чисел(";
			vector<unsigned int> v(6);
			cout << (int)v.size() << "): ";
			for (int i = 0; i < v.size(); i++) 
			{
				v[i] = i;
				cout << i;
			}
			cout << endl; 
			alice.Send(v);
		}
	}
	catch(NetWork::client::except &obj)
	{
		cerr << obj.errstr << ' ' << errno << ' ' << endl;
		return EXIT_FAILURE;
	}	
}//end main()

//--------------------------------------------------
//Определения функций Боба