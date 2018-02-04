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
	class HalleyStaticsPimpl
	{
	public:
		HalleyStaticsPimpl()
		{
			logger = new Logger();
			maskStorage = MaskStorageInterface::createMaskStorage();
			os = OS::createOS();

			executors = std::make_unique<Executors>();
			executors->set(*executors);
		}

		~HalleyStaticsPimpl()
		{
			diskIOThreadPool.reset();
			cpuThreadPool.reset();
			cpuAuxThreadPool.reset();
			executors.reset();
		}

		Vector<TypeDeleterBase*> typeDeleters;
		void* maskStorage;
		OS* os;
		Logger* logger;
		
		std::unique_ptr<Executors> executors;
		std::unique_ptr<ThreadPool> cpuThreadPool;
		std::unique_ptr<ThreadPool> cpuAuxThreadPool;
		std::unique_ptr<ThreadPool> diskIOThreadPool;
	};
}

HalleyStatics::HalleyStatics()
	: pimpl(std::make_unique<HalleyStaticsPimpl>())
{
}

HalleyStatics::~HalleyStatics()
{
	pimpl.reset();
}

void HalleyStatics::resume(SystemAPI* system)
{
	setupGlobals();

#if HAS_THREADS
	auto makeThread = [=] (String name, std::function<void()> runnable) -> std::thread
	{
		if (system) {
			return system->createThread(name, runnable);
		} else {
			return std::thread([=] () {
				Concurrent::setThreadName(name);
				runnable();
			});
		}
	};

	pimpl->cpuThreadPool = std::make_unique<ThreadPool>("CPU", pimpl->executors->getCPU(), std::thread::hardware_concurrency(), makeThread);
	pimpl->cpuAuxThreadPool = std::make_unique<ThreadPool>("CPUAux", pimpl->executors->getCPUAux(), std::thread::hardware_concurrency(), makeThread);
	pimpl->diskIOThreadPool = std::make_unique<ThreadPool>("IO", pimpl->executors->getDiskIO(), 1, makeThread);
#endif
}

void HalleyStatics::setupGlobals() const
{
	Logger::setInstance(*pimpl->logger);

	ComponentDeleterTable::getDeleters() = &pimpl->typeDeleters;
	MaskStorageInterface::setMaskStorage(pimpl->maskStorage);
	OS::setInstance(pimpl->os);

	Executors::set(*pimpl->executors);
}

void HalleyStatics::suspend()
{
	pimpl->diskIOThreadPool.reset();
	pimpl->cpuThreadPool.reset();
	pimpl->cpuAuxThreadPool.reset();
}
