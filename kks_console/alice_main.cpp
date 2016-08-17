//Точка входа консольного приложения Алисы
#include <iostream>
#include <errno.h>//Для использования errno
#include <string.h>//Ради strerror
#include <sstream>
#include <string>
#include <vector>

#include "board_if.h"
#include "NetWork.h"
#include "detections.cpp"

#include "common_func.cpp"

const std::string URL = "http://localhost4/qchannel/1/";
//---------------------------------------
	//Алгоритм генерации ключа
	regimes generation_key(board_if::board_if &brd, NetWork::server &bob);//TODO: добавить ссылку на сокет
	void test(board_if::board_if &brd, NetWork::server &bob);

//---------------------------------------
//Точка входа
int main( int argc, char** argv )
{
	using namespace std;
	try{
		char port[] = "50000";
		NetWork::server bob(port);
		bob.accept_cli();
		board_if::board_if brd;

		test(brd, bob);
	}
	catch(NetWork::server::except &obj)
	{
		cerr << obj.errstr << ' ' << errno << ' ' << endl;
		return EXIT_FAILURE;
	}
	catch(board_if::board_if::except &obj)
	{
		cerr << obj.errstr << endl;
		return EXIT_FAILURE;
	}
}

void generation(board_if::board_if &brd, NetWork::server &bob)
{
	using namespace std;
	
	//TODO: В документации упомянуть, что нельзя работать с регистрами одновременно с работой DMA
	brd.SetDMA(false);

	//Запись full-rnd в TableRNG
	{
		srand(time(0));
		vector<int> tmp(AnBTableMaxSize);
		for (size_t i = 0; i < tmp.size(); i++)
			tmp[i] = random();
		brd.TableRNG(tmp);
	}

	brd.SetBufSize(2*collect_time);
	brd.SetDMA(true);
	brd.clear_buf();
	
	regimes curr_regime;
	{
		int tmp;
		bob.Recv(tmp);
		curr_regime = (regimes)tmp;
	}
	while (curr_regime == regimes::gen_key)
	{
		//Принимаем detections от боба
		detections bob_detect;
		bob.Recv(bob_detect);

		//Вытащим нужные отсчеты из своего DMA
		detections my_detect = brd.get_detect(bob_detect.count);

		//Отправим detections Бобу, предварительно удалив ключи
		{
			detections to_send = my_detect;
			to_send.key.clear(); 
			bob.Send(to_send);
		} 

		vector<bool> key = sift_key(my_detect, bob_detect);
		
		unsigned int seed_qber;
		vector<bool> qber_key = get_qber_key(key, seed_qber);
		
		bob.Send(seed_qber);
		bob.Send(qber_key);

		//Принимаем от боба код ошибки.
		errors errcode;
		{
			int tmp;
			bob.Recv(tmp);
			errcode = (errors)tmp;
		}
		if (errcode == errors::qber) throw errcode;

		//Примем код Хэмминга
		vector<bool> ham_code;
		bob.Recv(ham_code);
		key = HammingDecode(key, ham_code);

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

void test(board_if::board_if &brd, NetWork::server &bob)
{
	//TableRNG в статику
	{
		vector<int> rng(1024,0);
		brd.TableRNG(rng);
	}

	brd.SetBufSize(2*collect_time);
	int start;
	bob.Recv(start);
	brd.SetDMA(true);

	sleep(1);

	brd.SetDMA(false);
}