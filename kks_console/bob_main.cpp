//Точка входа консольного приложения Боба
#include <iostream>
#include <errno.h>//Для использования errno
#include <string.h>//Ради strerror
#include <string>
#include <vector>

#include "board_if.h"
#include "NetWork.h"
#include "detections.cpp"

#include "common_func.cpp"

const std::string URL = "http://localhost4/qchannel/1/";
//---------------------------------------
//Алгоритм генерации ключа
void gen_key_alg(board_if::board_if &brd, NetWork::NetWork &server) throw();
//---------------------------------------
//Точка входа
int main( void )
{
	using namespace std;
	try{
		board_if::board_if brd;
		NetWork::NetWork server(1, "50000");
		while (true)
		{
			regimes r = gen_key;
			server.Send((int)r, NetWork::peers::alice);
			switch (r)
			{
				default:
				gen_key_alg(brd, server);
				break;
			}
		}
	}
	catch(board_if::board_if::except &obj)
	{
		cerr << obj.errstr << endl;
		return EXIT_FAILURE;
	}
	catch(NetWork::NetWork::except &obj)
	{
		cerr << obj.errstr << endl;
		return EXIT_FAILURE;
	}	
}//end main()

//--------------------------------------------------
//Определения функций Боба
void gen_key_alg(board_if::board_if &brd, NetWork::NetWork &server)
{
	using namespace std;
	//Запишем таблицу случайных чисел
	{
		unsigned int t_size = AnBTableMaxSize;
		vector<unsigned short int> tablerng(t_size); 
		for (int i = 0; i < t_size; i++)
			tablerng[i] = random();//Пофиг на %4 - всё равно это проверяется
		brd.TableRNG(tablerng);
	}

	detections bob_data = brd.get_detect(1000);//Время, за которое собираем статистику по срабатываниям детектора

	//Отметим некоторые (10%) биты для измерения QBER
	{
		float qber_percentage;
		bob_data.special.resize(bob_data.count.size(), false);
		for (int i = 0; i < bob_data.count.size()*qber_percentage; i++)
		bob_data.special[random()%bob_data.count.size()] = true;
	}

	server.Send(bob_data, NetWork::peers::alice);
	detections alice_data;
	server.Recv(alice_data, NetWork::peers::alice);

	double qber;
	vector<bool> my_key = sift_key(bob_data, alice_data, qber);
	//Примем ответ Алисы
	for (int i = 0; i < alice_data.key.size(); i++)
		if (!bob_data.special[i]) alice_data.key[i] = 0;

	alice_data.special = bob_data.special;

	bob.Send(alice_data);

	if (qber >= max_qber)
	{
		string cmd = "curl ";
		cmd += URL;
		cmd += "2";
		system(cmd.c_str());//Отправили команду, что канал компрометирован
		return;
	}
	
	//Принимаем код Хэмминга
	{
		vector<bool> ham_code;
		bob.Recv(ham_code);
		//my_key = hamming_decode(my_key, ham_code);
	}
	
	//В данной точке my_key - это полностью готовый ключ
	//Переведём его в ASCII и отправим по curl
	{
		string key;
		for (bool b : my_key) key += b?'1':'0';

		string cmd = "curl -d "; cmd += key;
		cmd += URL;
	}
}