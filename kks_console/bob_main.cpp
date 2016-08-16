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
	}
	catch(NetWork::client::except &obj)
	{
		cerr << obj.errstr << endl;
		return EXIT_FAILURE;
	}	
}//end main()

//--------------------------------------------------
//Определения функций Боба