#ifndef __DETECTIONS
#define __DETECTIONS
//Данная структура предназначена для обмена между Алисой и Бобом
//отсчётами, на которых было срабатывание детектора
	struct detections
	{
		vector<bool> basis;//Базис
		vector<bool> key;
		//Значение ключа в данном отсчёта
		vector<bool> special;
		//Для вычисления QBER или чего-нибудь странного

		vector<unsigned int> count;
		//Число временных отсчётов от предыдущего срабатывания
		
		push_back( detections &other )
		{
			for ( auto i : other.basis ) basis.push_back(i);
			for ( auto i : other.key ) key.push_back(i);
			for ( auto i : other.special ) special.push_back(i);
			for ( auto i : other.count ) count.push_back(i);
		};
	};
#endif