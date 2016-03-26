#pragma once
#include "../../core/src/external/irunner.h"

// TODO
class StaticRunner : public IRunner
{
public:
	PrivateEngineData* initialize(std::vector<std::string> args) override;
	void terminate(PrivateEngineData* data) override;
	bool step(PrivateEngineData* data) override;
};
