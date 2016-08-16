#ifndef SOCKETS_CPP
#define SOCKETS_CPP

#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <pthread.h>
#include <linux/futex.h>
#include <string>
#include <errno.h>
#include <unistd.h>
#include <vector>

#include "common_func.cpp"
#include "detections.cpp"

namespace send_recv
{
	using namespace std;

	enum peers
	{
		alice,
		bob,
		gui,
		
		peers_size
	};

	//Структура для обработки исключений
	struct except{
		std::string errstr;
		except(string e)//e - имя функции, вобудившей исключение
		{
			errstr = e; errstr += ": ";
			errstr += gai_strerror(errno);
		}
	};

	//В данном пространстве имён определены переменные и типы, необходимые для работы сокета. Работа с ними напрямую не рекоммендуется.
	namespace details {
		class send_recv
		{
		public:
			void Send( int number )
				{
					void *pnumber = &number;
					//TODO: Обновить в памяти поведение при различных ошибках отправки/получения
					if (send(fd, pnumber, sizeof(number), MSG_WAITALL) == -1) throw except("send_recv send");
				};
			void Recv( int &number ) 
				{
					void *pnumber = &number;
					if (recv(fd, pnumber, sizeof(number), MSG_WAITALL) == -1) throw except("send_recv recv");
				};
			
			//TODO: Скопировать реализации из send_recv, удалив в них выбор между пирами
			void Send(std::vector<bool> &v);
			void Recv(std::vector<bool> &v);

			void Send(std::vector<unsigned int> &v);
			void Recv(std::vector<unsigned int> &v);

			void Send(detections &d);
			void Recv(detections &d);
		protected:
			int fd;
		};

		void send_recv::Send(std::vector<bool> &v)
		{
			Send((int)v.size());//Сообщили получателю из скольких элементов состоит массив
			
			if (v.size() == 0) return;
			
			size_t bytes = v.size()/8 + ((v.size()%8)?1:0); 
			//v.resize(bytes*8, false);
					
			unsigned char *package = new unsigned char [bytes];
			for (size_t i = 0; i < bytes; i++) package[i] = 0;
			for (size_t i = 0; i < v.size(); i++)
				package[i/8] |= (v[i] & 0b1) << (i%8);

			if (send(fd, (void*)package, bytes, MSG_WAITALL) == -1) throw except("Send vector<bool>");
			
			delete package;
		};
		void send_recv::Recv(std::vector<bool> &v)
		{
			int size;
			Recv(size);
			if (size == 0) return;

			size_t bytes = size/8 + ((size%8)?1:0);

			unsigned char *package = new unsigned char[bytes];
			if (recv(fd, (void*)package, bytes, MSG_WAITALL) == -1) throw except("Recv vector<bool>");
			
			for (size_t i = 0; i < (size_t)size; i++)
			v.push_back(package[i/8] & (0b1 << (i%8)));

			delete package;
		};

		void send_recv::Send(vector<unsigned int> &v)
		{
			Send((int)v.size());

			if (v.size() == 0) return;
			
			void *ptr = &v[0];
			if (send(fd, (void*)&v[0], v.size()*sizeof(v.front()), MSG_WAITALL) == -1)
				throw except("Send vector<unsigned int>");
		}
		void send_recv::Recv(vector<unsigned int> &v)
		{
			int size;
			Recv(size);

			if (v.size() == 0) return;

			v.resize(size);
			void *ptr = &v[0];
			if (recv(fd, (void*)&v[0], v.size()*sizeof(v.front()), MSG_WAITALL) == -1)
				throw except("Recv vector<unsigned int>");
		}

		void send_recv::Send(detections &d, peers p)
		{
			Send(d.basis, p);
			Send(d.key, p);
			Send(d.special, p);
			Send(d.count, p);
		};
		void send_recv::Recv(detections &d, peers p)
		{
			Recv(d.basis, p);
			Recv(d.key, p);
			Recv(d.special, p);
			Recv(d.count, p);
		};
	}

	class client: public details::send_recv
	{
	public:
		client(char *_hostname, char *_port);
		~client();
	private:
		//Файловый дескриптора сокета
		string hostname;
		string port;

		//Процедура выполняющая подключение к hostname через порт port
		void connection_sequence(void);
	};

	static void *acception( void* arg );//Существует в отдельном потоке и принимает входящие подключения - создаётся только для сервера
	class server: public details::send_recv
	{
		server(char *port);
	};

	class send_recv
	{
	public:
		send_recv( int _max_cli, char *_port );//Сервер
		send_recv( char *_hostname, char *_port );//Клиент
		~send_recv();
		
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
			if (send(fd, cmd, size, MSG_WAITALL) == -1) throw except("send_recv send");
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
						else send_recv(hostname, port);
					}
		};
	};

	//Поток, который устанавливает входящие подключения
	static void* acception( void* arg )
	{
		send_recv *obj = (send_recv*)arg;
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

	send_recv::send_recv( int _max_cli, char* _port )
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

	client::client(char *_hostname, char *_port)
	{
		fd = -1;
		hostname = _hostname;
		port = _port;

		connection_sequence();
	}

	void client::connection_sequence(void)
	{
		close(fd);

		struct addrinfo hints;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;
		
		struct addrinfo *res = nullptr;//Здесь будут все возможные варианты, подходящие под hostname

		if ( getaddrinfo( hostname.c_str(), port.c_str(), &hints, &res ) != 0 )
			throw except("getaddrinfo");
		
		struct addrinfo *rp;
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

	send_recv::~send_recv()
	{
		pthread_cancel(thread_acception);
		pthread_mutex_destroy(&mtx_cli_list);
		delete clients;
		delete cli_types;
	};

	

	bool send_recv::IsReady(int p)
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