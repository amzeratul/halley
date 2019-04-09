#include "video_metal.h"
#include <halley/plugin/plugin.h>

namespace Halley {

  class MetalPlugin : public Plugin {
    HalleyAPIInternal* createAPI(SystemAPI* system) override { return new VideoMetal(*system); }
    PluginType getType() override { return PluginType::GraphicsAPI; }
    String getName() override { return "Video/Metal"; }
  };

}

void initMetalPlugin(Halley::IPluginRegistry &registry)
{
  registry.registerPlugin(std::make_unique<Halley::MetalPlugin>());
}
