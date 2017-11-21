#pragma once
#include "api/halley_api_internal.h"

namespace Halley {
	class DummyPlatformAPI : public PlatformAPIInternal {
	public:
		void init() override;
		void deInit() override;
	};
}
