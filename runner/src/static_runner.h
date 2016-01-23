#pragma once
#include "../../core/src/irunner.h"

// TODO
class StaticRunner : public IRunner
{
public:
	PrivateEngineData* Initialize(std::vector<std::string> args) override;
	void Terminate(PrivateEngineData* data) override;
	bool Step(PrivateEngineData* data) override;
};
