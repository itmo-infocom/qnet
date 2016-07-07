#ifndef DETECTIONS_CPP
#define DETECTIONS_CPP

#include <vector>

//Данная структура предназначена для обмена между Алисой и Бобом
//отсчётами, на которых было срабатывание детектора
	struct detections
	{
		std::vector<bool> basis;//Базис
		std::vector<bool> key;
		//Значение ключа в данном отсчёта
		std::vector<bool> special;
		//Для вычисления QBER или чего-нибудь странного

		std::vector<unsigned int> count;
		//Число временных отсчётов от предыдущего срабатывания
		
		//Добавляет другой объект detections
		void append( detections other )
		{
			other.count[0] += count.back();//Сшивка двух массивов
			count.pop_back();

			basis.insert(basis.end(), other.basis.begin(), other.basis.end());
			key.insert(key.end(), other.key.begin(), other.key.end());
			special.insert(special.end(), other.special.begin(), other.special.end());
			count.insert(count.end(), other.count.begin(), other.count.end());
		};
	};
#endif // !DETECTIONS_CPP
