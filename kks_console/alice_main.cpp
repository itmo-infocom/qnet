//Точка входа консольного приложения Алисы
#include <iostream>
#include <errno.h>//Для использования errno
#include <string.h>//Ради strerror
#include <sstream>
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

	brd.setBufSize(2*collect_time);
	brd.SetDMA(true);
	brd.clear_buf();
	
	//while (bob.regime() == regimes::gen_key)
	{
		//Принимаем detections от боба
		detections bob_detect;
		//bob.recv(bob_detect);

		//Вытащим нужные отсчеты из своего DMA
		detections my_detect = brd.get_detect(bob_detect.count);

		//Отправим detections Бобу, предварительно удалив ключи
		{
			detections to_send = my_detect;
			to_send.key.clear(); 
			//bob.send(to_send);
		} 

		vector<bool> key = sift_key(my_detect, bob_detect);
		
		unsigned int seed_qber;
		vector<bool> qber_key = get_qber_key(key, seed_qber);
		
		//bob.send(seed_qber);
		//bob.send(qber_key);

		//Принимаем от боба код ошибки.
		errors errcode; 
		//bob.recv(errcode)
		if (errcode == errors::qber) throw errcode;

		//Примем код Хэмминга
		vector<bool> ham_code;
		//bob.recv(ham_code);
		//key = HamDecode(key, ham_code);

		//Сформируем команду для отправки curl-запроса потребителю.
		//Ключ передаётся в шестнадцатеричном виде в видe ascii-текста
		//Если чисто бит не кратно четырём, то ненужные биты с конца отбрасываем 
		{
			string cmd = "curl --data ";
			stringstream ss;
			//Отбросим некратные четырём биты с конца
			key.resize((key.size()/4)*4);
			//Теперь будем формировать по одному hex-символу и добавлять ко строке
			for (size_t i = 0; i < key.size()/4; i++)
			{
				uint8_t val = 0;
				for (size_t j = 0; j < 4; j++) val |= key[i*4 + j] << j;
				ss << hex << val;
			}
			cmd += ss.str();
			cmd += ' ' + URL;
			system(cmd.c_str());
		} 
	}

	brd.SetDMA(false);
}