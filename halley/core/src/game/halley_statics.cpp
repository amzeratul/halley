#include "game/halley_statics.h"
#include <halley/entity/type_deleter.h>
#include <vector>
#include <halley/entity/family_mask.h>
#include <halley/os/os.h>

using namespace Halley;

namespace Halley {
	class HalleyStaticsPimpl
	{
	public:
		HalleyStaticsPimpl()
		{
			maskStorage = MaskStorageInterface::createMaskStorage();
			os = OS::createOS();
		}

		std::vector<TypeDeleterBase*> typeDeleters;
		void* maskStorage;
		OS* os;
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
	ComponentDeleterTable::getDeleters() = &pimpl->typeDeleters;
	MaskStorageInterface::setMaskStorage(pimpl->maskStorage);
	OS::setInstance(pimpl->os);
}
