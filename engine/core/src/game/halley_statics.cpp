#include "game/halley_statics.h"
#include <halley/entity/type_deleter.h>
#include <halley/data_structures/vector.h>
#include <halley/entity/family_mask.h>
#include <halley/os/os.h>
#include <halley/concurrency/executor.h>
#include <halley/concurrency/concurrent.h>
#include <thread>
#include "halley/support/logger.h"

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
			executors.reset();
		}

		Vector<TypeDeleterBase*> typeDeleters;
		void* maskStorage;
		OS* os;
		Logger* logger;
		
		std::unique_ptr<Executors> executors;
		std::unique_ptr<ThreadPool> cpuThreadPool;
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

void HalleyStatics::resume()
{
	setupGlobals();

#if HAS_THREADS
	pimpl->cpuThreadPool = std::make_unique<ThreadPool>(pimpl->executors->getCPU(), std::thread::hardware_concurrency());
	pimpl->diskIOThreadPool = std::make_unique<ThreadPool>(pimpl->executors->getDiskIO(), 1);
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
}
