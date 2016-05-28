#pragma once

#include <cstdlib>

namespace Halley {
	class GameReloader
	{
	public:
		virtual ~GameReloader() {}

		virtual bool needsToReload() const { return false; }
		virtual void reload() {}
	};
}
