#pragma once
#include <memory>

namespace Halley
{
	class HalleyStaticsPimpl;
	class SystemAPI;

	class HalleyStatics
	{
	public:
		HalleyStatics();
		~HalleyStatics();
		void setupGlobals() const;
		void resume(SystemAPI* system);
		void suspend();
		
		void setMaskStorage(void* storage);
		void* getMaskStorage() const;

	private:
		std::unique_ptr<HalleyStaticsPimpl> pimpl;
	};
}
