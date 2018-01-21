#include "dx11_video.h"
#include <halley/plugin/plugin.h>

namespace Halley {
	
	class DX11Plugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new DX11Video(*system); }
		PluginType getType() override { return PluginType::GraphicsAPI; }
		String getName() override { return "Video/DX11"; }
		int getPriority() const override { return 1; }
	};
	
}

void initDX11Plugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::DX11Plugin>());
}
