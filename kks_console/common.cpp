//Коды ошибок
	#define CANNOT_CREATE_SOCKET
	#define CANNOT_CONNECT
	#define CANNOT_SEND

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

//-------------------------------------------------------------------
//Общие функции
