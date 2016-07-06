#ifndef COMMON_CPP
#define COMMON_CPP
//Коды ошибок

//-------------------------------------------------------------------
//Общие константы
	enum Err
	{
		sock_create,
		sock_send_recv,
		sock_bind,
    sock_connect
	};
	
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
	#include "detections.cpp"
	#include "DMAFrame.cpp"
	#include "sockets.cpp"
	#include "board_if.cpp"

//-------------------------------------------------------------------
//Общие функции

#endif