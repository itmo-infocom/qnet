#ifndef __SOCKETS
#define __SOCKETS

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 

#include <types.h>
#include <net.db>//Для работы gethostbyname()

#include <iostream>
#include <pthread.h>
#include <linux/futex.h>
#include <string.h>

//Класс для общения по сети посредством сокета
class NetWork
{
	NetWork( int max_cli, int port, int* clients );
	NerWork( char* hostname, int port ) { connect( hostname, port); };
	~NetWork();
	
	void SendCmd( peers p = peers::peers_size, int cmd ) 
		{PrivateSend(p,&cmd,sizeof(cmd));};
	void SendData( peers p = peers::peers_size, void *cmd, size_t size ) 
		{PrivateRecv(p,cmd,size);};
	void RecvCmd( peers p = peers::peers_size, int &cmd ) 
		{PrivateSend(p,&cmd,sizeof(cmd));};
	void RecvData( peers p = peers::peers_size, void *cmd, size_t size )
		{PrivateSend(p,cmd,size);};
	
	bool IsReady(int p);//Возвращает - инициализирован ли сокет p
	
	enum NetComds
	{
	};
private:
	int fd;//Дескриптор сокета
	int *clients;
	int *cli_types;//Типы присоединившихся клиентов
	int clients_num;
	
	pthread_t thr;
	pthread_mutex_t mtx_cli;
	pthread_mutex_init(&mtx_cli, NULL);
	void *acception( NetWork* );
	
	PrivateSend( peers p, void *cmd, size_t size )
	{
		int fd;
		if (p < peers::peers_size) fd = clients[p];
		int bytes = 0;
		while (true)
		{
			bytes += send( fd, &cmd, size, 0 );
			if ( bytes < 0 )
			{
				std::cerr << strerror(errno) << std::endl;
				close(fd);
			}
			if ( bytes < size ) 
				if (errno == EAGAIN) continue;
				else throw Err::sock_send_recv;
			break;
		}
	};
	PrivateRecv( peers p, void *cmd, size_t size )
	{
		int fd;
		if (p < peers::peers_size) fd = clients[p];
		int bytes = 0;
		while (true)
		{
			bytes += recv( fd, &cmd, size, 0 );
			if ( bytes < 0 )
			{
				std::cerr << strerror(errno) << std::endl;
				close(fd);
			}
			if ( bytes < size ) 
				if (errno == EAGAIN) continue;
				else throw Err::sock_send_recv;
			break;
		}
	};
}

NetWork::NetWork( int max_cli, int port, int* clients )
{ 
	struct addrinfo hints;
	hints.sin_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_nex = NULL;
	
	struct addrinfo *res = nullptr;
	
	if ( int errno = getaddinfo( NULL, port, &hints, &res ) != 0 ) 
	{
		std::cerr << gai_strerror( errno ) << std::endl;
		throw errno;
	}

	struct addrinfo *rp;
	for ( rp = res; rp != NULL; rp = rp->ai_next ) {
		fd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
		if ( sfd == -1 ) continue;

		if ( bind( sfd, rp->ai_addr, rp->ai_addrlen ) == 0 )
			break;                  /* Success */

		close(sfd);
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
	for (int i = 0; i < clients_num; i++) clients = -1;
	
	pthread_create( thr, NULL, acception, this );
};

NetWork::NetWork( char* hostname, int port )
{
	fd = -1;
	
	struct addrinfo hints;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	
	struct addrinfo *res = nullptr;
	
	if ( int errno = getaddrinfo( "localhost", port, &hosts, &res) != 0 ) 
	{
		std::cerr << gai_strerror( errno ) << std::endl;
		throw errno;
	}
	
	struct addrinfo *res = nullptr;
	for ( rp = result; rp != NULL; rp = rp->ai_next ) 
	{
		fd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
			if ( fd == -1 ) continue;

			if ( connect(fd, rp->ai_addr, rp->ai_addrlen) != -1 )
				break;                  /* Success */

		close(fd);
	}
	
	if (rp == NULL) 
	{               /* Ни один из адресов не подошёл */
		std::cerr << "Could not connect" << std::endl;
		throw Err::sock_connect;
	}

	freeaddrinfo(res);
}

NetWork::~NetWork()
{
	pthread_cancel(thr);
	pthread_mutex_destroy(mtx_cli);
	delete clients;
	delete cli_types;
}

static void* NetWork::acception( NetWork* obj )
{
	while (true)
	{
		int client = accept( obj->fd, NULL, NULL );
		int peer_type;//После установки соединения клиент представляется
		RecvCmd( peer_type );
		pthread_mutex_lock(&mtx_cli);
			if ( peer_type < peers::peers_size ) clients[peer_type] = client;
			else close(client);
		pthread_mutex_unlock(&mtx_cli);
	}
}

bool IsReady(int p)
{
	pthread_mutex_lock(&mtx_cli);
		if (p < peers::peers_size) return clients[p] == -1;
		else return fd = -1;
	pthread_mutex_unlock(&mtx_cli);
}
#endif