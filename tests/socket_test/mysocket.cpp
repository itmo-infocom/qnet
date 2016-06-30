#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <iostream>

using namespace std;

int main (void)
{
	//Server
	int sockfd = socket( AF_INET, SOCK_STREAM, 0 );

	if (sockfd < 0)
	{
		cerr << "Cannot create socket" << endl;
		return 1;
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( 50000 );
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof( addr )) < 0)
	{
		cerr << "Cannot bind socket" << endl;
		return 2;
	}

	if ( listen( sockfd, 2 ) < 0 )
	{
		cerr << "Cannot listen socket" << endl;
		return 3;
	}

	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof( cli_addr );
	int clisockfd  = accept( sockfd, (struct sockaddr *)&cli_addr, &cli_len);

	if ( clisockfd < 0 )
	{
		cerr << "Cannot accept connetion" << endl;
		return 4;
	}
	
	//Communication
	char buf[256];
	int n = recv( clisockfd, buf, 255, 0 );
	if ( n < 0 ) 
	{
		cerr << "Error reading buffer" << endl;
		return 5;
	}

	cout << "Recieved message: " << buf << endl;

	n = send( clisockfd, "I recieved your message.\n", 25, 0 );
	if ( n < 0 )
	{
		cerr << "Cannot send a message" << endl;
		return 6;
	}

	cout << "Always was fine - bye bye!" << endl;

	return 0;
}
