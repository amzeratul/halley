#include "dx12_video.h"
#include <halley/plugin/plugin.h>

namespace Halley {

    class DX12Plugin final : public Plugin {
        HalleyAPIInternal* createAPI(SystemAPI* system) override { return new DX12Video(*system); }
        PluginType getType() override { return PluginType::GraphicsAPI; }
        String getName() override { return "Video/DX12"; }
        int getPriority() const override { return 1; }
    };

}

void initDX12Plugin(Halley::IPluginRegistry &registry)
{
    registry.registerPlugin(std::make_unique<Halley::DX12Plugin>());
}
