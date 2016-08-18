#ifndef COMMON_CPP
#define COMMON_CPP

#include <vector>
#include <stdlib.h>
#include <time.h>

#include "detections.cpp"
#include "HammingCode.cpp"

//Максимальное значений квантового коэффициента ошибок.
const double max_qber = 6.5e-2;
//Время сбора детектирований в миллисекундах
const double collect_time = 1e3;

//Режимы работы
enum regimes
{
	gen_key,
	test_pyrk,

	regimes_size
};

enum errors
{
	ok,
	qber,

	errors_size
};

//Возвращает ключ, отфильтрованный по базису
std::vector<bool> sift_key(const detections &my, const detections &other)
{
	std::vector<bool> answer;

	for (std::size_t i = 0; i < other.key.size(); i++)
		if (other.basis[i] == my.basis[i]) answer.push_back(my.key[i]);	

	return answer;
}

//Возвращает битовую последовательность случайных бит из ключа key. Размер возвращаемой последовательности - 10% от размера key. Все биты, которые попали в выходную последовательность, из key удаляются.
std::vector<bool> get_qber_key(std::vector<bool> &key, unsigned int &seed)
{
	const double part = 0.1;

	seed = time(NULL);
	srandom(seed);

	std::vector<bool> answer;

	size_t size = key.size();
	answer.reserve(size*part);
	for (size_t i = 0; i < size*part; i++)
	{
		size_t pos = random()%size; 
		answer.push_back(key[pos]);
		key.erase(key.begin() + pos);
	}

	return answer;
}
#endif // ! COMMON_CPP