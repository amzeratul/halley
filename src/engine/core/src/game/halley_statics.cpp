#include "game/halley_statics.h"
#include <halley/entity/type_deleter.h>
#include <halley/data_structures/vector.h>
#include <halley/entity/family_mask.h>
#include <halley/os/os.h>
#include <halley/concurrency/executor.h>
#include <halley/concurrency/concurrent.h>
#include <thread>
#include "halley/support/logger.h"
#include "api/system_api.h"

using namespace Halley;

namespace Halley {
	class HalleyStaticsShared
	{
	public:
		HalleyStaticsShared()
		{
			logger = new Logger();
			os = OS::createOS();

			executors = std::make_unique<Executors>();
			executors->setInstance(*executors);
		}

		~HalleyStaticsShared()
		{
			diskIOThreadPool.reset();
			cpuThreadPool.reset();
			cpuAuxThreadPool.reset();
			executors.reset();
		}

		OS* os = nullptr;
		Logger* logger;
		
		std::unique_ptr<Executors> executors;
		std::unique_ptr<ThreadPool> cpuThreadPool;
		std::unique_ptr<ThreadPool> cpuAuxThreadPool;
		std::unique_ptr<ThreadPool> diskIOThreadPool;
	};
}

HalleyStatics::HalleyStatics()
	: sharedData(std::make_shared<HalleyStaticsShared>())
{
}

HalleyStatics::HalleyStatics(const HalleyStatics& other)
	: sharedData(other.sharedData)
{
}

HalleyStatics::~HalleyStatics()
{
	sharedData.reset();
}

void HalleyStatics::resume(SystemAPI* system, size_t maxThreads)
{
	setupGlobals();

#if HAS_THREADS
	auto makeThread = [=] (String name, std::function<void()> runnable) -> std::thread
	{
		if (system) {
			return system->createThread(name, ThreadPriority::Normal, runnable);
		} else {
			return std::thread([=] () {
				runnable();
			});
		}
	};

	sharedData->cpuThreadPool = std::make_unique<ThreadPool>("CPU", sharedData->executors->getCPU(), maxThreads, makeThread);
	sharedData->cpuAuxThreadPool = std::make_unique<ThreadPool>("CPUAux", sharedData->executors->getCPUAux(), maxThreads, makeThread);
	sharedData->diskIOThreadPool = std::make_unique<ThreadPool>("IO", sharedData->executors->getDiskIO(), 1, makeThread);
#endif
}

void HalleyStatics::setupGlobals() const
{
	Expects(sharedData);
	
	Logger::setInstance(*sharedData->logger);
	OS::setInstance(sharedData->os);
	Executors::setInstance(*sharedData->executors);
}

void HalleyStatics::suspend()
{
	sharedData->diskIOThreadPool.reset();
	sharedData->cpuThreadPool.reset();
	sharedData->cpuAuxThreadPool.reset();
}

