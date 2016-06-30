#include <iostream>
#include <bitset>
#include <functional>

using namespace std;

class myclass
{
	char val[256];
};


struct detections
{
#define N 64
	bitset<N> basis;
	bitset<N> key;
	bitset<N> special;
	unsigned int count[N];
};

int main (void)
{
	cout << "Sizes of different objects" << endl;
	cout << "int: " << sizeof( int ) << endl;
	cout << "unsigned int: " << sizeof( unsigned int ) << endl;
	cout << "unsigned short int: " << sizeof( unsigned short int ) << endl;
	cout << "bool: " << sizeof( bool ) << endl;
	cout << "myclass:" << sizeof( myclass ) << endl;
	cout << "detections: " << sizeof( detections ) << endl;
	cout << "N = " << N << endl;

	int a;
	function<double()> f = [=](){return a;};
	cout << "func = " << sizeof(f) << endl;

	return 0;
}
