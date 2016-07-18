//Точка входа консольного приложения Алисы
#include <iostream>
#include <errno.h>//Для использования errno
#include <string.h>//Ради strerror
#include <string>
#include <vector>

#include "board_if.h"
//#include "NetWork.h"
#include "detections.cpp"

//#include "common_func.cpp"

const std::string URL = "http://localhost4/qchannel/1/";
//---------------------------------------
//Алгоритм генерации ключа
//void gen_key_alg(board_if::board_if &brd, NetWork::NetWork &bob) throw();

//Приводит сплошной массив Алисы к разреженному как у Боба
//detections raw_detect_to_count(detections &alice, detections &bob);
void brd_test(board_if::board_if &brd, int argc, char** argv);

//---------------------------------------
//Точка входа
int main( int argc, char** argv )
{
	using namespace std;
	try{
		board_if::board_if brd;
		brd_test(brd, argc, argv);
	}
	catch(board_if::board_if::except &obj)
	{
		cerr << obj.errstr << endl;
		return EXIT_FAILURE;
	}
	
}

void brd_test(board_if::board_if &brd, int argc, char** argv)
{
	using namespace std;
	//Для начала попробуем записать TableRNG
	{
		vector<unsigned short int> table_rng(20);
		for (unsigned int i = 0; i < table_rng.size(); i++) table_rng[i] = 0; 
		//brd.TableRNG(table_rng);
	}

	{
		brd.StoreDMA(1000, argc, argv);

		int x;
		x++;
	}
}