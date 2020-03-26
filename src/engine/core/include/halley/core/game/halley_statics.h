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
		
		void setupGlobals() const;
		void resume(SystemAPI* system);
		void suspend();

	private:
		std::shared_ptr<HalleyStaticsShared> sharedData;
	};
}
