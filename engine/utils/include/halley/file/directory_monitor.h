#pragma once

#include <memory>

namespace Halley
{
	class Path;
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
