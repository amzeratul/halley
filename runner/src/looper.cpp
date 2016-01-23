#include "looper.h"
#include "../../core/src/irunner.h"

void Looper::Run(IRunner& runner, std::vector<std::string> args)
{
	PrivateEngineData* data = runner.Initialize(args);
	bool run = true;
	while (run) {
		run = runner.Step(data);
	}
	runner.Terminate(data);
}