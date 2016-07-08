#ifndef SOCKETS_CPP
#define SOCKETS_CPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <sys/types.h>
#include <netdb.h>//Для работы gethostbyname()

#include <pthread.h>
#include <linux/futex.h>
#include <string>
#include <errno.h>
#include <unistd.h>
#include <vector>

//#include "common_func.cpp"
#include "detections.cpp"

namespace NetWork
{
	using namespace std;
	static void *acception( void* arg );//Существует в отдельном потоке и принимает входящие подключения - создаётся только для сервера

	enum peers
	{
		alice,
		bob,
		gui,
		telnet,
		
		peers_size
	};

	class NetWork
	{
	public:
		NetWork( int _max_cli, char *_port );//Сервер
		NetWork( char *_hostname, char *_port );//Клиент
		~NetWork();
		
		void Send( int cmd, peers p = peers_size )
			{
				void *pcmd = &cmd;
				PrivateSend(p, pcmd, sizeof(cmd));
			};
		void Send( void *cmd, size_t size, peers p = peers_size ) 
			{PrivateSend(p, cmd, size);};
		void Recv( int &cmd, int p = peers_size ) 
			{
				void *pcmd = &cmd;
				PrivateRecv(p, pcmd, sizeof(cmd));
			};
		void Recv( void *&cmd, size_t size, peers p = peers_size )
			{PrivateRecv(p, cmd, size);};
		
		void Send(std::vector<bool> &v, peers p = peers_size);
		void Recv(std::vector<bool> &v, peers p = peers_size);

		void Send(std::vector<unsigned int> &v, peers p = peers_size);
		void Recv(std::vector<unsigned int> &v, peers p = peers_size);

		void Send(detections &d, peers p = peers_size);
		void Recv(detections &d, peers p = peers_size);
		
		bool IsReady(int p);//Возвращает - подключён ли клиент p
		
		//Список команд, которые передаём по сети
		enum NetComds
		{
		};

		struct except{
			std::string errstr;
			except(string e)//e - имя функции, вобудившей исключение
			{
				errstr = e; errstr += ": ";
				errstr += gai_strerror(errno);
			}
		};
	private:
		int fd;//Дескриптор сокета
		int *clients;//Дескрипторы клиентов
		int *cli_types;//Типы присоединившихся клиентов
		int clients_num;//Текущее число подключенных клиентов
		char *hostname;//Адрес хоста для подключения
		int max_cli;//Максимальное число подключенных клиентов
		char *port;//Номер порта или имя сервиса (напр.ftp, http)  в виде текстовой строки (напр."50000" иди "google.com")
		bool server;//Определяет тип сокета - сервер или клиент
		
		pthread_t thread_acception;//поток, в котором слушается сокет на входящие соединения
		pthread_mutex_t mtx_cli_list = PTHREAD_MUTEX_INITIALIZER;//Блокировка списка дескрипторов клиентов
		pthread_mutex_t mtx_server = PTHREAD_MUTEX_INITIALIZER;//Блокировка на соединение с сервером
		friend void* acception( void* arg );
		
		void PrivateSend( int p, void *cmd, size_t size ) throw(except)
		{
			//TODO: Сделать так, чтобы при разрыве после восстановления соединения всё продолжналось с того же места
			int fd;
			if (p < peers_size) fd = clients[p];
			while (true)
			if (send(fd, cmd, size, MSG_WAITALL) == -1) throw except("NetWork send");
			else break;
		};

		void PrivateRecv( int p, void *&cmd, size_t size )
		{
			int fd;
			cmd = new unsigned char [size];
			if (p < peers_size) fd = clients[p];
			while (true)
				if (recv( fd, cmd, size, MSG_WAITALL ) != -1) break;
					else {
						if (server) sleep(1);
						else NetWork(hostname, port);
					}
		};
	};

	//Поток, который устанавливает входящие подключения
	static void* acception( void* arg )
	{
		NetWork *obj = (NetWork*)arg;
			while (true)
			{
				int client = accept( obj->fd, NULL, NULL );
				int *peer_type;
				//После установки соединения клиент представляется
				recv(client, peer_type, sizeof(int), MSG_WAITALL);
				
				pthread_mutex_lock(&obj->mtx_cli_list);
				if ( *peer_type < peers_size ) obj->clients[*peer_type] = client;
					else close(client);
				pthread_mutex_unlock(&obj->mtx_cli_list);
			}
		return 0;
	}

