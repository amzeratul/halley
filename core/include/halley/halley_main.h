#pragma once

namespace Halley {} // Get GitHub to realise this is C++ :3

#include "halley_core.h"
//#include <SDL_main.h>
#include "main_wrapper.h"

// Macro to implement program
#define HalleyGame(T) int main(int argc, char* argv[]) { return Halley::MainWrapper::main<T>(argc, argv); }
