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

//---------------------------------------
//Точка входа
int main( int argc, char** argv )
{
	using namespace std;
	try{
		char hostname[] = "localhost4";
		char port[] = "50000";
		NetWork::NetWork bob(hostname, port);
		//bob.Send(NetWork::peers::alice);
	}
	catch(NetWork::NetWork::except &obj)
	{
		cerr << obj.errstr << endl;
		return EXIT_FAILURE;
	}
}