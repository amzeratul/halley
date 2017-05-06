#pragma once
#include "halley/core/api/halley_api_internal.h"

namespace Halley
{
	class AsioNetworkAPI : public NetworkAPIInternal
	{
	public:
		std::unique_ptr<NetworkService> createService(int port) override;
		void init() override;
		void deInit() override;
	};
}
