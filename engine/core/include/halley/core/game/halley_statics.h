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
		void suspend();

	private:
		std::unique_ptr<HalleyStaticsPimpl> pimpl;
	};
}
