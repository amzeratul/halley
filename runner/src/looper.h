#pragma once

#include <vector>
#include <string>

#include "irunner.h"

class IRunner;

class Looper {
public:
	void Run(IRunner& runner, std::vector<std::string> args);
};
