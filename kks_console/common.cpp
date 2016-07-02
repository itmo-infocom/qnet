//Коды ошибок
	#define CANNOT_CREATE_SOCKET
	#define CANNOT_CONNECT
	#define CANNOT_SEND

//-------------------------------------------------------------------
//Общие константы
	//Чтобы отличить отправителя от получателя
		enum device
		{
			bob,
			alice,
			GUI
		};
  
  //Перечисление режимов работы
  enum regimes_list
  {
    generation,//Режим генерации ключа

    regime_Illegal //Для создания массивов и контроля ошибок
  };

//-------------------------------------------------------------------
//Общие типы данных и структуры
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
		};

//-------------------------------------------------------------------
//Общие функции
	  //Выполняет отправку/приём vector<bool> в/из сокета sock
    bool my_send( int sock, vector<bool> &v)
    {
      bool *to_send = new bool[v.size()];
      for (size_t i = 0; i < v.size(); i++) to_send[i] = v[i];

      //Теперь отправим сформированный to_send по сети, указав в качестве размера именно число бит
      return send( sock, to_send, v.size(), 0 ) < 0;
    }

    bool my_recv( int sock, vector<bool> &v)
    {
      bool *recieve = nullptr;
      unsigned int size;
      int r;//Число принятых байт
      r = recv( sock, &size, sizeof(size), 0 );//Приняли сколько бит должно придти

      if ( r < 0 ) return false;
      if ( r < (int)sizeof(size) ) return false;

      r = recv( sock, recieve, sizeof(size), 0 );//Считали это число бит
      if ( r < 0 ) return false;
      if ( r < (int)sizeof(size) ) return false;

      return true;
    }
