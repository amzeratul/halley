#include "game/halley_statics.h"
#include <halley/entity/type_deleter.h>
#include <halley/data_structures/vector.h>
#include <halley/entity/family_mask.h>
#include <halley/os/os.h>
#include <halley/concurrency/executor.h>

using namespace Halley;

namespace Halley {
	class HalleyStaticsPimpl
	{
	public:
		HalleyStaticsPimpl()
		{
			maskStorage = MaskStorageInterface::createMaskStorage();
			os = OS::createOS();

			executors = new Executors();
			executors->set(*executors);
			threadPool = std::make_unique<ThreadPool>(executors->getCPU(), 4);
		}

		std::unique_ptr<ThreadPool> threadPool;
		Vector<TypeDeleterBase*> typeDeleters;
		void* maskStorage;
		OS* os;
		Executors* executors;
	};
}

HalleyStatics::HalleyStatics()
	: pimpl(std::make_unique<HalleyStaticsPimpl>())
{
}

HalleyStatics::~HalleyStatics()
{
}

void HalleyStatics::setup()
{
	Executors::set(*pimpl->executors);
	ComponentDeleterTable::getDeleters() = &pimpl->typeDeleters;
	MaskStorageInterface::setMaskStorage(pimpl->maskStorage);
	OS::setInstance(pimpl->os);
}
