#pragma once

#include "halley/text/halleystring.h"
#include <memory>
#include <halley/data_structures/vector.h>

namespace Halley
{
	class HalleyAPIInternal;
	class SystemAPI;

	enum class PluginType
	{
		SystemAPI,
		GraphicsAPI,
		AudioOutputAPI,
		InputAPI,
		NetworkAPI
	};

	class Plugin
	{
	public:
		virtual ~Plugin() {}

		virtual PluginType getType() = 0;
		virtual String getName() = 0;
		virtual int getPriority() const { return 0; }

		virtual HalleyAPIInternal* createAPI(SystemAPI*) { return nullptr; }
	};

	class IPluginRegistry
	{
	public:
		virtual ~IPluginRegistry() {}
		virtual void registerPlugin(std::unique_ptr<Plugin> plugin) = 0;
		virtual Vector<Plugin*> getPlugins(PluginType type) = 0;
	};
}
