#pragma once
#include <memory>

namespace Halley
{
	class HalleyStaticsPimpl;

	class HalleyStatics
	{
	public:
		HalleyStatics();
		~HalleyStatics();
		void setup();

	private:
		std::unique_ptr<HalleyStaticsPimpl> pimpl;
	};
}
