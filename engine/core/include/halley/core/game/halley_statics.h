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
		void setupGlobals() const;
		void resume();
		void suspend();

	private:
		std::unique_ptr<HalleyStaticsPimpl> pimpl;
	};
}
