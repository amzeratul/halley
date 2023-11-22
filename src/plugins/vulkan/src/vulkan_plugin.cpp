#include "vulkan_video.h"
#include <halley/plugin/plugin.h>

namespace Halley {
	
	class VulkanPlugin final : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new VulkanVideo(*system); }
		PluginType getType() override { return PluginType::GraphicsAPI; }
		String getName() override { return "Video/Vulkan"; }
		int getPriority() const override { return 1; }
	};
	
}

void initVulkanPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::VulkanPlugin>());
}
