#ifndef COMMON_CPP
#define COMMON_CPP
//Коды ошибок

//-------------------------------------------------------------------
//Общие константы
	//Коды ошибок
	enum Err
	{
		sock_create,
		sock_send_recv,
		sock_bind,
    sock_connect
	};
	
	//Перечисление всех видов клиентов
	enum peers
	{
		alice,
		bob,
		gui,
		telnet,
		
		peers_size
	};
//-------------------------------------------------------------------
//Общие типы данных и структуры
	//#include "detections.cpp"
	//#include "DMAFrame.cpp"
	//#include "sockets.cpp"
	#include "board_if.h"

//-------------------------------------------------------------------
//Общие функции

#endif // ! COMMON_CPP