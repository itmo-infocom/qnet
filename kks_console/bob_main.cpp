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

		//Отправка отдельного числа int
		{
			cout << "Посылаю число 7..." << endl;
			alice.Send(7);
		}

		//Отправка vector<unsigned int>
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

		//Отправка vector<bool>
		{
			cout << "Отправка vector<bool>(";
			vector<bool> v(16);
			cout << v.size() << "): ";
			for (int i = 0; i < v.size(); i++) 
			{
				v[i] = i & 0b1;
				cout << (i & 0b1);
			}
			alice.Send(v);
			cout << endl;
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