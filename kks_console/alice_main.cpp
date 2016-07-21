//Точка входа консольного приложения Алисы
#include <iostream>
#include <errno.h>//Для использования errno
#include <string.h>//Ради strerror
#include <string>
#include <vector>

#include "board_if.h"
//#include "NetWork.h"
#include "detections.cpp"

#include "common_func.cpp"

const std::string URL = "http://localhost4/qchannel/1/";
//---------------------------------------
	//Алгоритм генерации ключа
	void generation_key(board_if::board_if &brd);//TODO: добавить ссылку на сокет

//---------------------------------------
//Точка входа
int main( int argc, char** argv )
{
	using namespace std;
	try{
		board_if::board_if brd;
		generation_key(brd);
	}
	catch(board_if::board_if::except &obj)
	{
		cerr << obj.errstr << endl;
		return EXIT_FAILURE;
	}
}

void generation(board_if::board_if &brd)
{
	using namespace std;
	
	brd.SetDMA(false);//TODO: В документации упомянуть, что нельзя работать с регистрами одновременно с работой DMA

	//Запись full-rnd в TableRNG
	{
		srand(time(0));
		vector<int> tmp(AnBTableMaxSize);
		for (size_t i = 0; i < tmp.size(); i++)
			tmp[i] = random();
		brd.TableRNG(tmp);
	}

	brd.SetDMA(true);
	brd.clear_buf();
	
	//while (bob.regime() == regimes::gen_key)
	{
		//Принимаем detections от боба
		detections bob_detect;
		//bob.recv(bob_detect);

		//Вытащим нужные отсчеты из своего DMA
		detections my_detect = brd.get_detect(bob_detect.count);
	}

	brd.SetDMA(false);
}