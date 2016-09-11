#pragma once

#include "filesystem.h"
#include <memory>

namespace Halley
{
	class DirectoryMonitorPimpl;
	
	class DirectoryMonitor
	{
	public:
		explicit DirectoryMonitor(const Path& p);
		~DirectoryMonitor();

		bool poll();
		bool hasRealImplementation() const;

	private:
		std::unique_ptr<DirectoryMonitorPimpl> pimpl;
	};
}
