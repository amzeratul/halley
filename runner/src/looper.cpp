#include "looper.h"
#include "../../core/src/external/irunner.h"

void Looper::Run(IRunner& runner, std::vector<std::string> args)
{
	PrivateEngineData* data = runner.initialize(args);
	bool run = true;
	while (run) {
		run = runner.step(data);
	}
	runner.terminate(data);
}