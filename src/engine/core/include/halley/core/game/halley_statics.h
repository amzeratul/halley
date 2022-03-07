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
		HalleyStatics(const HalleyStatics& other);
		~HalleyStatics();
		
		void setupGlobals() const;
		void resume(SystemAPI* system, size_t maxThreads);
		void suspend();

	private:
		std::shared_ptr<HalleyStaticsShared> sharedData;
	};
}
