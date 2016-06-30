#include <iostream>

using namespace std;

int main (void)
{
	char buf[80];
	int i = 0;

	while( true )
	{
		std::cin >> buf;
		std::cout << i << " : " << buf << std::endl;
	}
	
	return 0;
}
