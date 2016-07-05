#ifndef SOCKETS_CPP
#define SOCKETS_CPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <sys/types.h>
#include <netdb.h>//Для работы gethostbyname()

#include <iostream>
#include <pthread.h>
#include <linux/futex.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "common.cpp"

static void *acception( void* arg );

//Класс для общения по сети посредством сокета
class NetWork
{
public:
	NetWork( int max_cli, char *port );
	NetWork( char* hostname, char *port );
	~NetWork();
	
	void SendCmd( int cmd, int p = peers_size ) 
		{PrivateSend(p,&cmd,sizeof(cmd));};
	void SendData( void *cmd, size_t size, peers p = peers_size ) 
		{PrivateRecv(p,cmd,size);};
	void RecvCmd( int &cmd, int p = peers_size ) 
		{PrivateSend(p,&cmd,sizeof(cmd));};
	void RecvData( void *cmd, size_t size, int p = peers_size )
		{PrivateSend(p,cmd,size);};
	
	bool IsReady(int p);//Возвращает - подключён ли клиент p
	
	enum NetComds
	{
	};
private:
	int fd;//Дескриптор сокета
	int *clients;
	int *cli_types;//Типы присоединившихся клиентов
	int clients_num;
	
	pthread_t thr;
	pthread_mutex_t mtx_cli = PTHREAD_MUTEX_INITIALIZER;
  friend void* acception( void* arg );
	
	void PrivateSend( int p, void *cmd, size_t size )
	{
		int fd;
		if (p < peers_size) fd = clients[p];
		int bytes = 0;
		while (true)
		{
			bytes += send( fd, &cmd, size, 0 );
			if ( bytes < 0 )
			{
				std::cerr << strerror(errno) << std::endl;
				close(fd);
			}
			if ( bytes < (int)size )
      {
				if (errno == EAGAIN) continue;
				else throw sock_send_recv;
      }
			break;
		}
	};
	void PrivateRecv( int p, void *cmd, size_t size )
	{
		int fd;
		if (p < peers_size) fd = clients[p];
		int bytes = 0;
		while (true)
		{
			bytes += recv( fd, &cmd, size, 0 );
			if ( bytes < 0 )
			{
				std::cerr << strerror(errno) << std::endl;
				close(fd);
			}
			if ( bytes < (int)size ) 
			{
        if (errno == EAGAIN) continue;
				else throw sock_send_recv;
      }
			break;
		}
	};
};

//Для создания потока, в котором устанавливается соединение со всеми
static void* acception( void* arg )
{
  NetWork *obj = (NetWork*)arg;
	while (true)
	{
		int client = accept( obj->fd, NULL, NULL );
		int peer_type;//После установки соединения клиент представляется
    obj->RecvCmd( peer_type );
		
    pthread_mutex_lock(&obj->mtx_cli);
			if ( peer_type < peers_size ) obj->clients[peer_type] = client;
			else close(client);
		pthread_mutex_unlock(&obj->mtx_cli);
	}
  return 0;
}

NetWork::NetWork( int max_cli, char* port )
{ 
	struct addrinfo hints;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	
	struct addrinfo *res = nullptr;
	
	if ( getaddrinfo( NULL, port, &hints, &res ) != 0 ) 
	{
		std::cerr << gai_strerror( errno ) << std::endl;
		throw errno;
	}

	struct addrinfo *rp;//указатель на один из вариантов, к кому подключаться
	for ( rp = res; rp != NULL; rp = rp->ai_next ) {
		fd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
		if ( fd == -1 ) continue;

		if ( bind( fd, rp->ai_addr, rp->ai_addrlen ) == 0 )
			break;                  /* Success */

		close(fd);
	}
	
	if ( rp == NULL ) //Ни один из адресов не подошёл
	{
		std::cerr << "Cannot bind" << std::endl;
		throw Err::sock_bind;
	}
	
	freeaddrinfo(res);
	
	clients_num = max_cli;
	clients = new int[clients_num];
	cli_types = new int[clients_num];
	for (int i = 0; i < clients_num; i++) clients[i] = -1;
	
	pthread_create( &thr, NULL, &acception, &fd );
};

NetWork::NetWork( char* hostname, char* port )
{
	fd = -1;
	
	struct addrinfo hints;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	
	struct addrinfo *res = nullptr;

	if ( getaddrinfo( hostname, port, &hints, &res ) != 0 ) 
	{
		std::cerr << gai_strerror( errno ) << std::endl;
		throw errno;
	}

struct addrinfo *rp;//Результат у

//Цикл на число попыток
for (int trying = 0; trying < 10; trying++)
{
	for ( rp = res; rp != NULL; rp = rp->ai_next ) 
	{
		fd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
			if ( fd == -1 ) continue;

			if ( connect(fd, rp->ai_addr, rp->ai_addrlen) != -1 )
				break;                  /* Success */

		close(fd);
	}
	sleep(1);
}
	
	if (rp == NULL) 
	{               /* Ни один из адресов за все попытки не подошёл */
		std::cerr << "Could not connect" << std::endl;
		throw Err::sock_connect;
	}

	freeaddrinfo(res);
}

NetWork::~NetWork()
{
	pthread_cancel(thr);
	pthread_mutex_destroy(&mtx_cli);
	delete clients;
	delete cli_types;
}

bool NetWork::IsReady(int p)
{
	pthread_mutex_lock(&mtx_cli);
		if (p < peers::peers_size) return clients[p] == -1;
		else 
    {
      fd = -1;
      return fd;
    }
	pthread_mutex_unlock(&mtx_cli);
};
#endif