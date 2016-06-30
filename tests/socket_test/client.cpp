#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <iostream>
#include <netdb.h>
#include <strings.h>

using namespace std;

int main ( int argc, char **argv )
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
	struct hostent *server = gethostbyname( argv[1] );

	bcopy( 	(char *) server->h_addr,
		(char *) &addr.sin_addr.s_addr,
		server->h_length );

	if ( connect( 
		sockfd, (struct sockaddr *)&addr, sizeof( addr ) ) < 0)
	{
		cerr << "Cannot connect socket" << endl;
		return 2;
	}

	cout << "Enter a message: ";
	char buf[256];
	cin >> buf;

	int n = send( sockfd, buf, 255, 0 );
	if ( n < 0 )
	{
		cerr << "Cannot send a message" << endl;
		return 3;
	}

	n = recv( sockfd, buf, 255, 0 );
	if ( n < 0 )
	{
		cerr << "Cannot recieve a message" << endl;
		return 4;
	}
	cout << buf << endl;
	cout << "Client - bye bye!" << endl;

	return 0;
}
