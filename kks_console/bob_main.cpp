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

#include "FFT_CODE/complex.h"
#include "FFT_CODE/fft.cpp"

#include "common_func.cpp"

const std::string URL = "http://localhost4/qchannel/1/";
//---------------------------------------
	//Алгоритм генерации ключа
	regimes generation_key(board_if::board_if &brd, NetWork::client &alice, NetWork::server &GUI);
	
	//Тестовая функция, сохраняющая пырк в текстовом файле
	void test(board_if::board_if &brd, NetWork::client &alice);
	
	//Функция, подстраивающая амплитуды и фазы модуляторов Боба. Возвращает значение статического контраста
	double static_contrast(board_if::board_if &brd, NetWork::client &alice);

//---------------------------------------
//Точка входа
int main( int argc, char** argv )
{
	using namespace std;
	try{
		board_if::board_if brd;
		char hostname[] = "localhost4";
		char port[] = "50000";
		NetWork::client alice(hostname, port);
		NetWork::server GUI(port, SOCK_NONBLOCK);

		regimes current_regime = regimes::gen_key;
		
		while(true)
		{
			if (GUI.accept_cli()) GUI.Recv(current_regime);
			
			switch(current_regime)
			{
				case test_pyrk: test(brd, alice); break;
				default: current_regime = generation_key(brd, alice, GUI);
			}
		}
		//test(brd, alice);
	}
	catch(NetWork::client::except &obj)
	{
		cerr << obj.errstr << ' ' << errno << ' ' << endl;
		return EXIT_FAILURE;
	}	
	catch(board_if::board_if::except &obj)
	{
		cerr << obj.errstr << endl;
		return EXIT_FAILURE;
	}
}//end main()

//--------------------------------------------------
//Определения функций Боба

regimes generation_key(board_if::board_if &brd, NetWork::client &alice, NetWork::server &GUI)
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
	brd.clear_buf();//Удаление всех фреймов, пока не встретим с номером ноль

	regimes curr_regime = regimes::gen_key;	
	
	while (curr_regime == regimes::gen_key)
	{
		detections my_detect = brd.detects(collect_time);//Будем собирать детектирования в течение одной секунды
		alice.Send(my_detect);
		detections alice_detect;
		alice.Recv(alice_detect);

		vector<bool> key = sift_key(my_detect, alice_detect);

		//Вычисление qber
		{
			unsigned int seed_qber;
			alice.Recv(seed_qber);
			vector<bool> qber_key = get_qber_key(key, seed_qber);
			vector<bool> alice_qber_key;
			alice.Recv(alice_qber_key);
			size_t mismatch = 0;//Число несовпадающих бит
			for (size_t i = 0; i < qber_key.size(); i++)
			if (qber_key[i] != alice_qber_key[i]) mismatch++;
			
			if ((double)mismatch/qber_key.size() > max_qber)
			{
				errors errcode = errors::qber;
				alice.Send(errcode);
				throw errcode;
			}
		}

		//Формируем код Хэмминга и отправляем его Алисе
		alice.Send(HammingCode(key));

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
	return curr_regime;
}

void test(board_if::board_if &brd, NetWork::client &alice)
{
	//Соберём пырк
	{
		//TableRNG с пыпыркой
		{
			vector<int> rng(1024,0);
			rng.back() = 2;
			brd.TableRNG(rng);
		}

		brd.SetBufSize(2*collect_time);
		alice.Send(0);
		brd.SetDMA(true);
		brd.clear_buf();//Удаление всех фреймов, пока не встретим с номером ноль

		detections my_detect = brd.detects(collect_time);

		brd.SetDMA(false);
		//Удалим сшивочный элемент
		my_detect.count.pop_back();

		//Теперь создадим массив одного с TableRNG размера и запишем в ней набранную статистику
		{
			vector<unsigned int> stat(1024,0);
			unsigned int sum = 0;
			for (auto i : my_detect.count)
			{
				sum += i;
				stat[sum%1024]++;
			}

			//Выведем статистику во внешний файл
			{
				ofstream f("pyrk", ios::trunc);
				for (auto i : stat) f << i << endl;
			}
		}
	}
}

double static_contrast(board_if::board_if &brd, NetWork::client &alice)
{
	
}