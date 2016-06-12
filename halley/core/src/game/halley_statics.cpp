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

			executor = new Executor();
			ExecutorRunner::makeThreadPool(*executor, 4);
		}

		Vector<TypeDeleterBase*> typeDeleters;
		void* maskStorage;
		OS* os;
		Executor* executor;
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
	Executor::setDefault(*pimpl->executor);
	ComponentDeleterTable::getDeleters() = &pimpl->typeDeleters;
	MaskStorageInterface::setMaskStorage(pimpl->maskStorage);
	OS::setInstance(pimpl->os);
}
