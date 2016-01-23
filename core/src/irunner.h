#pragma once

#include <vector>
#include <string>

class PrivateEngineData {};

class IRunner {
public:
	virtual ~IRunner() {}

	virtual PrivateEngineData* Initialize(std::vector<std::string> args) = 0;
	virtual void Terminate(PrivateEngineData* data) = 0;
	virtual bool Step(PrivateEngineData* data) = 0;
};
