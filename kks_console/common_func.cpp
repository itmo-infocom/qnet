#ifndef COMMON_CPP
#define COMMON_CPP

#include <vector>

#include "detections.cpp"

const double max_qber = 6.5e-2;

//Режимы работы
enum regimes
{
	gen_key,

	regimes_size
};
#endif // ! COMMON_CPP