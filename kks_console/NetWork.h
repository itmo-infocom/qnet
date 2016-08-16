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

namespace NetWork
{
	using namespace std;

	enum peers
	{
		alice,
		bob,
		gui,
		
		peers_size
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
					if (send(fd, pnumber, sizeof(number), MSG_WAITALL) == -1) throw except("send_recv send int");
				};
			void Recv( int &number ) 
				{
					void *pnumber = &number;
					if (recv(fd, pnumber, sizeof(number), MSG_WAITALL) == -1) throw except("send_recv recv int`");
				};
			
			void Send(unsigned int number)
			{
				int tmp = number;
				Send(tmp);
			}
			void Recv(unsigned int &number)
			{
				int tmp;
				Recv(tmp);
				number = tmp;
			}
			
			//TODO: Скопировать реализации из send_recv, удалив в них выбор между пирами
			void Send(std::vector<bool> v);
			void Recv(std::vector<bool> &v);

			void Send(std::vector<unsigned int> v);
			void Recv(std::vector<unsigned int> &v);

			void Send(detections d);
			void Recv(detections &d);

			//Структура для обработки исключений
			struct except{
				std::string errstr;
				except(string e)//e - имя функции, вобудившей исключение
				{
					errstr = e; errstr += ": ";
					errstr += gai_strerror(errno);
				}
			};
		protected:
			int fd;
		};

		void send_recv::Send(std::vector<bool> v)
		{
			Send((int)v.size());//Сообщили получателю из скольких элементов состоит массив
			
			if (v.size() == 0) return;
			
			size_t bytes = v.size()/8 + ((v.size()%8)?1:0); 
			//v.resize(bytes*8, false);
					
			vector<unsigned char> package(bytes,0);
			for (size_t i = 0; i < v.size(); i++)
				package[i/8] |= (v[i] & 0b1) << (i%8);

			if (send(fd, &package[0], bytes, MSG_WAITALL) == -1) throw except("Send vector<bool>");
		};
		void send_recv::Recv(std::vector<bool> &v)
		{
			int size;
			Recv(size);
			if (size == 0) return;

			size_t bytes = size/8 + ((size%8)?1:0);

			vector<unsigned char> package(bytes);
			if (recv(fd, &package[0], bytes, MSG_WAITALL) == -1) throw except("Recv vector<bool>");
			
			for (size_t i = 0; i < (size_t)size; i++)
			v.push_back(package[i/8] & (0b1 << (i%8)));
		};

		void send_recv::Send(vector<unsigned int> v)
		{
			Send((int)v.size());

			if (v.size() == 0) return;
			
			if (send(fd, &v[0], v.size()*sizeof(v.front()), MSG_WAITALL) == -1)
				throw except("Send vector<unsigned int>");
		}
		void send_recv::Recv(vector<unsigned int> &v)
		{
			int size;
			Recv(size);

			if (size == 0) return;

			v.resize(size);

			if (recv(fd, &v[0], v.size()*sizeof(v.front()), MSG_WAITALL) == -1)
				throw except("Recv vector<unsigned int>");
		}

		void send_recv::Send(detections d)
		{
			Send(d.basis);
			Send(d.key);
			Send(d.special);
			Send(d.count);
		};
		void send_recv::Recv(detections &d)
		{
			Recv(d.basis);
			Recv(d.key);
			Recv(d.special);
			Recv(d.count);
		};
	}

	class client: public details::send_recv
	{
	public:
		client(char *_hostname, char *_port);
		~client() {close(fd);};
	private:
		//Файловый дескриптора сокета
		string hostname;
		string port;

		//Процедура выполняющая подключение к hostname к порту port
		void connection_sequence(void);
	};

	class server: public details::send_recv
	{
	public:
		//Конструктор. flag - для указания работы в неблокирующем (SOCK_NONBLOCK) режиме (чтобы Боб не зависал GUI, который может никогда и не подключиться)
		server(char *port, int flag = 0);

		//Принимает входящее соединение с флагом flag - в блокирующем или неблокирующем режиме.
		//Возвращает успешность принятия соединения. 
		bool accept_cli(void);

		~server() {close(fd); close(socket_fd);}
	private:
		//Дескриптор сокета - для клиента используется fd из details::send_recv
		int socket_fd;
	};

	server::server(char* _port, int flag)
	{ 
		struct addrinfo hints;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE + flag;
		hints.ai_protocol = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;
		
		struct addrinfo *res = nullptr;
		
		//Кусок ниже - это копия из мануала. Вся эта канитель только ради использования getaddrinfo
		{
			{
				int loc_errno;
				if ( (loc_errno = getaddrinfo( NULL, _port, &hints, &res )) != 0 )
				{
					string tmp = "getaddrinfo: ";
					tmp += gai_strerror(loc_errno);
					throw except(tmp);
				}
			}
			
			struct addrinfo *rp;//указатель на один из вариантов, к кому подключаться
			for ( rp = res; rp != NULL; rp = rp->ai_next ) {
				socket_fd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
				if ( fd == -1 ) 
					continue;

				if ( bind( socket_fd, rp->ai_addr, rp->ai_addrlen ) == 0 )
					break;                  /* Success */

				close(socket_fd);
			}
			
			if ( rp == NULL ) //Ни один из адресов не подошёл
				throw except("bind");
			
			freeaddrinfo(res);
		}

		listen(socket_fd, 1);
	};

	bool server::accept_cli(void)
	{
		if ((fd = accept(socket_fd, NULL, NULL)) == -1)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN) return false;
			else throw except("accept");
		} else return true;
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

		{
			int loc_errno;
			if ( (loc_errno = getaddrinfo( hostname.c_str(), port.c_str(), &hints, &res )) != 0 )
			{
				string tmp = "getaddrinfo: ";
				tmp += gai_strerror(loc_errno);
				throw except(tmp);
			}
		}

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
}
#endif