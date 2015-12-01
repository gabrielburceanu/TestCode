#include "Common.h"
#include <SDL.h>

namespace Utils
{

void logSDLError(const char* msg)
{
	std::cout << " error: "<< msg << " " << SDL_GetError() << std::endl;
}

}