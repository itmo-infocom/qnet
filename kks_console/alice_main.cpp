//Точка входа консольного приложения Алисы
#include <iostream>
#include <errno.h>//Для использования errno
#include <string.h>//Ради strerror
#include <string>
#include <vector>

//#include "board_if.h"
#include "NetWork.h"
#include "detections.cpp"

//---------------------------------------
//Точка входа
int main( int argc, char** argv )
{
	using namespace std;
	try{
		char port[] = "50000";
		NetWork::server bob(port);
		bob.accept_cli();
		
		//Приём отдельного числа int
		{
			cout << "Принятое число: ";
			int number;
			bob.Recv(number);
			cout << number << endl;
		}

		//Приём vector<unsigned int>
		{
			cout << "Принятый вектор целых чисел (";
			vector<unsigned int> v;
			bob.Recv(v);
			cout << v.size() << "): ";
			for (auto i : v) cout << i;
			cout << endl;
		}		

		//Приём vector<bool>
		{
			cout << "Принятый vector<bool>(";
			vector<bool> v;
			bob.Recv(v);
			cout << v.size() << "): ";
			for (auto i : v) cout << i;
			cout << endl;
		}
	}
	catch(NetWork::server::except &obj)
	{
		cerr << obj.errstr << ' ' << errno << ' ' << endl;
		return EXIT_FAILURE;
	}
}