	NetWork::NetWork( int _max_cli, char* _port )
	{ 
		max_cli = _max_cli;
		server = true;
		struct addrinfo hints;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags |= AI_PASSIVE;
		hints.ai_protocol = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;
		
		struct addrinfo *res = nullptr;
		
		if ( getaddrinfo( NULL, _port, &hints, &res ) != 0 ) 
		throw except("getaddrinfo");
		
		struct addrinfo *rp;//указатель на один из вариантов, к кому подключаться
		for ( rp = res; rp != NULL; rp = rp->ai_next ) {
			fd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
			if ( fd == -1 ) 
				continue;

			if ( bind( fd, rp->ai_addr, rp->ai_addrlen ) == 0 )
				break;                  /* Success */

			close(fd);
		}
		
		if ( rp == NULL ) //Ни один из адресов не подошёл
			throw except("bind");
		
		freeaddrinfo(res);
		listen(fd, _max_cli);
		
		clients_num = 0;
		clients = new int[clients_num];
		cli_types = new int[clients_num];
		for (int i = 0; i < clients_num; i++) clients[i] = -1;
		
		pthread_create( &thread_acception, NULL, &acception, &fd );
	};

	NetWork::NetWork( char* _hostname, char* _port )
	{
		fd = -1;
		hostname = _hostname;
		port = _port;
		server = false;
		
		struct addrinfo hints;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;
		
		struct addrinfo *res = nullptr;

		if ( getaddrinfo( _hostname, _port, &hints, &res ) != 0 ) 
			throw except("getaddrinfo");
		
		struct addrinfo *rp;//Результат у

		while(fd == -1)
		{
			for ( rp = res; rp != NULL; rp = rp->ai_next ) 
			{
				fd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
					if ( fd == -1 )
					 continue;

					if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1 )
						break;                  /* Успех */

				close(fd);
				fd = -1;
				sleep(1);
			}
		}
		


		if (rp == NULL) /* Ни один из адресов за все попытки не подошёл */
			throw except("Cannot connect");

		freeaddrinfo(res);
	}

	NetWork::~NetWork()
	{
		pthread_cancel(thread_acception);
		pthread_mutex_destroy(&mtx_cli_list);
		delete clients;
		delete cli_types;
	};

	void NetWork::Send(std::vector<bool> &v, peers p)
	{
		Send((int)v.size(), p);//Сообщили получателю из скольких элементов состоит массив
		
		if (v.size() == 0) return;
		
		size_t bytes = v.size()/8 + ((v.size()%8)?1:0); 
		//v.resize(bytes*8, false);
				
		unsigned char *package = new unsigned char [bytes];
		for (size_t i = 0; i < bytes; i++) package[i] = 0;
		for (size_t i = 0; i < v.size(); i++)
 			package[i/8] |= (v[i] & 0b1) << (i%8);

		Send((void*)package, bytes, p);
	};
	void NetWork::Recv(std::vector<bool> &v, peers p)
	{
		int size;
		Recv(size, p);
		if (size == 0) return;

		size_t bytes = size/8 + ((size%8)?1:0);
		void *r;
		Recv(r, bytes, p);

		unsigned char *package = (unsigned char*)r;
		for (size_t i = 0; i < (size_t)size; i++)
		v.push_back(package[i/8] & (0b1 << (i%8)));
	};

	void NetWork::Send(vector<unsigned int> &v, peers p)
	{
		Send((int)v.size(), p);

		if (v.size() == 0) return;
		
		void *ptr = &v[0];
		PrivateSend(p, ptr, v.size()*sizeof(v.front()));
	}
	void NetWork::Recv(vector<unsigned int> &v, peers p)
	{
		int size;
		Recv(size, p);

		if (v.size() == 0) return;

		v.resize(size);
		void *ptr = &v[0];
		PrivateRecv(p, ptr, v.size()*sizeof(v.front()));
	}

	void NetWork::Send(detections &d, peers p)
	{
		Send(d.basis, p);
		Send(d.key, p);
		Send(d.special, p);
		Send(d.count, p);
	};
	void NetWork::Recv(detections &d, peers p)
	{
		Recv(d.basis, p);
		Recv(d.key, p);
		Recv(d.special, p);
		Recv(d.count, p);
	};

	bool NetWork::IsReady(int p)
	{
		pthread_mutex_lock(&mtx_cli_list);
			if (p < peers::peers_size) return clients[p] == -1;
			else 
		{
		fd = -1;
		return fd;
		}
		pthread_mutex_unlock(&mtx_cli_list);
	};
}
#endif