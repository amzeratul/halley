#pragma once

#include <vector>
#include <string>

class PrivateEngineData {};

class IRunner {
public:
	virtual ~IRunner() {}

	virtual PrivateEngineData* initialize(std::vector<std::string> args) = 0;
	virtual void terminate(PrivateEngineData* data) = 0;
	virtual bool step(PrivateEngineData* data) = 0;
};
