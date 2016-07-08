//Точка входа консольного приложения Алисы
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
void gen_key_alg(board_if::board_if &brd, NetWork::NetWork &bob) throw();

//Приводит сплошной массив Алисы к разреженному как у Боба
detections raw_detect_to_count(detections &alice, detections &bob);
void brd_test(board_if::board_if &brd);

//---------------------------------------
//Точка входа
int main( void )
{
	using namespace std;
	try{
		board_if::board_if brd;
		brd_test(brd);
		char hostname[] = "localhost4";
		char port[] = "50000";
		NetWork::NetWork bob(hostname, port);
		bob.Send(NetWork::peers::alice);
		while (true)
		{
			int regime;
			bob.Recv(regime);
			switch (regime)
			{
				default:
				gen_key_alg(brd, bob);
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
}

void gen_key_alg(board_if::board_if &brd, NetWork::NetWork &bob) throw()
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

	detections alice_data = brd.get_detect(1000);//Время, за которое собираем статистику по срабатываниям детектора
	detections bob_data;
	bob.Recv(bob_data);

	alice_data = raw_detect_to_count(alice_data, bob_data);

	double qber;
	vector<bool> my_key = sift_key(alice_data, bob_data, qber);
	//Подготовим ответ Бобу
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

detections raw_detect_to_count(detections &alice, detections &bob)
{
	detections answer;
	answer.count = bob.count;
	unsigned int pos = 0;
	for (unsigned int count : bob.count)
	{
		pos += count;
		answer.basis.push_back(alice.basis[pos]);
		answer.key.push_back(alice.key[pos]);
	}
	return answer;
}

void brd_test(board_if::board_if &brd)
{
	using namespace std;
	//Для начала попробуем записать TableRNG
	{
		vector<unsigned short int> table_rng(20);
		for (unsigned int i = 0; i < table_rng.size(); i++) table_rng[i] = i; 
		brd.TableRNG(table_rng);
	}

	//Теперь включим DMA и поглядим сколько буфером запишется
	{
		brd.StoreDMA(100);

		int x;
		x++;
	}
}