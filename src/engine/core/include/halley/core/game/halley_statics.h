#pragma once
#include <memory>

namespace Halley
{
	class HalleyStaticsShared;
	class HalleyStaticsPrivate;
	class SystemAPI;

	class HalleyStatics
	{
	public:
		HalleyStatics();
		~HalleyStatics();
		HalleyStatics(HalleyStatics& parent);
		
		void setupGlobals() const;
		void resume(SystemAPI* system);
		void suspend();
		
		void setMaskStorage(void* storage);
		void* getMaskStorage() const;

	private:
		std::shared_ptr<HalleyStaticsShared> sharedData;
		std::unique_ptr<HalleyStaticsPrivate> privateData;
	};
}
