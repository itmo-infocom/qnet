#ifndef COMMON_CPP
#define COMMON_CPP

#include <vector>

#include "detections.cpp"

const double max_qber = 6.5e-2;

std::vector<bool> sift_key(const detections &my, const detections &other, double &qber)
{
	using namespace std;
	unsigned int err_count = 0;//Число несовпавших бит qber
	unsigned int qber_count = 0;//Число бит, использованных для нахождения ключа 
	vector<bool> answer;
	unsigned int pos = 0;

	for (size_t i = 0; i < my.key.size(); i++)
	{
		if (other.special[i])
		{
			qber_count++;
			if (my.key[pos] != other.key[i]) err_count++;
			continue; 
		}
		if (my.basis[pos] == other.basis[i])
		answer.push_back(my.key[pos]);
	}

	qber = (double)err_count/qber_count;

	return answer;
}

//Возвращает пырк
std::vector<unsigned int> pyrk(std::size_t size)
{
	
}
#endif // ! COMMON_CPP