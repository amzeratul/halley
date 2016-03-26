#pragma once

#include "halley_core.h"
//#include <SDL_main.h>
#include "main_wrapper.h"

// Macro to implement program
#define HalleyGame(T) int main(int argc, char* argv[]) { return Halley::MainWrapper::main<T>(argc, argv); }
