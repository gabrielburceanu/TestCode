#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <iostream>

namespace Utils
{
	struct Point
	{
		int x;
		int y;
		Point(int x, int y) : x(x), y(y) {}
	};
	void logSDLError(const char* msg);
};

#endif//COMMON_UTILS_H
